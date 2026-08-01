#define _POSIX_C_SOURCE 200809L
#include "cubalc_jit.h"
#include "cubalc_async.h"
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <math.h>

/* Hybrid x86_64 JIT:
 * - Pure ALU/control emitted as machine code operating on reg[32] beside a thin ctx
 * - Cube host ops call C trampoline
 *
 * Layout of generated function:
 *   rdi = cubalc_jit_rt *
 *   rt->reg, rt->vm, rt->img, rt->pc
 */

typedef struct cubalc_jit_rt {
  int64_t reg[CUBALC_ISA_MAX_REG];
  cubalc_vm *vm;
  const cubalc_image *img;
  int pc;
  int halted;
  int fatal;
  int asserts_ok;
  int asserts_fail;
  int stmts;
  char last_print[256];
  char err[160];
  FILE *trace;
} cubalc_jit_rt;

/* Host trampoline for cube / I-O ops — called from JIT code */
static void cubalc_jit_host(cubalc_jit_rt *rt, uint8_t op, uint8_t a, uint8_t b, int32_t imm);

#if defined(__x86_64__) || defined(_M_X64)
#define CUBALC_JIT_X64 1
#else
#define CUBALC_JIT_X64 0
#endif

const char *cubalc_jit_backend(void) {
#if CUBALC_JIT_X64
  return "x86_64-hybrid";
#else
  return "interp";
#endif
}

/* ---- tiny x86 encoder ---- */
typedef struct {
  uint8_t *p;
  uint8_t *end;
} enc_t;

static int e_ok(enc_t *e, size_t n) { return e->p + n <= e->end; }
static void e1(enc_t *e, uint8_t b) { if (e_ok(e, 1)) *e->p++ = b; }
static void e2(enc_t *e, uint8_t a, uint8_t b) { e1(e, a); e1(e, b); }
static void e4(enc_t *e, uint32_t v) {
  if (!e_ok(e, 4)) return;
  memcpy(e->p, &v, 4); e->p += 4;
}
static void e8(enc_t *e, uint64_t v) {
  if (!e_ok(e, 8)) return;
  memcpy(e->p, &v, 8); e->p += 8;
}

/* mov rax, imm64 */
static void emit_mov_rax_imm(enc_t *e, uint64_t imm) {
  e1(e, 0x48); e1(e, 0xB8); e8(e, imm);
}
/* mov rdi, [rsp+off]  — we keep rt in r12 */
/* System V: on entry rsp%16==8. After 3 pushes rsp%16==0 for calls. */
static void emit_prologue(enc_t *e) {
  e1(e, 0x41); e1(e, 0x54);           /* push r12 */
  e1(e, 0x53);                         /* push rbx */
  e1(e, 0x50);                         /* push rax (align) */
  e2(e, 0x49, 0x89); e1(e, 0xFC);     /* mov r12, rdi */
}
static void emit_epilogue(enc_t *e) {
  e1(e, 0x58);                         /* pop rax */
  e1(e, 0x5B);                         /* pop rbx */
  e1(e, 0x41); e1(e, 0x5C);           /* pop r12 */
  e1(e, 0xC3);                         /* ret */
}

/* mov rax, [r12 + off] */
static void emit_load_reg(enc_t *e, int r) {
  uint32_t off = (uint32_t)(offsetof(cubalc_jit_rt, reg) + (size_t)r * 8);
  e1(e, 0x49); e1(e, 0x8B); e1(e, 0x84); e1(e, 0x24); e4(e, off); /* mov rax,[r12+off] */
}
/* mov [r12+off], rax */
static void emit_store_reg(enc_t *e, int r) {
  uint32_t off = (uint32_t)(offsetof(cubalc_jit_rt, reg) + (size_t)r * 8);
  e1(e, 0x49); e1(e, 0x89); e1(e, 0x84); e1(e, 0x24); e4(e, off);
}
/* mov rdx, [r12+off_b] */
static void emit_load_reg_rdx(enc_t *e, int r) {
  uint32_t off = (uint32_t)(offsetof(cubalc_jit_rt, reg) + (size_t)r * 8);
  e1(e, 0x49); e1(e, 0x8B); e1(e, 0x94); e1(e, 0x24); e4(e, off);
}

