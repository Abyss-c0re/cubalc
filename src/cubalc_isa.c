#define _POSIX_C_SOURCE 200809L
#include "cubalc_isa.h"
#include "cubalc_async.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <math.h>

/* ---- string table ---- */
static int str_add(cubalc_image *img, const char *s) {
  if (!s) s = "";
  for (int i = 0; i < img->n_str; i++)
    if (strcmp(img->str[i], s) == 0) return i;
  if (img->n_str >= CUBALC_ISA_MAX_STRTAB) return -1;
  snprintf(img->str[img->n_str], CUBALC_ISA_MAX_STR, "%s", s);
  return img->n_str++;
}

static int emit(cubalc_image *img, uint8_t op, uint8_t a, uint8_t b, uint8_t c, int32_t imm) {
  if (img->n_ins >= CUBALC_ISA_MAX_INS) return -1;
  cubalc_ins *i = &img->code[img->n_ins++];
  i->op = op; i->a = a; i->b = b; i->c = c; i->imm = imm;
  return (int)img->n_ins - 1;
}

/* ---- simple assembler: one instruction per line ----
 * HALT / NOP / RING / OS
 * GENESIS [str]
 * SPAWN id role proton
 * PLUG a b
 * IMPULSE id proton
 * FLOW n
 * SETBIT id bit on
 * LOADI r imm
 * ADD rA rB | SUB | MUL | DIV | MOD | MOV | AND | OR | XOR
 * NEG r
 * GETSET r id | GETCUBES r | ENERGY r id
 * CMP rA op rB   op: eq ne lt le gt ge
 * JMP d | JZ r d | JNZ r d
 * PRINT str r | ASSERT r
 * HOLD n | DECIDE [id] | FLOW_P n
 */
