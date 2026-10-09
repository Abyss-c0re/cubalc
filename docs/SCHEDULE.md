# Schedule plate

Law version `1.16.27-schedule` (`include/cubalc_law.h`, `VERSION`).

`SYS SLEEP` executes a wait. A schedule does not. `SCHED`, `BAND`, `NOTE`, `AMP`, and `SCHEDOUT` describe a bounded pulse schedule. `SCHEDOUT` writes a plate. The host scans that plate later. The language core does not name pins, plasma, Arduino, or MindStorm.

## Statements

Arguments are integer expressions on that same line. A newline ends the statement.

| Statement | Effect |
|-----------|--------|
| `SCHED name` | Open a schedule. Clear bands, notes, and AMP steps. Copy `name` (max 15 characters). Set `SCHED_NAME` and `SCHED_N=0`. A bad ident fails. |
| `BAND name sus0 sus1 duty0 duty1 gap0 gap1 twin sheet` | Append one band. `sus0<=sus1`, `duty0<=duty1`, `gap0<=gap1`. Duties are `0..255`. `twin` and `sheet` are `0..100`. Sustain and gap times are `0..60000`. Max 8 bands. Name max 15. Refused on a plate that already has `AMP`. |
| `NOTE hz ms` | Append one note. `hz` is `0..20000` (`0` is a rest). `ms` is `1..60000`. Max 16 notes. A `NOTE` that is text or a single ident stays the agent breadcrumb. Refused on a plate that already has `AMP`. |
| `AMP intensity hz ms` | Append one step. Intensity is `0..255`. `hz` is `0..255` (`0` holds that intensity). `ms` is `1..60000`. Max 16 steps. Refused on a plate that already has `BAND` or `NOTE`. |
| `SCHEDOUT` | Write `$CUBALC_STATE/cubalc_schedule.txt`, or `state/cubalc_schedule.txt` when `CUBALC_STATE` is unset. |
| `SCHEDOUT path` | Write that path. Quoted strings and idents use the usual string argument. A slash-split path is glued. The parent directory is created one level deep when it is missing. |

`BAND`, `NOTE`, and `AMP` require an open `SCHED`. On success each form sets `OK` to 1. `SCHED_N` is the number of bands plus notes plus AMP steps (`0` just after `SCHED`). `SCHEDOUT` also sets `LAST` to the path it wrote and `LAST_N` to that same count. A plate is bands and notes, or AMP steps, not both.

An empty schedule still writes `NAME` and `END`.

## Plate

LF newlines, one space between fields, no trailing space:

```
MSCHED 1
NAME <schedname>
BAND <name> <sus0> <sus1> <duty0> <duty1> <gap0> <gap1> <twin> <sheet>
NOTE <hz> <ms>
AMP <intensity> <hz> <ms>
END
```

`BAND`, `NOTE`, and `AMP` lines stay in append order. An AMP plate has no band or note lines.

## Proof

`programs/proof/schedule_storm.cubalc` writes a storm schedule and checks the plate. `programs/proof/schedule_bad_band.cubalc` fails closed when sustain is inverted. `programs/proof/schedule_amp.cubalc` checks four AMP steps. `programs/proof/schedule_bad_amp.cubalc` fails when hertz is above 255. `programs/realworld/plasma_storm.cubalc` is the band schedule. `plasma_steady.cubalc`, `plasma_slow.cubalc`, `plasma_fast.cubalc`, and `plasma_flicker.cubalc` are AMP schedules: intensity, hertz, and hold time. The numbers are the program. The core stays a plate writer.