/* call host: args rdi=rt, esi=op, edx=a, ecx=b, r8d=imm */
static void emit_call_host(enc_t *e, uint8_t op, uint8_t a, uint8_t b, int32_t imm) {
  /* mov rdi, r12 */
  e2(e, 0x4C, 0x89); e1(e, 0xE7);
  /* mov esi, op */
  e1(e, 0xBE); e4(e, op);
  /* mov edx, a */
  e1(e, 0xBA); e4(e, a);
  /* mov ecx, b */
  e1(e, 0xB9); e4(e, b);
  /* mov r8d, imm */
  e1(e, 0x41); e1(e, 0xB8); e4(e, (uint32_t)imm);
  /* mov rax, imm64 host */
  emit_mov_rax_imm(e, (uint64_t)(uintptr_t)cubalc_jit_host);
  /* call rax */
  e2(e, 0xFF, 0xD0);
  /* check fatal: cmp dword [r12+fatal], 0; jne end */
}

static int find_cube(cubalc_vm *vm, const char *id) {
  for (int i = 0; i < vm->ch.n_cubes; i++)
    if (strcmp(vm->ch.cubes[i].id, id) == 0) return i;
  return -1;
}
static const char *IS(const cubalc_image *img, int i) {
  if (!img || i < 0 || i >= img->n_str) return "";
  return img->str[i];
}

static void cubalc_jit_host(cubalc_jit_rt *rt, uint8_t op, uint8_t a, uint8_t b, int32_t imm) {
  if (!rt || !rt->vm || !rt->img || rt->fatal) return;
  cubalc_vm *vm = rt->vm;
  const cubalc_image *img = rt->img;
  rt->stmts++;
  switch (op) {
  case OP_HOLD:
    vm->hold_flash = imm ? 1 : 0;
    vm->ch.hold_flash = (uint8_t)vm->hold_flash;
    break;
  case OP_GENESIS: {
    cubalc_matrix gen;
    const char *plate = IS(img, a);
    if (!plate[0]) plate = "NEXUS_COORD v1 | from=jit | hold_flash=1 |";
    cubalc_coord_to_matrix(plate, &gen);
    cubalc_chain_from_initial(&vm->ch, &gen, 1);
    vm->ch.hold_flash = (uint8_t)vm->hold_flash;
    break;
  }
  case OP_SPAWN: {
    if (vm->ch.n_cubes == 0) {
      cubalc_matrix gen;
      cubalc_coord_to_matrix("NEXUS_COORD v1 | from=jit | hold_flash=1 |", &gen);
      cubalc_chain_from_initial(&vm->ch, &gen, 1);
    }
    float x = (float)(vm->ch.n_cubes % 5) * 0.28f;
    float z = (float)(vm->ch.n_cubes / 5) * 0.28f;
    cubalc_cube_spawn(&vm->ch, IS(img, a), IS(img, b), (uint8_t)(imm ? 1 : 0), x, 0.f, z);
    break;
  }
  case OP_PLUG: {
    int ia = find_cube(vm, IS(img, a)), ib = find_cube(vm, IS(img, b));
    if (ia >= 0 && ib >= 0) cubalc_cube_plug(&vm->ch, ia, ib);
    break;
  }
  case OP_RING:
    for (int i = 0; i < vm->ch.n_cubes; i++)
      cubalc_cube_plug(&vm->ch, i, (i + 1) % vm->ch.n_cubes);
    break;
  case OP_IMPULSE:
    cubalc_chain_impulse(&vm->ch, IS(img, a), (uint8_t)(imm ? 1 : 0));
    break;
  case OP_FLOW:
    for (int t = 0; t < imm; t++) cubalc_chain_flow(&vm->ch);
    break;
  case OP_FLOW_P:
    cubalc_async_chain_flow(&vm->ch, imm > 0 ? imm : 1);
    break;
  case OP_SETBIT: {
    int ix = find_cube(vm, IS(img, a));
    if (ix >= 0) {
      cubalc_matrix_set(&vm->ch.cubes[ix].atom.matrix, b, imm ? 1 : 0);
      vm->ch.cubes[ix].atom.digit =
        (uint8_t)cubalc_algocube_digit(&vm->ch.cubes[ix].atom.matrix);
    }
    break;
  }
  case OP_GETSET: {
    int ix = find_cube(vm, IS(img, b));
    if (a < CUBALC_ISA_MAX_REG)
      rt->reg[a] = ix >= 0 ? cubalc_matrix_popcount(&vm->ch.cubes[ix].atom.matrix) : 0;
    break;
  }
  case OP_GETCUBES:
    if (a < CUBALC_ISA_MAX_REG) rt->reg[a] = vm->ch.n_cubes;
    break;
  case OP_ENERGY: {
    int ix = find_cube(vm, IS(img, b));
    if (a < CUBALC_ISA_MAX_REG)
      rt->reg[a] = ix >= 0 ? (int64_t)lround(vm->ch.cubes[ix].atom.energy * 100.0) : 0;
    break;
  }
  case OP_PRINT: {
    char line[256];
    snprintf(line, sizeof line, "%s %lld", IS(img, a),
             (long long)(b < CUBALC_ISA_MAX_REG ? rt->reg[b] : 0));
    if (rt->trace) fprintf(rt->trace, "%s\n", line);
    snprintf(rt->last_print, sizeof rt->last_print, "%s", line);
    break;
  }
  case OP_ASSERT:
    if (a < CUBALC_ISA_MAX_REG && rt->reg[a]) {
      rt->asserts_ok++;
      if (rt->trace) fprintf(rt->trace, "# ASSERT ok\n");
    } else {
      rt->asserts_fail++;
      rt->fatal = 1;
      snprintf(rt->err, sizeof rt->err, "ASSERT failed (jit)");
    }
    break;
  case OP_DECIDE: {
    int ix = -1;
    if (imm && a < img->n_str) ix = find_cube(vm, IS(img, a));
    if (ix < 0) {
      for (int i = 0; i < vm->ch.n_cubes; i++)
        if (strstr(vm->ch.cubes[i].id, "brain") || strstr(vm->ch.cubes[i].role, "brain")) {
          ix = i; break;
        }
      if (ix < 0 && vm->ch.n_cubes > 0) ix = 0;
    }
    if (ix >= 0) {
      vm->ch.cubes[ix].atom.digit =
        (uint8_t)cubalc_algocube_digit(&vm->ch.cubes[ix].atom.matrix);
      rt->reg[0] = vm->ch.cubes[ix].atom.digit;
    } else rt->reg[0] = 4;
    break;
  }
  case OP_OS:
    cubalc_chain_os_aspects(&vm->ch);
    break;
  case OP_SHOW:
    if (rt->trace) cubalc_chain_print_cubes(&vm->ch, rt->trace);
    break;
  case OP_HALT:
    rt->halted = 1;
    break;
  default:
    break;
  }
}