int cubalc_isa_assemble(const char *src, size_t n, cubalc_image *img, char *err, size_t errn) {
  if (!src || !img) return -1;
  memset(img, 0, sizeof *img);
  img->magic = CUBALC_ISA_MAGIC;
  img->ver = CUBALC_ISA_VER;
  if (err && errn) err[0] = 0;

  const char *p = src;
  const char *end = src + n;
  int line = 1;

  while (p < end) {
    while (p < end && (*p == ' ' || *p == '\t' || *p == '\r')) p++;
    if (p >= end) break;
    if (*p == '\n') { p++; line++; continue; }
    if (*p == '#' || (*p == '/' && p + 1 < end && p[1] == '/')) {
      while (p < end && *p != '\n') p++;
      continue;
    }
    char op[32]; size_t oi = 0;
    while (p < end && (isalnum((unsigned char)*p) || *p == '_')) {
      if (oi + 1 < sizeof op) op[oi++] = (char)toupper((unsigned char)*p);
      p++;
    }
    op[oi] = 0;
    while (p < end && (*p == ' ' || *p == '\t')) p++;

    #define NEED() do{ if(img->n_ins>=CUBALC_ISA_MAX_INS){ if(err)snprintf(err,errn,"code full L%d",line); return -1;} }while(0)
    #define FAIL(m) do{ if(err)snprintf(err,errn,"%s L%d",m,line); return -1; }while(0)

    if (!op[0]) { while (p < end && *p != '\n') p++; continue; }

    if (strcmp(op, "HALT") == 0) { NEED(); emit(img, OP_HALT, 0, 0, 0, 0); }
    else if (strcmp(op, "NOP") == 0) { NEED(); emit(img, OP_NOP, 0, 0, 0, 0); }
    else if (strcmp(op, "RING") == 0) { NEED(); emit(img, OP_RING, 0, 0, 0, 0); }
    else if (strcmp(op, "OS") == 0) { NEED(); emit(img, OP_OS, 0, 0, 0, 0); }
    else if (strcmp(op, "GENESIS") == 0) {
      char s[CUBALC_ISA_MAX_STR] = "";
      if (*p == '"') {
        p++; size_t k = 0;
        while (p < end && *p != '"') {
          if (k + 1 < sizeof s) s[k++] = *p;
          p++;
        }
        s[k] = 0; if (p < end && *p == '"') p++;
      }
      int si = str_add(img, s[0] ? s : "NEXUS_COORD v1 | from=jit | hold_flash=1 |");
      if (si < 0) FAIL("strtab");
      NEED(); emit(img, OP_GENESIS, (uint8_t)si, 0, 0, 0);
    }
    else if (strcmp(op, "SPAWN") == 0 || strcmp(op, "CUBE") == 0) {
      char id[64] = "", role[64] = "aspect"; int proton = 1;
      if (sscanf(p, "%63s %63s %d", id, role, &proton) < 1) FAIL("SPAWN id");
      int si = str_add(img, id), ri = str_add(img, role);
      if (si < 0 || ri < 0) FAIL("strtab");
      NEED(); emit(img, OP_SPAWN, (uint8_t)si, (uint8_t)ri, 0, proton ? 1 : 0);
    }
    else if (strcmp(op, "PLUG") == 0) {
      char a[64], b[64];
      if (sscanf(p, "%63s %63s", a, b) < 2) FAIL("PLUG a b");
      int sa = str_add(img, a), sb = str_add(img, b);
      NEED(); emit(img, OP_PLUG, (uint8_t)sa, (uint8_t)sb, 0, 0);
    }
    else if (strcmp(op, "IMPULSE") == 0) {
      char id[64]; int pr = 1;
      if (sscanf(p, "%63s %d", id, &pr) < 1) FAIL("IMPULSE");
      int si = str_add(img, id);
      NEED(); emit(img, OP_IMPULSE, (uint8_t)si, 0, 0, pr ? 1 : 0);
    }
    else if (strcmp(op, "FLOW") == 0) {
      int n = 1; sscanf(p, "%d", &n);
      NEED(); emit(img, OP_FLOW, 0, 0, 0, n);
    }
    else if (strcmp(op, "FLOW_P") == 0 || strcmp(op, "PFLOW") == 0) {
      int n = 1; sscanf(p, "%d", &n);
      NEED(); emit(img, OP_FLOW_P, 0, 0, 0, n);
    }
    else if (strcmp(op, "SETBIT") == 0) {
      char id[64]; int bit = 0, on = 1;
      if (sscanf(p, "%63s %d %d", id, &bit, &on) < 2) FAIL("SETBIT");
      int si = str_add(img, id);
      NEED(); emit(img, OP_SETBIT, (uint8_t)si, (uint8_t)(bit & 255), 0, on ? 1 : 0);
    }
    else if (strcmp(op, "LOADI") == 0) {
      int r = 0; long imm = 0;
      if (sscanf(p, "r%d %ld", &r, &imm) < 2 && sscanf(p, "%d %ld", &r, &imm) < 2) FAIL("LOADI");
      NEED(); emit(img, OP_LOADI, (uint8_t)r, 0, 0, (int32_t)imm);
    }
    else if (strcmp(op, "ADD") == 0 || strcmp(op, "SUB") == 0 || strcmp(op, "MUL") == 0 ||
             strcmp(op, "DIV") == 0 || strcmp(op, "MOD") == 0 || strcmp(op, "MOV") == 0 ||
             strcmp(op, "AND") == 0 || strcmp(op, "OR") == 0 || strcmp(op, "XOR") == 0) {
      int ra = 0, rb = 0;
      if (sscanf(p, "r%d r%d", &ra, &rb) < 2 && sscanf(p, "%d %d", &ra, &rb) < 2) FAIL(op);
      uint8_t o =
        strcmp(op,"ADD")==0?OP_ADD:strcmp(op,"SUB")==0?OP_SUB:strcmp(op,"MUL")==0?OP_MUL:
        strcmp(op,"DIV")==0?OP_DIV:strcmp(op,"MOD")==0?OP_MOD:strcmp(op,"MOV")==0?OP_MOV:
        strcmp(op,"AND")==0?OP_AND:strcmp(op,"OR")==0?OP_OR:OP_XOR;
      NEED(); emit(img, o, (uint8_t)ra, (uint8_t)rb, 0, 0);
    }
    else if (strcmp(op, "NEG") == 0) {
      int r = 0; sscanf(p, "r%d", &r); if (!r) sscanf(p, "%d", &r);
      NEED(); emit(img, OP_NEG, (uint8_t)r, 0, 0, 0);
    }
    else if (strcmp(op, "GETSET") == 0 || strcmp(op, "SET") == 0) {
      int r = 0; char id[64];
      if (sscanf(p, "r%d %63s", &r, id) < 2 && sscanf(p, "%d %63s", &r, id) < 2) FAIL("GETSET");
      int si = str_add(img, id);
      NEED(); emit(img, OP_GETSET, (uint8_t)r, (uint8_t)si, 0, 0);
    }
    else if (strcmp(op, "GETCUBES") == 0 || strcmp(op, "CUBES") == 0) {
      int r = 0; sscanf(p, "r%d", &r); if (!r && isdigit((unsigned char)*p)) sscanf(p, "%d", &r);
      NEED(); emit(img, OP_GETCUBES, (uint8_t)r, 0, 0, 0);
    }
    else if (strcmp(op, "ENERGY") == 0) {
      int r = 0; char id[64];
      if (sscanf(p, "r%d %63s", &r, id) < 2 && sscanf(p, "%d %63s", &r, id) < 2) FAIL("ENERGY");
      int si = str_add(img, id);
      NEED(); emit(img, OP_ENERGY, (uint8_t)r, (uint8_t)si, 0, 0);
    }
    else if (strcmp(op, "CMP") == 0) {
      int ra = 0, rb = 0; char cop[8] = "eq";
      if (sscanf(p, "r%d %7s r%d", &ra, cop, &rb) < 3 &&
          sscanf(p, "%d %7s %d", &ra, cop, &rb) < 3) FAIL("CMP");
      int opc = 0;
      if (!strcmp(cop,"eq")||!strcmp(cop,"==")) opc=0;
      else if (!strcmp(cop,"ne")||!strcmp(cop,"!=")) opc=1;
      else if (!strcmp(cop,"lt")||!strcmp(cop,"<")) opc=2;
      else if (!strcmp(cop,"le")||!strcmp(cop,"<=")) opc=3;
      else if (!strcmp(cop,"gt")||!strcmp(cop,">")) opc=4;
      else if (!strcmp(cop,"ge")||!strcmp(cop,">=")) opc=5;
      else FAIL("CMP op");
      NEED(); emit(img, OP_CMP, (uint8_t)ra, (uint8_t)rb, 0, opc);
    }
    else if (strcmp(op, "JMP") == 0) {
      int d = 0; sscanf(p, "%d", &d);
      NEED(); emit(img, OP_JMP, 0, 0, 0, d);
    }
    else if (strcmp(op, "JZ") == 0 || strcmp(op, "JNZ") == 0) {
      int r = 0, d = 0;
      if (sscanf(p, "r%d %d", &r, &d) < 2 && sscanf(p, "%d %d", &r, &d) < 2) FAIL(op);
      NEED(); emit(img, strcmp(op,"JZ")==0?OP_JZ:OP_JNZ, (uint8_t)r, 0, 0, d);
    }
    else if (strcmp(op, "PRINT") == 0) {
      char lab[64] = ""; int r = 0;
      if (*p == '"') {
        p++; size_t k = 0;
        while (p < end && *p != '"') { if (k+1<sizeof lab) lab[k++]=*p; p++; }
        if (p < end && *p == '"') p++;
        while (p < end && (*p==' '||*p=='\t')) p++;
        sscanf(p, "r%d", &r); if (!r) sscanf(p, "%d", &r);
      } else sscanf(p, "%63s r%d", lab, &r);
      int si = str_add(img, lab);
      NEED(); emit(img, OP_PRINT, (uint8_t)si, (uint8_t)r, 0, 0);
    }
    else if (strcmp(op, "ASSERT") == 0) {
      int r = 0; sscanf(p, "r%d", &r); if (!r) sscanf(p, "%d", &r);
      NEED(); emit(img, OP_ASSERT, (uint8_t)r, 0, 0, 0);
    }
    else if (strcmp(op, "HOLD") == 0 || strcmp(op, "HOLD_FLASH") == 0) {
      int v = 1; sscanf(p, "%d", &v);
      NEED(); emit(img, OP_HOLD, 0, 0, 0, v ? 1 : 0);
    }
    else if (strcmp(op, "DECIDE") == 0) {
      char id[64] = "";
      sscanf(p, "%63s", id);
      int si = id[0] ? str_add(img, id) : 0;
      NEED(); emit(img, OP_DECIDE, (uint8_t)si, 0, 0, id[0] ? 1 : 0);
    }
    else if (strcmp(op, "SHOW") == 0) {
      NEED(); emit(img, OP_SHOW, 0, 0, 0, 0);
    }
    else FAIL("unknown op");

    while (p < end && *p != '\n') p++;
    if (p < end && *p == '\n') { p++; line++; }
  }
  if (img->n_ins == 0 || img->code[img->n_ins - 1].op != OP_HALT)
    emit(img, OP_HALT, 0, 0, 0, 0);
  return 0;
}

