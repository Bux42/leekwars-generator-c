# Fight mechanics parity inventory

Target: official Java generator commit
[`9edc5b96cf5ae41d1b0b6eb7b482db50d98fca77`](https://github.com/leek-wars/leek-wars-generator/tree/9edc5b96cf5ae41d1b0b6eb7b482db50d98fca77).
The C port does not record an exact Java base revision; its Python
reference originated around April 2026. This is an initial inventory, not
proof that all differences have been found. Parity must be checked against
the pinned Java implementation, not just the older Python port.

## Identified changes

- **Raw strength shackle (effect 65):** fixed strength reduction, unaffected
  by caster magic; removable unless irreducible. Implemented in C. Host-defined
  start effects now apply at fight initialization and are restored after
  resurrection.
  [Java effect](https://github.com/leek-wars/leek-wars-generator/blob/9edc5b96cf5ae41d1b0b6eb7b482db50d98fca77/src/main/java/com/leekwars/generator/effect/EffectRawShackleStrength.java)
- **Periodic effects:** poison, aftereffect, and heal-over-time duration
  counts actual applications on the target's turns. This is a prerequisite
  for current superinfection behavior. Aftereffect ticks respect invincibility.
  [Java periodic effects](https://github.com/leek-wars/leek-wars-generator/blob/9edc5b96cf5ae41d1b0b6eb7b482db50d98fca77/src/main/java/com/leekwars/generator/effect/PeriodicEffect.java)
- **Superinfection (effect 64):** detonates finite poisons, consumes one
  tick from each, and deals poison damage with erosion. Implemented in C,
  including effect-duration update actions; one direct Java fixture matches.
  [Java effect](https://github.com/leek-wars/leek-wars-generator/blob/9edc5b96cf5ae41d1b0b6eb7b482db50d98fca77/src/main/java/com/leekwars/generator/effect/EffectSuperinfection.java)
- **Damage-to-resistance (effect 63):** direct damage converts to a
  temporary resistance buff. The passive hook is implemented in C. Both Java
  and C reject effect 63 as a directly created effect; it creates a raw
  resistance buff instead. Java differential fixtures cover stacking and
  replacement across two damage events and buff expiration.
- **Effect and state rules:** non-stacking state effects, reduction/removal
  behavior, unhealable vitality, and irreducible shackles. State
  replacement/reduction, unhealable vitality, amputation weapon restrictions, and
  raw-shackle removal are implemented; other rules require comparison.
- **States:** sterile summon restrictions, amputation weapon restrictions,
  rooted movement restrictions, and dense teleport/permutation restrictions
  are implemented. Plant behavior requires comparison with Java.
- **Summons:** corn, chilli pepper and prototaxite; plant awakening during
  movement, action ordering, summon cooldowns and resurrection. Awakening
  support is in progress, including a callback exposing the triggering entity;
  initial summon cooldowns and Colossus multipliers are applied. Java
  differential tests and several edge cases remain outstanding.
- **Movement and attacks:** fixed-distance repel (including critical distance
  and obstacle stops) and dead-target movement guards are implemented. A*,
  movement toward obstacles, other push rules, and line targeting remain to compare.
- **Fight initialization and outcome:** fixed-map obstacles and occupied
  starting cells, per-AI RNG, turn order, resurrection, battle-royale winner
  and draw handling, and action-stream differences.
- **Baseline stats:** scenario-provided level, life, TP/MP, strength, agility,
  frequency, wisdom, resistance, science, magic, cores and RAM. C maps these
  fields during entity creation, but has no Java differential fixture for the
  full set yet; no specific mismatch is confirmed. Loadout parity is excluded.
  Host-defined initial effects are applied and restored on revival.
- **Python weapon API:** `set_weapon` and `use_weapon_on_cell` expose explicit
  selection and firing. Legacy `fire_weapon` still silently selects a weapon
  without the Java selection TP cost or action log; it is not parity-compatible.
- **Item data:** compared with the Python reference snapshot, the target has
  11 additional chips, 4 additional weapons and 3 additional summons, plus
  changes to existing chips and summons. The library needs compatible data
  supplied by its caller; mechanics and data must both be verified.

## Verification still required

Expand direct Java-versus-C differential tests with fixed seeds, identical
scenarios and equivalent AI actions. Compare outcome, state, RNG-dependent
events and action stream across existing mechanics and each new mechanic.
Review the remaining Java changes and add missing differences to this list;
passing isolated C tests alone does not establish full parity.
The differential harness currently checks seventeen cases:
superinfection with mixed, infinite-only and invincible poison targets, and
plant awakening on walking, sliding, teleporting and a non-entry teleport.
It also compares statistics-backed operation metadata, raw strength shackling,
damage-to-resistance stacking and replacement, poison, aftereffect and
healing duration, start-effect restoration after resurrection, two consecutive
turns without AI, and turret start-of-fight state application. The harness
compares selected resulting state values, every action emitted during each
operation, and the operations map. It does not execute complete fights or
establish parity for other effects, awakening scenarios, item data, AI
decisions or RNG.
See [README.md](README.md) for running the comparison.

An older-Python regression sample using reference commit
`bb0077919619347d03167738528a359c0e836883` matched normalized actions and
winners in 99 of 108 fights (36 weapons, three seeds each), and 50 of 50
mixed-action fights. Nine weapon fights diverged on poison timing: double gun,
flame thrower and gazor, each at all three seeds. The current Java fixture
confirms two poison ticks occur on the target's turns; it does not reproduce
those complete weapon fights. The historical normalizers discard some action
types, and their scripts may suppress AI errors, so these results do not prove
full action-stream or current-Java parity.

## Reliability and integration findings

- Fixed random-map initialization passing an integer to `lw_map_set_entity`.
  Seeded two-entity fights now place both entities and can damage a target.
- Corrected several incompatible declarations and statistics callback stubs.
  GCC and MSVC build and pass the focused tests; a GCC LTO build with
  `-Werror=lto-type-mismatch` also passes for those test executables. This
  does not prove every declaration in the library is correct.
- Fight-level statistics callbacks for initialization, characteristics,
  timing, errors and fight completion now forward to the registered
  statistics manager; its `generator_fight` back-reference is retained.
  Java `Actions.addOpsAndTimes` output metadata is retained through the
  statistics manager's operations-map callbacks. AI timing values and
  register-manager persistence remain unimplemented; Python JSON output
  does not yet serialize the retained operations map.
- Poison and aftereffect now have distinct statistics damage types while
  both continue to log action code 110, matching Java.
- Team cooldowns now decrement at round boundaries instead of being silently
  left unchanged by a no-op; focused summon tests cover their expiry.
- Missing summon templates, allocation/initialization failures, and summon
  storage exhaustion now report fatal errors before logging or spending TP.
  The fight aborts; Python raises `RuntimeError`. Missing-template behavior
  has regression coverage; allocation failure has not been injected.
- Summon registry grows dynamically and supports borrowed C templates and
  owned templates released on replacement or registry clear. Python rejects
  missing chip IDs. Template chip lists grow dynamically; entities still have
  a separate 32-chip storage limit, which is not Java parity.
- State cleanup tracks generated and cloned allocations separately from
  borrowed objects, including summons removed before fight completion.
  Python disposes state on rerun or destruction. C callers must dispose a
  used state before reinitializing it. Clone allocation failures are
  reported and partial allocations released; failure injection is untested.
  Clone snapshots retain entity combat fields and can summon without changing
  the source fight. Opaque AI state is not copied.
- Item and summon registries remain process-global; replacement affects other
  engines. Chip and weapon registries grow dynamically and retain managed
  items. Entities, clones, effects and summon templates retain their managed
  items; callers holding lookup pointers across replacement must retain them.
  Python registrations own item names and release their temporary buffers.
  Managed item reference counting supports concurrent retain/release, but
  registry access during mutation still requires external synchronization.
- Effects now use state-owned allocations released on state disposal, rather
  than the wrapping global pool. Allocation failure sets a fatal state error.
  Clones preserve identity between an effect and its launched-effect entry.
  Effects retain their managed attack items until state disposal.
- Plant awakening has focused C and Python coverage and one direct Java
  movement comparison, but other scenarios remain unverified. Summons support
  optional AI IDs and clone-specific dispatch;
  callbacks and external AI state are not copied. Clones created during an
  awakening preserve the active plant, and require explicit completion of
  the cloned awakening before later awakenings can run.

## Deferred robustness investigation

Per user decision, investigate these in a red-team exercise after the
mechanics update rather than redesigning them now:

- An entity silently drops active or launched effects beyond 64 entries.
- The action stream silently drops entries beyond 2,048; complete logs are
  not currently a priority.
- Per user decision, entity and team cooldowns silently drop entries beyond
  32; investigate this limit during the red-team exercise as well.
- Per user decision, per-turn item-use tracking beyond 32 distinct items is
  also deferred; dropped entries can bypass per-item use limits.

These are known limits on correctness and must not be presented as full
parity. Their reachability in official fights remains to be established.
