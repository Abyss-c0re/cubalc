# Contained hive — multi-container nanobot mesh

Three **isolated** nanobot members (`hive-a` / `hive-b` / `hive-c`) on Docker network `cube_hive_net`, shared peer token, host CubalC board as SoT.

## Run

```bash
./samples/hive_containers/run_hive.sh
# teardown
docker compose -f samples/hive_containers/docker-compose.yml down -v
```

## What is observed

| Layer | Behavior when contained |
|-------|-------------------------|
| Isolation | each member `IN_DOCKER=1`, own volume, own hostname |
| Mesh | bridge DNS `hive-a:8787` … health links |
| Auth | shared `HIVE_TOKEN` on mutate routes |
| Hive board | host CubalC SMX unites peer digits → agree |
| Viz | per-object render plan of hive board |

## Ports (host)

| Member | Port | Role |
|--------|------|------|
| a | 19001 | host |
| b | 19002 | body |
| c | 19003 | brain |

Plate: `out/HIVE_CONTAINED.json`