/* ---- save / load ---- */
int cubalc_isa_save(const cubalc_image *img, const char *path) {
  if (!img || !path) return -1;
  FILE *f = fopen(path, "wb");
  if (!f) return -1;
  fwrite(img, 1, sizeof *img, f);
  fclose(f);
  return 0;
}

int cubalc_isa_load(cubalc_image *img, const char *path) {
  if (!img || !path) return -1;
  FILE *f = fopen(path, "rb");
  if (!f) return -1;
  size_t n = fread(img, 1, sizeof *img, f);
  fclose(f);
  if (n < sizeof(uint32_t) || img->magic != CUBALC_ISA_MAGIC) return -1;
  if (img->n_ins > CUBALC_ISA_MAX_INS) img->n_ins = CUBALC_ISA_MAX_INS;
  return 0;
}

int cubalc_isa_disasm(const cubalc_image *img, FILE *out) {
  if (!img || !out) return -1;
  fprintf(out, "; CBLC ver=%u str=%u ins=%u\n", img->ver, img->n_str, img->n_ins);
  for (uint32_t i = 0; i < img->n_ins; i++) {
    const cubalc_ins *x = &img->code[i];
    fprintf(out, "%04u  op=%02x a=%u b=%u c=%u imm=%d\n",
            i, x->op, x->a, x->b, x->c, (int)x->imm);
  }
  return 0;
}

/* ---- interpreter ---- */
static int find_cube_vm(cubalc_vm *vm, const char *id) {
  for (int i = 0; i < vm->ch.n_cubes; i++)
    if (strcmp(vm->ch.cubes[i].id, id) == 0) return i;
  return -1;
}

static const char *S(const cubalc_image *img, int i) {
  if (i < 0 || i >= img->n_str) return "";
  return img->str[i];
}

