# leekwars-generator-c

A pure-C re-implementation of the [Leek Wars](https://leekwars.com) fight
engine, ported line-by-line from the official
[Java engine](https://github.com/leek-wars/leek-wars-generator) (via
[leekwars-generator-python](https://github.com/Traual/leekwars-generator-python)).

## The three engines

| Engine | Source | Speed | Use case |
|---|---|---|---|
| **Java** | upstream reference | baseline | production server (leekwars.com) |
| **Python** | line-by-line port of Java | ~250 fights/sec | tooling, debugging, ground truth |
| **C** (this repo) | line-by-line port of Java | ~50 000 fights/sec | AI training, self-play, MCTS |

The C port is based on an older Python port of the Java engine. Work toward
parity with a pinned newer Java version is tracked in [MECHANICS.md](MECHANICS.md);
current Java parity is not established.

## Parity status

Historical results reported against the older Python engine:

| Test | Coverage | Result |
|---|---|---|
| `test_v2_parity.py` | 36 weapons × 20 seeds | **720 / 720** |
| `test_v2_mass_chips_all.py` | 109 chips × 20 seeds | **2 180 / 2 180** |
| `test_v2_combos_simple.py` | 50 (weapon + 2 chips) × 5 seeds | **250 / 250** |
| `test_v2_combos.py` | 4 weapons × 6 dmg chips × 10 summons × 5 seeds | **1 200 / 1 200** |
| `test_v2_summon.py` | 10 bulb templates × 5 seeds | **50 / 50** |
| `test_v2_death.py` | 7 weapons × 9 HP levels × 10 seeds | **630 / 630** |
| `test_v2_fuzzer.py` | random loadouts | **1 000 / 1 000** |
| `test_v2_team_real.py` (NvN with movement, 2v2..6v6) | 50 seeds | **50 / 50** |
| `test_v2_br.py` (battle royale, 2..10 players) | 90 seeds | **90 / 90** |
| **Total** | | **6 190 / 6 190 — 100 %** |

The historical tests compared actions including `USE_WEAPON`, `USE_CHIP`, `LOST_LIFE`, `HEAL`,
`ADD_WEAPON_EFFECT`, `ADD_CHIP_EFFECT`, `STACK_EFFECT`, `REMOVE_EFFECT`,
`UPDATE_EFFECT`, `MOVE_TO`, `KILL`, `INVOCATION`, `LEEK_TURN`,
`END_TURN`, `NEW_TURN`, `START_FIGHT`, and `END_FIGHT`. These results do
not establish parity with the current Java reference.

## Coverage

- **All 36 weapons**: pistols, machine guns, lasers, shotguns, axes,
  flame thrower, the gazor family, lightninger / enhanced /
  unbridled / katana / b_laser / m_laser / j_laser / electrisor /
  rhino / etc., including weapons with `passive_effects` (NOVA_DAMAGE,
  DAMAGE_TO_*).
- **All 109 chips**: damage / heal / shield / buff / debuff / poison /
  shackle / vulnerability / antidote / aftereffect / nova / steal /
  kill / vitality / multiply_stats / push / attract / teleport /
  permutation / repel / resurrect / remove_shackles / + all 10
  summon templates.
- **All 10 summon templates**: puny_bulb, light_bulb, healer_bulb,
  fire_bulb, iced_bulb, lightning_bulb, metallic_bulb, wizard_bulb,
  savant_bulb, tactician_bulb — bulbs run their own AI, fire their
  own chip lists, and contribute to win/loss detection identically.
- **A\* pathfinding**, BFS reachability, line-of-sight, can-use-attack,
  custom maps with obstacles + team-cell placement, full Bradley-Terry
  start order with shared LCG RNG, initial cooldowns, max_uses,
  team_cooldown.

## Speed

End-to-end 1v1 fights, basic AI vs basic AI, no I/O:

```
RNG draws/sec
  Python:        1 867 543 / s
  C:            39 843 665 / s        (21x)

Full 1v1 fights
  Python:    386 fights/sec   2.57 ms/fight   65 turns avg
  C:      55 689 fights/sec   17.8 us/fight   19.5 turns avg
                                      (144x raw, 43x per-turn, 13x per-action)
```

## Build

C library + tests (Windows + VS Build Tools):

```
build.bat
```

Python bindings:

```
cd bindings/python
python setup.py build_ext --inplace
python test_v2_parity.py --seeds 5
```

For a minimal 1v1 fight without the upstream Python data files, run
`python example_fight.py` from `bindings/python`. It registers a
fictional weapon and prints the winner and numeric action stream.
The example selects a weapon with `set_weapon` before firing with
`use_weapon_on_cell`; selection costs 1 TP and emits `SET_WEAPON`. Set
`template_id` when registering a weapon if it differs from its item ID.
The older `fire_weapon` method remains available but silently equips without
the selection cost or action, so it is not Java-compatible.
`Engine.add_entity` accepts optional `start_effects=[(effect_type, value), ...]`;
the engine applies them after fight initialization and restores them on resurrection.

Missing summon templates and summon allocation or capacity failures abort the
fight. Python raises `RuntimeError`; C callers can inspect
`lw_state_get_fatal_error` and `lw_state_get_fatal_error_message`.
C AI dispatchers must stop issuing actions after a fatal error. C callers must
call `lw_state_dispose` before reinitializing or discarding a used state;
caller-owned entities remain caller-owned unless explicitly adopted and must
outlive the state. Call `lw_entity_dispose` when releasing such an entity.

Summon templates are process-global. Python registration rejects unregistered
chip IDs. Replacing a template affects all engines; owned templates are
released on replacement or `lw_bulbs_clear`.
Chip and weapon registry lookups return borrowed pointers. Retain managed
items with `lw_item_retain` before keeping them across registry replacement,
then call `lw_item_release`. Unmanaged C registrations remain caller-owned.
Managed item retain/release operations are thread-safe when each caller owns a
valid reference. Registry mutation and `lw_item_manage` require external
synchronization; lookup pointers must be retained before releasing that lock.
Registry arrays returned by `get_templates` are invalidated by mutation.

Python summon templates accept `states` and `awakening_zone`.
`summon(..., ai_id=...)` assigns an AI identifier; without an ID, `use_chip`
summons without AI. Register `set_summon_ai_callback` for ordinary summon turns
and `set_plant_awaken_callback` for plant awakenings. Clones do not inherit
callbacks: bind callbacks and external AI state to each clone separately.
A clone created inside an awakening retains the plant's active action context;
after simulating its actions, call `finish_plant_awakening()` on that clone
before continuing to other awakenings. C callers use
`lw_state_finish_cloned_awakening`. Plant parity remains under verification.

Linux is supported by the same `setup.py` (uses `gcc -O2`).

## Java differential fixtures

The optional harness requires Python, Git, JDK 25, Gradle, and a checkout of
the Java generator at commit `9edc5b96cf5ae41d1b0b6eb7b482db50d98fca77`
with its `leekscript` submodule initialized. Build the probe using the same
compiler environment as the C library:

```
cmake --build build-msvc --config Release --target lw_java_parity_probe
python tests\java_parity\compare.py --reference C:\path\to\leek-wars-generator --gradle C:\path\to\gradle.bat --c-probe build-msvc\lw_java_parity_probe.exe
```

Use the appropriate build and executable paths on other platforms. Add
`--offline` only when Gradle's dependencies are cached. The runner verifies
both pinned revisions and tracked checkout cleanliness, then compares every
recorded action, selected state values, and operation metadata. It rejects
missing or duplicate fixtures. Current low-level coverage is listed in
[MECHANICS.md](MECHANICS.md); passing it does not prove complete fight parity.

## Architecture

```
include/        public headers (lw_*.h)
src/            implementation (lw_*.c) — line-by-line Java port
tests/          C-level unit tests (CTest)
bindings/
  python/
    leekwars_c/        Cython wrapper -> _engine.pyx
    setup.py
    test_v2_*.py       parity tests vs Python upstream
```

Single translation unit per `.c` file. Effects and some summon and plant
tracking data are allocated during fights and freed with the state.

## Why a C port

Self-play and Monte-Carlo tree search need cheap simulation. The
Python engine is faithful to Java but pays Python's per-attribute
lookup cost on every state mutation. The C port keeps the Java
algorithm verbatim while running ~140× faster per fight, so a single
machine can drive hundreds of thousands of self-play games per hour
without changing the rules of the game.

## License

Same terms as the upstream Java engine.