int cubalc_jit_compile(const cubalc_image *img, cubalc_jit_blob *blob) {
  if (!img || !blob) return -1;
  memset(blob, 0, sizeof *blob);
#if !CUBALC_JIT_X64
  snprintf(blob->err, sizeof blob->err, "JIT only on x86_64");
  return -1;
#else
  size_t cap = 64 * 1024 + (size_t)img->n_ins * 64;
  void *mem = mmap(NULL, cap, PROT_READ | PROT_WRITE | PROT_EXEC,
                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (mem == MAP_FAILED) {
    snprintf(blob->err, sizeof blob->err, "mmap failed");
    return -1;
  }
  blob->code = mem;
  blob->code_cap = cap;
  enc_t e = { (uint8_t *)mem, (uint8_t *)mem + cap };

  /* label table for patching jumps: store offset of each instruction start */
  uint32_t *lab = calloc(img->n_ins + 1, sizeof(uint32_t));
  typedef struct { uint32_t at; int target_pc; uint8_t kind; } patch_t;
  patch_t patches[4096];
  int npatch = 0;

  emit_prologue(&e);

  for (uint32_t i = 0; i < img->n_ins; i++) {
    lab[i] = (uint32_t)(e.p - (uint8_t *)mem);
    const cubalc_ins *in = &img->code[i];
    switch (in->op) {
    case OP_HALT:
      /* mov byte [r12+halted], 1; jmp end */
      {
        uint32_t off = (uint32_t)offsetof(cubalc_jit_rt, halted);
        e1(&e, 0x41); e1(&e, 0xC6); e1(&e, 0x84); e1(&e, 0x24); e4(&e, off); e1(&e, 1);
      }
      /* jmp to epilogue — patch later with absolute near jmp using patch kind 2 = end */
      if (npatch < 4096) {
        patches[npatch].at = (uint32_t)(e.p - (uint8_t *)mem);
        patches[npatch].target_pc = -1;
        patches[npatch].kind = 2;
        npatch++;
      }
      e1(&e, 0xE9); e4(&e, 0); /* rel32 placeholder */
      break;
    case OP_NOP:
      e1(&e, 0x90);
      break;
    case OP_LOADI:
      /* mov rax, imm32 sign-ext; store reg[a] */
      e1(&e, 0x48); e1(&e, 0xC7); e1(&e, 0xC0); e4(&e, (uint32_t)in->imm);
      emit_store_reg(&e, in->a);
      break;
    case OP_MOV:
      emit_load_reg(&e, in->b);
      emit_store_reg(&e, in->a);
      break;
    case OP_ADD:
      emit_load_reg(&e, in->a);
      emit_load_reg_rdx(&e, in->b);
      e2(&e, 0x48, 0x01); e1(&e, 0xD0); /* add rax, rdx */
      emit_store_reg(&e, in->a);
      break;
    case OP_SUB:
      emit_load_reg(&e, in->a);
      emit_load_reg_rdx(&e, in->b);
      e2(&e, 0x48, 0x29); e1(&e, 0xD0);
      emit_store_reg(&e, in->a);
      break;
    case OP_MUL:
      emit_load_reg(&e, in->a);
      emit_load_reg_rdx(&e, in->b);
      e2(&e, 0x48, 0x0F); e1(&e, 0xAF); e1(&e, 0xC2); /* imul rax, rdx */
      emit_store_reg(&e, in->a);
      break;
    case OP_DIV:
      emit_load_reg(&e, in->a);
      emit_load_reg_rdx(&e, in->b);
      /* cqo; idiv rdx — careful: idiv uses rdx:rax / r/m */
      /* use: mov rbx, rdx; cqo; idiv rbx */
      e2(&e, 0x48, 0x89); e1(&e, 0xD3); /* mov rbx, rdx */
      e1(&e, 0x48); e1(&e, 0x99);       /* cqo */
      e2(&e, 0x48, 0xF7); e1(&e, 0xFB); /* idiv rbx */
      emit_store_reg(&e, in->a);
      break;
    case OP_MOD:
      emit_load_reg(&e, in->a);
      emit_load_reg_rdx(&e, in->b);
      e2(&e, 0x48, 0x89); e1(&e, 0xD3);
      e1(&e, 0x48); e1(&e, 0x99);
      e2(&e, 0x48, 0xF7); e1(&e, 0xFB);
      e2(&e, 0x48, 0x89); e1(&e, 0xD0); /* mov rax, rdx remainder */
      emit_store_reg(&e, in->a);
      break;
    case OP_NEG:
      emit_load_reg(&e, in->a);
      e2(&e, 0x48, 0xF7); e1(&e, 0xD8); /* neg rax */
      emit_store_reg(&e, in->a);
      break;
    case OP_AND:
      emit_load_reg(&e, in->a);
      emit_load_reg_rdx(&e, in->b);
      e2(&e, 0x48, 0x21); e1(&e, 0xD0);
      emit_store_reg(&e, in->a);
      break;
    case OP_OR:
      emit_load_reg(&e, in->a);
      emit_load_reg_rdx(&e, in->b);
      e2(&e, 0x48, 0x09); e1(&e, 0xD0);
      emit_store_reg(&e, in->a);
      break;
    case OP_XOR:
      emit_load_reg(&e, in->a);
      emit_load_reg_rdx(&e, in->b);
      e2(&e, 0x48, 0x31); e1(&e, 0xD0);
      emit_store_reg(&e, in->a);
      break;
    case OP_CMP: {
      emit_load_reg(&e, in->a);
      emit_load_reg_rdx(&e, in->b);
      e2(&e, 0x48, 0x39); e1(&e, 0xD0); /* cmp rax, rdx */
      /* setcc al */
      uint8_t cc = 0x94; /* sete */
      switch (in->imm) {
      case 0: cc = 0x94; break; /* e */
      case 1: cc = 0x95; break; /* ne */
      case 2: cc = 0x9C; break; /* l */
      case 3: cc = 0x9E; break; /* le */
      case 4: cc = 0x9F; break; /* g */
      case 5: cc = 0x9D; break; /* ge */
      }
      e1(&e, 0x0F); e1(&e, cc); e1(&e, 0xC0); /* setcc al */
      e2(&e, 0x48, 0x0F); e1(&e, 0xB6); e1(&e, 0xC0); /* movzx rax, al */
      emit_store_reg(&e, in->a);
      break;
    }
    case OP_JMP:
      if (npatch < 4096) {
        patches[npatch].at = (uint32_t)(e.p - (uint8_t *)mem);
        patches[npatch].target_pc = (int)i + in->imm;
        patches[npatch].kind = 0;
        npatch++;
      }
      e1(&e, 0xE9); e4(&e, 0);
      break;
    case OP_JZ:
    case OP_JNZ:
      emit_load_reg(&e, in->a);
      e2(&e, 0x48, 0x85); e1(&e, 0xC0); /* test rax,rax */
      if (npatch < 4096) {
        patches[npatch].at = (uint32_t)(e.p - (uint8_t *)mem);
        patches[npatch].target_pc = (int)i + in->imm;
        patches[npatch].kind = (uint8_t)(in->op == OP_JZ ? 1 : 3);
        npatch++;
      }
      if (in->op == OP_JZ) { e1(&e, 0x0F); e1(&e, 0x84); e4(&e, 0); } /* je */
      else { e1(&e, 0x0F); e1(&e, 0x85); e4(&e, 0); } /* jne */
      break;
    default:
      /* host call for cube ops / assert / print / decide */
      emit_call_host(&e, in->op, in->a, in->b, in->imm);
      /* if fatal: jmp end */
      {
        uint32_t off = (uint32_t)offsetof(cubalc_jit_rt, fatal);
        e1(&e, 0x41); e1(&e, 0x83); e1(&e, 0xBC); e1(&e, 0x24); e4(&e, off); e1(&e, 0); /* cmp dword [r12+fatal],0 */
        if (npatch < 4096) {
          patches[npatch].at = (uint32_t)(e.p - (uint8_t *)mem);
          patches[npatch].target_pc = -1;
          patches[npatch].kind = 4; /* jne end */
          npatch++;
        }
        e1(&e, 0x0F); e1(&e, 0x85); e4(&e, 0);
      }
      /* if halted jmp end */
      {
        uint32_t off = (uint32_t)offsetof(cubalc_jit_rt, halted);
        e1(&e, 0x41); e1(&e, 0x83); e1(&e, 0xBC); e1(&e, 0x24); e4(&e, off); e1(&e, 0);
        if (npatch < 4096) {
          patches[npatch].at = (uint32_t)(e.p - (uint8_t *)mem);
          patches[npatch].target_pc = -1;
          patches[npatch].kind = 4;
          npatch++;
        }
        e1(&e, 0x0F); e1(&e, 0x85); e4(&e, 0);
      }
      break;
    }
  }
  uint32_t end_off = (uint32_t)(e.p - (uint8_t *)mem);
  lab[img->n_ins] = end_off;
  emit_epilogue(&e);
  blob->code_len = (size_t)(e.p - (uint8_t *)mem);

  /* patch jumps */
  for (int i = 0; i < npatch; i++) {
    uint8_t *at = (uint8_t *)mem + patches[i].at;
    int32_t rel = 0;
    if (patches[i].kind == 0) { /* jmp rel32 at+1 */
      uint32_t tgt = (patches[i].target_pc < 0) ? end_off :
        (patches[i].target_pc < (int)img->n_ins ? lab[patches[i].target_pc] : end_off);
      rel = (int32_t)tgt - (int32_t)(patches[i].at + 5);
      memcpy(at + 1, &rel, 4);
    } else if (patches[i].kind == 2) { /* jmp end */
      rel = (int32_t)end_off - (int32_t)(patches[i].at + 5);
      memcpy(at + 1, &rel, 4);
    } else if (patches[i].kind == 1 || patches[i].kind == 3 || patches[i].kind == 4) {
      /* jcc rel32: opcode 0F xx rel32 — 6 bytes */
      uint32_t tgt = (patches[i].target_pc < 0) ? end_off :
        (patches[i].target_pc < (int)img->n_ins ? lab[patches[i].target_pc] : end_off);
      rel = (int32_t)tgt - (int32_t)(patches[i].at + 6);
      memcpy(at + 2, &rel, 4);
    }
  }
  free(lab);
  /* freeze exec */
  mprotect(mem, cap, PROT_READ | PROT_EXEC);
  blob->n_ins = (int)img->n_ins;
  blob->ok = 1;
  return 0;
#endif
}

void cubalc_jit_free(cubalc_jit_blob *blob) {
  if (!blob || !blob->code) return;
  munmap(blob->code, blob->code_cap);
  memset(blob, 0, sizeof *blob);
}

typedef void (*cubalc_jit_entry)(cubalc_jit_rt *rt);

int cubalc_jit_run(const cubalc_jit_blob *blob, const cubalc_image *img,
                   cubalc_run_result *out, FILE *trace) {
  if (!blob || !blob->ok || !blob->code || !img) return 2;
  cubalc_vm vm;
  memset(&vm, 0, sizeof vm);
  vm.hold_flash = 1;
  cubalc_chain_init(&vm.ch);
  vm.ch.hold_flash = 1;
  cubalc_jit_rt rt;
  memset(&rt, 0, sizeof rt);
  rt.vm = &vm;
  rt.img = img;
  rt.trace = trace;

  cubalc_jit_entry entry = (cubalc_jit_entry)blob->code;
  entry(&rt);

  if (vm.ch.n_cubes > 0) cubalc_chain_tick(&vm.ch);
  if (out) {
    memset(out, 0, sizeof *out);
    out->ok = !rt.fatal && rt.asserts_fail == 0;
    out->asserts_ok = rt.asserts_ok;
    out->asserts_fail = rt.asserts_fail;
    out->stmts = rt.stmts > 0 ? rt.stmts : (int)img->n_ins;
    out->n_cubes = vm.ch.n_cubes;
    out->unity = vm.ch.unity;
    snprintf(out->last_print, sizeof out->last_print, "%s", rt.last_print);
    if (rt.fatal) snprintf(out->err, sizeof out->err, "%s", rt.err);
  }
  /* Cube Law: united faces (LOVR + crimson GL + cells.bin) from one matrix */
  if (vm.ch.n_cubes > 0)
    cubalc_chain_publish_united(&vm.ch);
  return out && out->ok ? 0 : 1;
}

int cubalc_jit_exec(const cubalc_image *img, cubalc_run_result *out, FILE *trace) {
  cubalc_jit_blob blob;
  if (cubalc_jit_compile(img, &blob) == 0) {
    int rc = cubalc_jit_run(&blob, img, out, trace);
    cubalc_jit_free(&blob);
    return rc;
  }
  /* fallback interpreter */
  if (trace) fprintf(trace, "# jit fallback → interp (%s)\n", blob.err);
  return cubalc_isa_run(img, out, trace);
}

int cubalc_flow_manifest(const char *src_path, const char *cblc_path,
                         cubalc_run_result *out, FILE *trace) {
  FILE *f = fopen(src_path, "rb");
  if (!f) {
    if (out) { memset(out, 0, sizeof *out); snprintf(out->err, sizeof out->err, "open %s", src_path); }
    return 2;
  }
  fseek(f, 0, SEEK_END);
  long sz = ftell(f);
  fseek(f, 0, SEEK_SET);
  if (sz < 0 || sz > CUBALC_MAX_SRC) { fclose(f); return 2; }
  char *buf = malloc((size_t)sz + 1);
  if (!buf) { fclose(f); return 2; }
  size_t nr = fread(buf, 1, (size_t)sz, f);
  fclose(f);
  buf[nr] = 0;

  cubalc_image img;
  char err[160];
  if (cubalc_isa_compile_source(buf, nr, &img, err, sizeof err) != 0) {
    free(buf);
    if (out) { memset(out, 0, sizeof *out); snprintf(out->err, sizeof out->err, "compile: %s", err); }
    return 2;
  }
  free(buf);
  if (cblc_path && cblc_path[0])
    cubalc_isa_save(&img, cblc_path);
  if (trace)
    fprintf(trace, "# Cube Flow manifest: ins=%u str=%u jit=%s → %s\n",
            img.n_ins, img.n_str, cubalc_jit_backend(),
            cblc_path ? cblc_path : "(mem)");
  return cubalc_jit_exec(&img, out, trace);
}