int cubalc_isa_run(const cubalc_image *img, cubalc_run_result *out, FILE *trace) {
  if (!img) return 2;
  cubalc_vm vm;
  memset(&vm, 0, sizeof vm);
  vm.res = out;
  vm.trace = trace;
  vm.hold_flash = 1;
  cubalc_chain_init(&vm.ch);
  vm.ch.hold_flash = 1;
  if (out) { memset(out, 0, sizeof *out); out->ok = 1; }

  for (vm.pc = 0; !vm.halted && !vm.fatal && vm.pc >= 0 && (uint32_t)vm.pc < img->n_ins; ) {
    const cubalc_ins *in = &img->code[vm.pc];
    int next = vm.pc + 1;
    switch (in->op) {
    case OP_HALT: vm.halted = 1; break;
    case OP_NOP: break;
    case OP_HOLD:
      vm.hold_flash = in->imm ? 1 : 0;
      vm.ch.hold_flash = (uint8_t)vm.hold_flash;
      break;
    case OP_GENESIS: {
      cubalc_matrix gen;
      const char *plate = S(img, in->a);
      if (!plate[0]) plate = "NEXUS_COORD v1 | from=jit | hold_flash=1 |";
      cubalc_coord_to_matrix(plate, &gen);
      cubalc_chain_from_initial(&vm.ch, &gen, 1);
      vm.ch.hold_flash = (uint8_t)vm.hold_flash;
      break;
    }
    case OP_SPAWN: {
      if (vm.ch.n_cubes == 0) {
        cubalc_matrix gen;
        cubalc_coord_to_matrix("NEXUS_COORD v1 | from=jit | hold_flash=1 |", &gen);
        cubalc_chain_from_initial(&vm.ch, &gen, 1);
      }
      float x = (float)(vm.ch.n_cubes % 5) * 0.28f;
      float z = (float)(vm.ch.n_cubes / 5) * 0.28f;
      cubalc_cube_spawn(&vm.ch, S(img, in->a), S(img, in->b),
                        (uint8_t)(in->imm ? 1 : 0), x, 0.f, z);
      break;
    }
    case OP_PLUG: {
      int ia = find_cube_vm(&vm, S(img, in->a));
      int ib = find_cube_vm(&vm, S(img, in->b));
      if (ia >= 0 && ib >= 0) cubalc_cube_plug(&vm.ch, ia, ib);
      break;
    }
    case OP_RING:
      for (int i = 0; i < vm.ch.n_cubes; i++)
        cubalc_cube_plug(&vm.ch, i, (i + 1) % vm.ch.n_cubes);
      break;
    case OP_OS:
      cubalc_chain_os_aspects(&vm.ch);
      break;
    case OP_IMPULSE:
      cubalc_chain_impulse(&vm.ch, S(img, in->a), (uint8_t)(in->imm ? 1 : 0));
      break;
    case OP_FLOW:
      for (int t = 0; t < in->imm; t++) cubalc_chain_flow(&vm.ch);
      break;
    case OP_FLOW_P:
      cubalc_async_chain_flow(&vm.ch, in->imm > 0 ? in->imm : 1);
      break;
    case OP_SETBIT: {
      int ix = find_cube_vm(&vm, S(img, in->a));
      if (ix >= 0) {
        cubalc_matrix_set(&vm.ch.cubes[ix].atom.matrix, in->b, in->imm ? 1 : 0);
        vm.ch.cubes[ix].atom.digit =
          (uint8_t)cubalc_algocube_digit(&vm.ch.cubes[ix].atom.matrix);
      }
      break;
    }
    case OP_LOADI:
      if (in->a < CUBALC_ISA_MAX_REG) vm.reg[in->a] = in->imm;
      break;
    case OP_ADD:
      if (in->a < CUBALC_ISA_MAX_REG && in->b < CUBALC_ISA_MAX_REG)
        vm.reg[in->a] += vm.reg[in->b];
      break;
    case OP_SUB:
      if (in->a < CUBALC_ISA_MAX_REG && in->b < CUBALC_ISA_MAX_REG)
        vm.reg[in->a] -= vm.reg[in->b];
      break;
    case OP_MUL:
      if (in->a < CUBALC_ISA_MAX_REG && in->b < CUBALC_ISA_MAX_REG)
        vm.reg[in->a] *= vm.reg[in->b];
      break;
    case OP_DIV:
      if (in->a < CUBALC_ISA_MAX_REG && in->b < CUBALC_ISA_MAX_REG && vm.reg[in->b])
        vm.reg[in->a] /= vm.reg[in->b];
      break;
    case OP_MOD:
      if (in->a < CUBALC_ISA_MAX_REG && in->b < CUBALC_ISA_MAX_REG && vm.reg[in->b])
        vm.reg[in->a] %= vm.reg[in->b];
      break;
    case OP_MOV:
      if (in->a < CUBALC_ISA_MAX_REG && in->b < CUBALC_ISA_MAX_REG)
        vm.reg[in->a] = vm.reg[in->b];
      break;
    case OP_NEG:
      if (in->a < CUBALC_ISA_MAX_REG) vm.reg[in->a] = -vm.reg[in->a];
      break;
    case OP_AND:
      if (in->a < CUBALC_ISA_MAX_REG && in->b < CUBALC_ISA_MAX_REG)
        vm.reg[in->a] &= vm.reg[in->b];
      break;
    case OP_OR:
      if (in->a < CUBALC_ISA_MAX_REG && in->b < CUBALC_ISA_MAX_REG)
        vm.reg[in->a] |= vm.reg[in->b];
      break;
    case OP_XOR:
      if (in->a < CUBALC_ISA_MAX_REG && in->b < CUBALC_ISA_MAX_REG)
        vm.reg[in->a] ^= vm.reg[in->b];
      break;
    case OP_GETSET: {
      int ix = find_cube_vm(&vm, S(img, in->b));
      if (in->a < CUBALC_ISA_MAX_REG)
        vm.reg[in->a] = ix >= 0 ? cubalc_matrix_popcount(&vm.ch.cubes[ix].atom.matrix) : 0;
      break;
    }
    case OP_GETCUBES:
      if (in->a < CUBALC_ISA_MAX_REG) vm.reg[in->a] = vm.ch.n_cubes;
      break;
    case OP_ENERGY: {
      int ix = find_cube_vm(&vm, S(img, in->b));
      if (in->a < CUBALC_ISA_MAX_REG)
        vm.reg[in->a] = ix >= 0 ? (int64_t)lround(vm.ch.cubes[ix].atom.energy * 100.0) : 0;
      break;
    }
    case OP_CMP: {
      if (in->a >= CUBALC_ISA_MAX_REG || in->b >= CUBALC_ISA_MAX_REG) break;
      int64_t A = vm.reg[in->a], B = vm.reg[in->b], r = 0;
      switch (in->imm) {
      case 0: r = A == B; break;
      case 1: r = A != B; break;
      case 2: r = A < B; break;
      case 3: r = A <= B; break;
      case 4: r = A > B; break;
      case 5: r = A >= B; break;
      }
      vm.reg[in->a] = r;
      break;
    }
    case OP_JMP: next = vm.pc + in->imm; break;
    case OP_JZ:
      if (in->a < CUBALC_ISA_MAX_REG && vm.reg[in->a] == 0) next = vm.pc + in->imm;
      break;
    case OP_JNZ:
      if (in->a < CUBALC_ISA_MAX_REG && vm.reg[in->a] != 0) next = vm.pc + in->imm;
      break;
    case OP_PRINT: {
      char line[256];
      snprintf(line, sizeof line, "%s %lld", S(img, in->a),
               (long long)(in->b < CUBALC_ISA_MAX_REG ? vm.reg[in->b] : 0));
      if (trace) fprintf(trace, "%s\n", line);
      if (out) snprintf(out->last_print, sizeof out->last_print, "%s", line);
      if (out) out->stmts++;
      break;
    }
    case OP_ASSERT:
      if (in->a < CUBALC_ISA_MAX_REG && vm.reg[in->a]) {
        if (out) out->asserts_ok++;
        if (trace) fprintf(trace, "# ASSERT ok\n");
      } else {
        if (out) out->asserts_fail++;
        vm.fatal = 1;
        snprintf(vm.err, sizeof vm.err, "ASSERT failed at pc=%d", vm.pc);
        if (out) { out->ok = 0; snprintf(out->err, sizeof out->err, "%s", vm.err); }
      }
      break;
    case OP_DECIDE: {
      int ix = -1;
      if (in->imm && in->a < img->n_str) ix = find_cube_vm(&vm, S(img, in->a));
      if (ix < 0) {
        for (int i = 0; i < vm.ch.n_cubes; i++)
          if (strstr(vm.ch.cubes[i].role, "brain") || strstr(vm.ch.cubes[i].id, "brain")) {
            ix = i; break;
          }
        if (ix < 0 && vm.ch.n_cubes > 0) ix = 0;
      }
      if (ix >= 0) {
        vm.ch.cubes[ix].atom.digit =
          (uint8_t)cubalc_algocube_digit(&vm.ch.cubes[ix].atom.matrix);
        vm.reg[0] = vm.ch.cubes[ix].atom.digit;
      } else vm.reg[0] = 4;
      break;
    }
    case OP_SHOW:
      if (trace) cubalc_chain_print_cubes(&vm.ch, trace);
      break;
    default:
      break;
    }
    if (out) out->stmts++;
    vm.pc = next;
  }
  if (vm.ch.n_cubes > 0) cubalc_chain_tick(&vm.ch);
  if (out) {
    out->ok = !vm.fatal && out->asserts_fail == 0;
    out->n_cubes = vm.ch.n_cubes;
    out->unity = vm.ch.unity;
    if (vm.fatal && !out->err[0]) snprintf(out->err, sizeof out->err, "%s", vm.err);
  }
  return out && out->ok ? 0 : 1;
}

/* ---- high-level lowerer: LET/LOOP/IF/ASSERT/PRINT/CUBE/... → image ----
 * Enough for proof suite + Cube Flow self-manifest.
 */
typedef struct {
  char name[32];
  int reg;
} name_reg;

static int nr_get(name_reg *nr, int *nn, const char *name, int create) {
  for (int i = 0; i < *nn; i++)
    if (strcmp(nr[i].name, name) == 0) return nr[i].reg;
  if (!create || *nn >= 28) return -1;
  snprintf(nr[*nn].name, sizeof nr[0].name, "%s", name);
  /* r0-r1 temps · r2..r29 vars · r30-r31 expr scratch */
  nr[*nn].reg = *nn + 2;
  if (nr[*nn].reg >= 30) return -1;
  return nr[(*nn)++].reg;
}

/* Very small expression compiler → result in r0
 * supports: numbers, names, + - * / % ( ) == != < <= > >=
 */
static const char *skip_sp(const char *p) {
  while (*p == ' ' || *p == '\t') p++;
  return p;
}

static const char *compile_expr(cubalc_image *img, name_reg *nr, int *nn,
                                const char *p, int dst, char *err, size_t errn);

static const char *compile_prim(cubalc_image *img, name_reg *nr, int *nn,
                                const char *p, int dst, char *err, size_t errn) {
  p = skip_sp(p);
  if (*p == '(') {
    p = compile_expr(img, nr, nn, p + 1, dst, err, errn);
    if (!p) return NULL;
    p = skip_sp(p);
    if (*p == ')') p++;
    return p;
  }
  if (*p == '-') {
    p = compile_prim(img, nr, nn, p + 1, dst, err, errn);
    if (!p) return NULL;
    emit(img, OP_NEG, (uint8_t)dst, 0, 0, 0);
    return p;
  }
  if (isdigit((unsigned char)*p)) {
    long v = strtol(p, (char **)&p, 10);
    emit(img, OP_LOADI, (uint8_t)dst, 0, 0, (int32_t)v);
    return p;
  }
  /* SET(id) ENERGY(id) CUBES */
  if (!strncmp(p, "SET(", 4) || !strncmp(p, "POPCOUNT(", 9)) {
    int off = p[0] == 'S' ? 4 : 9;
    p += off;
    char id[64]; size_t k = 0;
    while (*p && *p != ')' && k + 1 < sizeof id) id[k++] = *p++;
    id[k] = 0;
    if (*p == ')') p++;
    int si = str_add(img, id);
    emit(img, OP_GETSET, (uint8_t)dst, (uint8_t)si, 0, 0);
    return p;
  }
  if (!strncmp(p, "ENERGY(", 7)) {
    p += 7;
    char id[64]; size_t k = 0;
    while (*p && *p != ')' && k + 1 < sizeof id) id[k++] = *p++;
    id[k] = 0;
    if (*p == ')') p++;
    int si = str_add(img, id);
    emit(img, OP_ENERGY, (uint8_t)dst, (uint8_t)si, 0, 0);
    return p;
  }
  if (!strncmp(p, "CUBES", 5) && !isalnum((unsigned char)p[5])) {
    emit(img, OP_GETCUBES, (uint8_t)dst, 0, 0, 0);
    return p + 5;
  }
  if (isalpha((unsigned char)*p) || *p == '_') {
    char name[32]; size_t k = 0;
    while ((isalnum((unsigned char)*p) || *p == '_') && k + 1 < sizeof name)
      name[k++] = *p++;
    name[k] = 0;
    int r = nr_get(nr, nn, name, 1);
    if (r < 0) { if (err) snprintf(err, errn, "reg full"); return NULL; }
    emit(img, OP_MOV, (uint8_t)dst, (uint8_t)r, 0, 0);
    return p;
  }
  if (err) snprintf(err, errn, "bad primary");
  return NULL;
}

/* r30/r31 reserved expression scratch — never allocate user vars there */
#define R_SCR 30
#define R_SCR2 31

static const char *compile_term(cubalc_image *img, name_reg *nr, int *nn,
                                const char *p, int dst, char *err, size_t errn) {
  p = compile_prim(img, nr, nn, p, dst, err, errn);
  if (!p) return NULL;
  for (;;) {
    p = skip_sp(p);
    uint8_t op = 0;
    if (*p == '*') op = OP_MUL;
    else if (*p == '/') op = OP_DIV;
    else if (*p == '%') op = OP_MOD;
    else break;
    p++;
    /* RHS scratch must not equal dst */
    int rhs = (dst == R_SCR2) ? R_SCR : R_SCR2;
    p = compile_prim(img, nr, nn, p, rhs, err, errn);
    if (!p) return NULL;
    emit(img, op, (uint8_t)dst, (uint8_t)rhs, 0, 0);
  }
  return p;
}

static const char *compile_add(cubalc_image *img, name_reg *nr, int *nn,
                               const char *p, int dst, char *err, size_t errn) {
  p = compile_term(img, nr, nn, p, dst, err, errn);
  if (!p) return NULL;
  for (;;) {
    p = skip_sp(p);
    uint8_t op = 0;
    if (*p == '+') op = OP_ADD;
    else if (*p == '-') op = OP_SUB;
    else break;
    p++;
    int rhs = (dst == R_SCR2) ? R_SCR : R_SCR2;
    p = compile_term(img, nr, nn, p, rhs, err, errn);
    if (!p) return NULL;
    emit(img, op, (uint8_t)dst, (uint8_t)rhs, 0, 0);
  }
  return p;
}

static const char *compile_expr(cubalc_image *img, name_reg *nr, int *nn,
                                const char *p, int dst, char *err, size_t errn) {
  p = compile_add(img, nr, nn, p, dst, err, errn);
  if (!p) return NULL;
  p = skip_sp(p);
  int opc = -1;
  if (p[0] == '=' && p[1] == '=') { opc = 0; p += 2; }
  else if (p[0] == '!' && p[1] == '=') { opc = 1; p += 2; }
  else if (p[0] == '<' && p[1] == '=') { opc = 3; p += 2; }
  else if (p[0] == '>' && p[1] == '=') { opc = 5; p += 2; }
  else if (p[0] == '<') { opc = 2; p += 1; }
  else if (p[0] == '>') { opc = 4; p += 1; }
  if (opc >= 0) {
    int rhs = (dst == R_SCR2) ? R_SCR : R_SCR2;
    p = compile_add(img, nr, nn, p, rhs, err, errn);
    if (!p) return NULL;
    emit(img, OP_CMP, (uint8_t)dst, (uint8_t)rhs, 0, opc);
  }
  return p;
}

int cubalc_isa_compile_source(const char *src, size_t n, cubalc_image *img,
                              char *err, size_t errn) {
  if (!src || !img) return -1;
  /* if looks like assembly (LOADI/HALT/FLOW_P), use assembler */
  if (strstr(src, "LOADI") || strstr(src, "\nHALT") || strstr(src, "JMP "))
    return cubalc_isa_assemble(src, n, img, err, errn);

  memset(img, 0, sizeof *img);
  img->magic = CUBALC_ISA_MAGIC;
  img->ver = CUBALC_ISA_VER;
  if (err && errn) err[0] = 0;

  name_reg nr[32];
  int nn = 0;
  const char *p = src;
  const char *end = src + n;
  int line = 1;

  /* default hold */
  emit(img, OP_HOLD, 0, 0, 0, 1);

  while (p < end) {
    while (p < end && (*p == ' ' || *p == '\t' || *p == '\r')) p++;
    if (p >= end) break;
    if (*p == '\n') { p++; line++; continue; }
    if (*p == '#' || (*p == '/' && p[1] == '/')) {
      while (p < end && *p != '\n') p++;
      continue;
    }

    /* keywords */
    char kw[32]; size_t ki = 0;
    const char *save = p;
    while (p < end && (isalnum((unsigned char)*p) || *p == '_')) {
      if (ki + 1 < sizeof kw) kw[ki++] = *p;
      p++;
    }
    kw[ki] = 0;

    #define SKIP_LINE() do{ while(p<end && *p!='\n') p++; }while(0)

    if (!strcasecmp(kw, "CREED") || !strcasecmp(kw, "SHARE") || !strcasecmp(kw, "BUDGET")) {
      SKIP_LINE(); continue;
    }
    if (!strcasecmp(kw, "HOLD_FLASH") || !strcasecmp(kw, "HOLD")) {
      int v = 1; sscanf(p, "%d", &v);
      emit(img, OP_HOLD, 0, 0, 0, v ? 1 : 0);
      SKIP_LINE(); continue;
    }
    if (!strcasecmp(kw, "GENESIS")) {
      char plate[200] = "NEXUS_COORD v1 | from=compile | hold_flash=1 |";
      p = skip_sp(p);
      if (*p == '"') {
        p++; size_t k = 0;
        while (p < end && *p != '"' && k + 1 < sizeof plate) plate[k++] = *p++;
        plate[k] = 0;
      }
      int si = str_add(img, plate);
      emit(img, OP_GENESIS, (uint8_t)si, 0, 0, 0);
      SKIP_LINE(); continue;
    }
    if (!strcasecmp(kw, "CUBE")) {
      char id[64] = "", role[64] = "aspect"; int proton = 1;
      sscanf(p, "%63s", id);
      const char *q = p;
      while (*q && strncmp(q, "ROLE", 4)) q++;
      if (!strncmp(q, "ROLE", 4)) sscanf(q + 4, " %63s", role);
      q = p;
      while (*q && strncmp(q, "PROTON", 6)) q++;
      if (!strncmp(q, "PROTON", 6)) sscanf(q + 6, " %d", &proton);
      int si = str_add(img, id), ri = str_add(img, role);
      emit(img, OP_SPAWN, (uint8_t)si, (uint8_t)ri, 0, proton ? 1 : 0);
      SKIP_LINE(); continue;
    }
    if (!strcasecmp(kw, "PLUG")) {
      p = skip_sp(p);
      if (!strncasecmp(p, "RING", 4)) emit(img, OP_RING, 0, 0, 0, 0);
      else {
        char a[64], b[64];
        if (sscanf(p, "%63s %63s", a, b) >= 2) {
          int sa = str_add(img, a), sb = str_add(img, b);
          emit(img, OP_PLUG, (uint8_t)sa, (uint8_t)sb, 0, 0);
        }
      }
      SKIP_LINE(); continue;
    }
    if (!strcasecmp(kw, "IMPULSE")) {
      char id[64]; int pr = 1;
      sscanf(p, "%63s %d", id, &pr);
      int si = str_add(img, id);
      emit(img, OP_IMPULSE, (uint8_t)si, 0, 0, pr ? 1 : 0);
      SKIP_LINE(); continue;
    }
    if (!strcasecmp(kw, "FLOW") || !strcasecmp(kw, "TICK")) {
      int n = 8; sscanf(p, "%d", &n);
      emit(img, OP_FLOW_P, 0, 0, 0, n);
      SKIP_LINE(); continue;
    }
    if (!strcasecmp(kw, "SETBIT")) {
      char id[64]; int bit = 0, on = 1;
      sscanf(p, "%63s %d %d", id, &bit, &on);
      int si = str_add(img, id);
      emit(img, OP_SETBIT, (uint8_t)si, (uint8_t)bit, 0, on ? 1 : 0);
      SKIP_LINE(); continue;
    }
    if (!strcasecmp(kw, "DECIDE")) {
      char id[64] = "";
      sscanf(p, "%63s", id);
      int si = id[0] ? str_add(img, id) : 0;
      emit(img, OP_DECIDE, (uint8_t)si, 0, 0, id[0] ? 1 : 0);
      SKIP_LINE(); continue;
    }
    if (!strcasecmp(kw, "LET")) {
      char name[32];
      p = skip_sp(p);
      size_t k = 0;
      while ((isalnum((unsigned char)*p) || *p == '_') && k + 1 < sizeof name)
        name[k++] = *p++;
      name[k] = 0;
      p = skip_sp(p);
      if (*p == '=') p++;
      int r = nr_get(nr, &nn, name, 1);
      if (r < 0) { if (err) snprintf(err, errn, "regs L%d", line); return -1; }
      p = compile_expr(img, nr, &nn, p, r, err, errn);
      if (!p) return -1;
      SKIP_LINE(); continue;
    }
    if (!strcasecmp(kw, "PRINT")) {
      char lab[64] = "print";
      p = skip_sp(p);
      if (*p == '"') {
        p++; size_t k = 0;
        while (p < end && *p != '"' && k + 1 < sizeof lab) lab[k++] = *p++;
        lab[k] = 0;
        if (*p == '"') p++;
      }
      p = skip_sp(p);
      /* optional expr */
      if (*p && *p != '\n') {
        p = compile_expr(img, nr, &nn, p, 0, err, errn);
        if (!p) return -1;
      } else emit(img, OP_LOADI, 0, 0, 0, 0);
      int si = str_add(img, lab);
      emit(img, OP_PRINT, (uint8_t)si, 0, 0, 0);
      SKIP_LINE(); continue;
    }
    if (!strcasecmp(kw, "ASSERT")) {
      p = compile_expr(img, nr, &nn, p, 0, err, errn);
      if (!p) return -1;
      emit(img, OP_ASSERT, 0, 0, 0, 0);
      SKIP_LINE(); continue;
    }
    if (!strcasecmp(kw, "LOOP")) {
      /* LOOP n ... END  — uses IT in r_it */
      p = compile_expr(img, nr, &nn, p, 0, err, errn); /* count in r0 */
      if (!p) return -1;
      int r_cnt = 29; /* reserved loop count — not expr scratch */
      emit(img, OP_MOV, (uint8_t)r_cnt, 0, 0, 0);
      int r_it = nr_get(nr, &nn, "IT", 1);
      if (r_it < 0) { if (err) snprintf(err, errn, "regs"); return -1; }
      emit(img, OP_LOADI, (uint8_t)r_it, 0, 0, 0);
      int loop_top = (int)img->n_ins;
      /* if IT >= count → jump end (use r0 only as cmp dest) */
      emit(img, OP_MOV, 0, (uint8_t)r_it, 0, 0);
      emit(img, OP_CMP, 0, (uint8_t)r_cnt, 0, 5); /* IT >= count → r0 */
      int j_exit = emit(img, OP_JNZ, 0, 0, 0, 0); /* patch later */

      /* body until END at depth 1 */
      int depth = 1;
      while (p < end && depth > 0) {
        while (p < end && (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')) {
          if (*p == '\n') line++;
          p++;
        }
        if (p >= end) break;
        if (*p == '#' || (*p == '/' && p[1] == '/')) {
          while (p < end && *p != '\n') p++;
          continue;
        }
        const char *wp = p;
        char w[16]; size_t wi = 0;
        while (p < end && (isalpha((unsigned char)*p)) && wi + 1 < sizeof w)
          w[wi++] = *p++;
        w[wi] = 0;
        if (!strcasecmp(w, "LOOP") || !strcasecmp(w, "IF")) depth++;
        else if (!strcasecmp(w, "END")) {
          depth--;
          if (depth == 0) break;
        }
        p = wp;
        /* recursively handle one statement by reusing outer loop logic is hard —
           fall back: only allow LET/ASSERT/PRINT/IMPULSE/FLOW/SETBIT inside via mini parse */
        if (!strcasecmp(w, "LET")) {
          p += 3;
          char name[32]; p = skip_sp(p); size_t k = 0;
          while ((isalnum((unsigned char)*p)||*p=='_') && k+1<sizeof name) name[k++]=*p++;
          name[k]=0; p=skip_sp(p); if(*p=='=')p++;
          int r = nr_get(nr, &nn, name, 1);
          p = compile_expr(img, nr, &nn, p, r, err, errn);
          if (!p) return -1;
          SKIP_LINE();
        } else if (!strcasecmp(w, "ASSERT")) {
          p += 6;
          p = compile_expr(img, nr, &nn, p, 0, err, errn);
          if (!p) return -1;
          emit(img, OP_ASSERT, 0, 0, 0, 0);
          SKIP_LINE();
        } else if (!strcasecmp(w, "PRINT")) {
          p += 5; p = skip_sp(p);
          char lab[64]="";
          if (*p=='"'){ p++; size_t k=0; while(p<end&&*p!='"'&&k+1<sizeof lab)lab[k++]=*p++; if(*p=='"')p++; }
          p = skip_sp(p);
          if (*p && *p!='\n') p = compile_expr(img, nr, &nn, p, 0, err, errn);
          else emit(img, OP_LOADI, 0, 0, 0, 0);
          if (!p) return -1;
          int si = str_add(img, lab);
          emit(img, OP_PRINT, (uint8_t)si, 0, 0, 0);
          SKIP_LINE();
        } else if (!strcasecmp(w, "IMPULSE")) {
          p += 7; char id[64]; int pr=1; sscanf(p, "%63s %d", id, &pr);
          int si = str_add(img, id);
          emit(img, OP_IMPULSE, (uint8_t)si, 0, 0, pr?1:0);
          SKIP_LINE();
        } else if (!strcasecmp(w, "FLOW") || !strcasecmp(w, "TICK")) {
          p += strlen(w); int n=1; sscanf(p, "%d", &n);
          emit(img, OP_FLOW_P, 0, 0, 0, n);
          SKIP_LINE();
        } else if (!strcasecmp(w, "SETBIT")) {
          p += 6; char id[64]; int bit=0,on=1; sscanf(p, "%63s %d %d", id,&bit,&on);
          int si = str_add(img, id);
          emit(img, OP_SETBIT, (uint8_t)si, (uint8_t)bit, 0, on?1:0);
          SKIP_LINE();
        } else if (!strcasecmp(w, "END") || !strcasecmp(w, "ELSE") || !strcasecmp(w, "THEN")) {
          SKIP_LINE();
        } else {
          /* skip unknown inside loop */
          SKIP_LINE();
        }
      }
      /* IT++ using r0 temp only */
      emit(img, OP_LOADI, 0, 0, 0, 1);
      emit(img, OP_ADD, (uint8_t)r_it, 0, 0, 0);
      /* jump back */
      int here = (int)img->n_ins;
      emit(img, OP_JMP, 0, 0, 0, loop_top - here);
      /* patch exit */
      img->code[j_exit].imm = (int)img->n_ins - j_exit;
      continue;
    }
    if (!strcasecmp(kw, "IF")) {
      p = compile_expr(img, nr, &nn, p, 0, err, errn);
      if (!p) return -1;
      p = skip_sp(p);
      if (!strncasecmp(p, "THEN", 4)) p += 4;
      int j_else = emit(img, OP_JZ, 0, 0, 0, 0); /* if false skip then */
      /* then body until ELSE or END */
      int depth = 1; int saw_else = 0; int j_end = -1;
      while (p < end && depth > 0) {
        while (p < end && (*p==' '||*p=='\t'||*p=='\r'||*p=='\n')) { if(*p=='\n')line++; p++; }
        if (p >= end) break;
        if (*p=='#'||(*p=='/'&&p[1]=='/')) { while(p<end&&*p!='\n')p++; continue; }
        const char *wp = p;
        char w[16]; size_t wi=0;
        while (p<end && isalpha((unsigned char)*p) && wi+1<sizeof w) w[wi++]=*p++;
        w[wi]=0;
        if (!strcasecmp(w,"IF")) depth++;
        else if (!strcasecmp(w,"END")) {
          depth--;
          if (depth==0) break;
        } else if (!strcasecmp(w,"ELSE") && depth==1) {
          j_end = emit(img, OP_JMP, 0, 0, 0, 0);
          img->code[j_else].imm = (int)img->n_ins - j_else;
          saw_else = 1;
          SKIP_LINE();
          continue;
        }
        p = wp;
        if (!strcasecmp(w, "LET")) {
          p += 3; char name[32]; p=skip_sp(p); size_t k=0;
          while((isalnum((unsigned char)*p)||*p=='_')&&k+1<sizeof name) name[k++]=*p++;
          name[k]=0; p=skip_sp(p); if(*p=='=')p++;
          int r = nr_get(nr,&nn,name,1);
          p = compile_expr(img,nr,&nn,p,r,err,errn); if(!p)return -1;
          SKIP_LINE();
        } else if (!strcasecmp(w,"ASSERT")) {
          p+=6; p=compile_expr(img,nr,&nn,p,0,err,errn); if(!p)return -1;
          emit(img,OP_ASSERT,0,0,0,0); SKIP_LINE();
        } else if (!strcasecmp(w,"PRINT")) {
          p+=5; p=skip_sp(p); char lab[64]="";
          if(*p=='"'){p++;size_t k=0;while(p<end&&*p!='"'&&k+1<sizeof lab)lab[k++]=*p++;if(*p=='"')p++;}
          p=skip_sp(p);
          if(*p && *p!='\n') p=compile_expr(img,nr,&nn,p,0,err,errn);
          else emit(img,OP_LOADI,0,0,0,0);
          if(!p)return -1;
          emit(img,OP_PRINT,(uint8_t)str_add(img,lab),0,0,0); SKIP_LINE();
        } else if (!strcasecmp(w,"IMPULSE")) {
          p+=7; char id[64]; int pr=1; sscanf(p,"%63s %d",id,&pr);
          emit(img,OP_IMPULSE,(uint8_t)str_add(img,id),0,0,pr?1:0); SKIP_LINE();
        } else { SKIP_LINE(); }
      }
      if (!saw_else)
        img->code[j_else].imm = (int)img->n_ins - j_else;
      if (j_end >= 0)
        img->code[j_end].imm = (int)img->n_ins - j_end;
      continue;
    }
    if (!strcasecmp(kw, "END") || !strcasecmp(kw, "ELSE") || !strcasecmp(kw, "THEN") ||
        !strcasecmp(kw, "OS_ASPECTS") || !strcasecmp(kw, "VIZ") || !strcasecmp(kw, "SPIN") ||
        !strcasecmp(kw, "WAIT") || !strcasecmp(kw, "SHOW")) {
      SKIP_LINE(); continue;
    }
    /* play dialect lines starting with [ — skip for compile path */
    if (kw[0] == 0 && *save == '[') { SKIP_LINE(); continue; }
    /* unknown: skip */
    p = save;
    SKIP_LINE();
  }
  emit(img, OP_HALT, 0, 0, 0, 0);
  return 0;
}
