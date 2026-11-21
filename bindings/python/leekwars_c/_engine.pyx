# cython: language_level=3, boundscheck=False, wraparound=False
"""
leekwars_c._engine -- Cython binding for the line-by-line Java port.

Minimal API focused on parity testing:

    eng = Engine()
    eng.add_weapon(item_id=37, name='pistol', cost=3, min_range=1, max_range=7,
                    launch_type=1, area_id=1, los=True, max_uses=-1,
                    effects=[(1, 15.0, 5.0, 0, 31, 0)])  # (type, v1, v2, turns, targets, modifiers)
    eng.add_farmer(1, 'A', 'fr')
    eng.add_farmer(2, 'B', 'fr')
    eng.add_team(1, 'T1')
    eng.add_team(2, 'T2')
    eng.add_entity(team=0, fid=1, name='A', cell=72, ..., weapons=[37])
    eng.add_entity(team=1, fid=2, name='B', cell=144, ..., weapons=[37])
    eng.set_seed(1)
    eng.set_custom_map(obstacles={}, team1=[72], team2=[144])
    eng.set_ai_dispatch(my_callback)   # called per-entity per-turn
    outcome = eng.run()
    stream = eng.stream_dump()         # list of dicts in Java JSON shape
"""

from libc.stdint cimport int64_t, uint64_t
from libc.stdlib cimport malloc, free, calloc
from libc.string cimport memset, memcpy, strncpy
import warnings


# ===================== C declarations ============================

cdef extern from "lw_constants.h":
    int LW_ENTITY_TYPE_LEEK
    int LW_FIGHT_TYPE_SOLO
    int LW_FIGHT_TYPE_FARMER
    int LW_FIGHT_TYPE_TEAM
    int LW_FIGHT_TYPE_BATTLE_ROYALE
    int LW_CONTEXT_TEST


cdef extern from "lw_action_stream.h":
    int LW_LOG_MAX_ARGS
    ctypedef struct LwActionLog:
        int type
        int v[8]
        int n_args
        int extra_offset
        int extra_len

    ctypedef struct LwActionStream:
        int          enabled
        int          n_entries
        LwActionLog  entries[2048]
        int          extra[8192]
        int          n_extra
        int          next_effect_log_id


cdef extern from "lw_actions.h":
    ctypedef struct LwActions:
        LwActionStream stream

    void lw_actions_init(LwActions *self)


cdef extern from "lw_state.h":
    int LW_STATE_ERROR_SUMMON_DISPATCH
    ctypedef struct LwState:
        uint64_t              rng_n
        int                   n_teams
        int                   n_entities
        int                   m_state
        LwActions             actions
        int                   context
        int                   type
        int                   seed
        int                   draw_check_life
    int lw_state_clone(LwState *self, const LwState *src)
    int lw_state_finish_cloned_awakening(LwState *self)
    void lw_state_dispose(LwState *self)
    int lw_state_get_fatal_error(const LwState *self)
    const char *lw_state_get_fatal_error_message(const LwState *self)
    void lw_state_set_fatal_error(LwState *self, int error, const char *message)


cdef extern from "lw_features.h":
    int LW_FEAT_TOTAL
    int LW_FEAT_SLOTS
    int LW_FEAT_FIELDS_PER_SLOT
    int LW_FEAT_SPATIAL_C
    int LW_FEAT_SPATIAL_H
    int LW_FEAT_SPATIAL_W
    int LW_FEAT_SPATIAL_TOTAL
    int lw_features_extract_v(const LwState *state, int active_idx,
                               float *out, int out_len)
    int lw_features_extract_spatial(const LwState *state, int active_idx,
                                      float *out, int out_len)


cdef extern from "lw_outcome.h":
    int LW_OUTCOME_MAX_LOGS

    ctypedef struct LwOutcomeLogEntry:
        int     key
        void   *value

    ctypedef struct LwOutcome:
        LwActions          fight
        LwOutcomeLogEntry  logs[64]
        int                n_logs
        int                winner
        int                duration
        void              *statistics
        int                exception_status
        char               exception_message[512]
        int64_t            analyze_time
        int64_t            compilation_time
        int64_t            execution_time

    void lw_outcome_init(LwOutcome *self)


cdef extern from "lw_fight.h":
    ctypedef int (*lw_fight_ai_dispatch_t)(LwFight *fight, LwEntity *entity, void *ai_file,
                                            int turn, void *userdata)
    ctypedef int (*lw_fight_plant_ai_dispatch_t)(LwFight *fight, LwEntity *plant, LwEntity *trigger,
                                                  int64_t ai_id, int turn, void *userdata)

    ctypedef struct LwFight:
        int                  m_winteam
        void                *generator
        int                  m_id
        int                  m_boss
        int                  m_start_farmer
        int                  max_turns
        int64_t              execution_time
        void                *listener
        LwState             *state
        lw_fight_ai_dispatch_t ai_dispatch
        void                *ai_dispatch_userdata

    void lw_fight_init(LwFight *self, LwGenerator *generator)
    void lw_fight_set_state(LwFight *self, LwState *state)
    void lw_fight_set_plant_ai_dispatch(LwFight *self, lw_fight_plant_ai_dispatch_t fn,
                                         void *userdata)
    int lw_fight_summon_entity_with_ai_id(LwFight *self, LwEntity *caster,
                                           LwCell *target, LwChip *chip, int64_t ai_id)


cdef extern from "lw_entity_info.h":
    int LW_NAME_MAX
    int LW_AI_PATH_MAX
    int LW_COUNTRY_MAX
    int LW_FARMER_NAME_MAX
    int LW_TEAM_NAME_MAX
    int LW_COMP_NAME_MAX
    int LW_ENTITY_INFO_MAX_WEAPONS
    int LW_ENTITY_INFO_MAX_CHIPS

    ctypedef struct LwStartEffect:
        int type
        int value

    ctypedef struct LwFarmerInfo:
        int  id
        char name[64]
        char country[16]

    ctypedef struct LwTeamInfo:
        int  id
        char name[64]
        char composition_name[64]
        int  has_composition_name
        int  level
        char turret_ai_path[256]
        int  turret_ai_owner

    ctypedef struct LwEntityInfo:
        int  id
        char name[64]
        char ai[256]
        int  ai_folder
        char ai_path[256]
        int  ai_version
        int  ai_strict
        int  aiOwner
        int  type
        int  farmer
        int  team
        int  level
        int  dead
        int  life
        int  tp
        int  mp
        int  strength
        int  agility
        int  frequency
        int  wisdom
        int  resistance
        int  science
        int  magic
        int  cores
        int  ram
        int  chips[32]
        int  n_chips
        int  weapons[16]
        int  n_weapons
        LwStartEffect *start_effects
        int n_start_effects
        int start_effects_capacity
        int  cell
        int  has_cell
        int  skin
        int  hat
        int  metal
        int  face
        int  custom_class
        int  orientation
        void *ai_file

    void lw_entity_info_init(LwEntityInfo *self)
    void lw_entity_info_dispose(LwEntityInfo *self)
    int lw_entity_info_add_start_effect(LwEntityInfo *self, int type, int value)


cdef extern from "lw_scenario.h":
    int LW_SCENARIO_MAX_FARMERS
    int LW_SCENARIO_MAX_TEAMS
    int LW_SCENARIO_MAX_ENTITIES_PER_TEAM
    int LW_SCENARIO_MAP_OBSTACLES

    ctypedef struct LwCustomMap:
        int present
        int id
        int width
        int height
        int n_obstacles
        int obstacles_cell[256]
        int obstacles_type[256]
        int team1[16]
        int n_team1
        int team2[16]
        int n_team2

    ctypedef struct LwScenario:
        int64_t       seed
        int           max_turns
        int           type
        int           context
        int           fight_id
        int           boss
        int           draw_check_life
        LwCustomMap   map
        # opaque internal arrays

    void lw_scenario_init(LwScenario *self)
    int lw_scenario_add_entity(LwScenario *self, int team_id, const LwEntityInfo *entity)
    void lw_scenario_dispose(LwScenario *self)
    void lw_scenario_add_farmer(LwScenario *self, const LwFarmerInfo *farmer)
    void lw_scenario_add_team  (LwScenario *self, const LwTeamInfo *team)


cdef extern from "lw_generator.h":
    ctypedef struct LwGenerator:
        int use_leekscript_cache
        lw_fight_ai_dispatch_t ai_dispatch
        void                  *ai_dispatch_userdata

    void lw_generator_init(LwGenerator *self)
    void lw_generator_set_ai_dispatch(LwGenerator *self,
                                      lw_fight_ai_dispatch_t fn,
                                      void *userdata)
    void lw_generator_set_plant_ai_dispatch(LwGenerator *self,
                                            lw_fight_plant_ai_dispatch_t fn,
                                            void *userdata)
    int  lw_generator_run_scenario(LwGenerator *self,
                                   LwScenario *scenario,
                                   void *listener,
                                   void *register_manager,
                                   void *statistics_manager,
                                   LwState *state,
                                   LwFight *fight,
                                   LwOutcome *outcome)


cdef extern from "lw_effect_params.h":
    ctypedef struct LwEffectParameters:
        int    id
        double value1
        double value2
        int    turns
        int    targets
        int    modifiers


cdef extern from "lw_attack.h":
    ctypedef struct LwAttack:
        pass

cdef extern from "lw_item.h":
    ctypedef struct LwItem:
        pass
    int lw_item_manage(LwItem *self)
    void lw_item_release(LwItem *self)


cdef extern from "lw_weapon.h":
    ctypedef struct LwWeapon:
        pass

    void lw_weapon_init(LwWeapon *self,
                         int id, int cost, int min_range, int max_range,
                         const LwEffectParameters *effects, int n_effects,
                         int launch_type, int area_id, int los,
                         int template_id, const char *name,
                         const LwEffectParameters *passive_effects, int n_passives,
                         int max_uses)
    int       lw_weapons_add_weapon(LwWeapon *weapon)
    LwWeapon *lw_weapons_get_weapon(int id)


cdef extern from "lw_chip.h":
    ctypedef struct LwChip:
        pass

    ctypedef int LwChipType

    void lw_chip_init(LwChip *self,
                       int id, int cost, int min_range, int max_range,
                       const LwEffectParameters *effects, int n_effects,
                       int launch_type, int area, int los,
                       int cooldown, int team_cooldown, int initial_cooldown,
                       int level, int template_, const char *name,
                       LwChipType chip_type, int max_uses)
    int     lw_chips_add_chip(LwChip *chip)
    LwChip *lw_chips_get_chip(int id)


cdef extern from "lw_entity.h":
    ctypedef struct LwEntity:
        int has_summon_ai
        int64_t summon_ai_id
    int lw_entity_has_weapon(const LwEntity *self, int id)
    LwChip *lw_entity_get_chip(const LwEntity *self, int id)

cdef extern from "lw_map.h":
    ctypedef struct LwMap:
        pass
    ctypedef struct LwCell:
        pass
    LwCell *lw_map_get_cell(LwMap *self, int id)

cdef extern from "lw_state.h":
    LwMap *lw_state_get_map(LwState *self)
    int lw_state_set_weapon(LwState *self, LwEntity *entity, LwWeapon *weapon)
    int lw_state_use_weapon(LwState *self, LwEntity *launcher, LwCell *target)


# Helpers exposed by lw_glue.c (avoids extern-of-static-inline issues).
cdef extern:
    int      lw_state_index_of_entity(const LwState *state, const LwEntity *e)
    LwEntity* lw_state_get_entity_at  (LwState *state, int idx)
    int      lw_state_n_entities      (const LwState *state)
    int      lw_glue_entity_alive    (const LwEntity *e)
    int      lw_glue_entity_hp       (const LwEntity *e)
    int      lw_glue_entity_fid      (const LwEntity *e)
    int      lw_glue_entity_team     (const LwEntity *e)
    int      lw_glue_entity_cell_id  (const LwEntity *e)
    int      lw_glue_entity_used_tp  (const LwEntity *e)
    int      lw_glue_entity_used_mp  (const LwEntity *e)
    void     lw_glue_entity_mark_has_ai(LwEntity *e)
    int      lw_glue_apply_use_weapon(LwState *state, int entity_idx,
                                       int weapon_id, int target_cell_id)
    int      lw_glue_apply_use_chip  (LwState *state, LwFight *fight,
                                       int entity_idx, int chip_id,
                                       int target_cell_id)
    int      lw_glue_move_toward_cell  (LwState *state, int entity_idx,
                                          int target_cell_id, int max_mp)
    int      lw_glue_cell_distance      (LwState *state, int cell_a, int cell_b)
    int      lw_glue_cell_distance2     (LwState *state, int cell_a, int cell_b)


# Generator AI dispatch hook
cdef extern from "lw_generator.h":
    void lw_generator_set_ai_dispatch(LwGenerator *self,
                                      lw_fight_ai_dispatch_t fn,
                                      void *userdata)


cdef extern from "lw_bulb_template.h":
    ctypedef struct LwBulbTemplate:
        pass
    void lw_bulb_template_init(LwBulbTemplate *self, int id, const char *name,
                                int min_life, int max_life,
                                int min_strength, int max_strength,
                                int min_wisdom, int max_wisdom,
                                int min_agility, int max_agility,
                                int min_resistance, int max_resistance,
                                int min_science, int max_science,
                                int min_magic, int max_magic,
                                int min_tp, int max_tp,
                                int min_mp, int max_mp)
    int lw_bulb_template_add_chip(LwBulbTemplate *self, LwChip *chip)
    int lw_bulb_template_add_state(LwBulbTemplate *self, int state)
    void lw_bulb_template_set_awakening_zone(LwBulbTemplate *self, int zone)
    void lw_bulb_template_dispose(LwBulbTemplate *self)
    int lw_bulbs_add_owned_invocation_template(LwBulbTemplate *invocation)


# ===================== Python-side wrappers ======================

# C trampoline that the Fight calls on each entity's turn. It dispatches
# to the Python callback installed via Engine.set_ai_callback.
cdef int _ai_trampoline(LwFight *fight, LwEntity *entity, void *ai_file,
                         int turn, void *userdata) noexcept with gil:
    cdef Engine eng = <Engine>userdata
    cdef LwEntity *e = entity
    cdef int idx = lw_state_index_of_entity(&eng.state, e)
    try:
        if e.has_summon_ai:
            if eng._summon_ai_callback is None:
                lw_state_set_fatal_error(&eng.state, LW_STATE_ERROR_SUMMON_DISPATCH,
                                         "summon AI dispatcher is not configured")
                return -1
            ret = eng._summon_ai_callback(e.summon_ai_id, idx, turn)
        elif eng._ai_callback is not None:
            ret = eng._ai_callback(idx, turn)
        else:
            return 0
        return int(ret) if ret is not None else 0
    except Exception:
        if lw_state_get_fatal_error(&eng.state):
            return -1
        import traceback; traceback.print_exc()
        return -1


cdef int _plant_trampoline(LwFight *fight, LwEntity *plant, LwEntity *trigger,
                           int64_t ai_id, int turn, void *userdata) noexcept with gil:
    cdef Engine eng = <Engine>userdata
    cdef int plant_idx = lw_state_index_of_entity(&eng.state, plant)
    cdef int trigger_idx = lw_state_index_of_entity(&eng.state, trigger)
    try:
        ret = eng._plant_ai_callback(ai_id, plant_idx, trigger_idx, turn)
        return int(ret) if ret is not None else 0
    except Exception:
        if lw_state_get_fatal_error(&eng.state):
            return -1
        import traceback; traceback.print_exc()
        return -1


cdef class Engine:
    """Top-level container for a single fight."""
    cdef LwGenerator generator
    cdef LwScenario  scenario
    cdef LwOutcome   outcome
    cdef LwState     state
    cdef LwFight     fight
    cdef object      _ai_callback     # Python callable
    cdef object      _summon_ai_callback
    cdef object      _plant_ai_callback
    cdef list        _registered_weapons  # keep alive
    cdef list        _registered_chips    # keep alive
    cdef bint        _is_clone        # True if state was filled via clone()
    cdef bint        _has_state
    cdef bint        _running

    def __cinit__(self):
        lw_generator_init(&self.generator)
        lw_scenario_init(&self.scenario)
        lw_outcome_init(&self.outcome)
        memset(&self.state, 0, sizeof(LwState))
        memset(&self.fight, 0, sizeof(LwFight))
        self._ai_callback = None
        self._summon_ai_callback = None
        self._plant_ai_callback = None
        self._registered_weapons = []
        self._registered_chips = []
        self._is_clone = False
        self._has_state = False
        self._running = False
        # Sensible defaults
        self.scenario.max_turns = 30
        self.scenario.type = LW_FIGHT_TYPE_SOLO
        self.scenario.context = LW_CONTEXT_TEST
        self.scenario.seed = 1
        # Wire the AI trampoline -- the actual Python callback is set
        # via set_ai_callback below.
        lw_generator_set_ai_dispatch(&self.generator, _ai_trampoline,
                                      <void*>self)

    def __dealloc__(self):
        if self._has_state:
            lw_state_dispose(&self.state)
        lw_scenario_dispose(&self.scenario)

    def clone(self):
        """Return a deep snapshot of this engine's current state.

        The returned Engine shares the global weapon/chip/bulb
        registries (immutable post-setup) but has its own copy of all
        entities, teams, effects, the map, and the action stream.
        Atomic actions (fire_weapon / use_chip / move_toward) applied
        on the clone do not affect the parent.

        Limitations:
          * The clone can't run a complete fight with ``run()``. Use it for
            state queries + atomic action rollouts.
          * Entity effect lists retain their existing fixed capacities.
          * A clone created during a plant callback retains that plant's
            active awakening. Call ``finish_plant_awakening()`` after its
            simulated actions, before continuing with other awakenings.
          * AI callbacks are not copied; install clone-specific callbacks.
        """
        cdef Engine c = Engine()
        # The fresh Engine's state was zero-init'd by __cinit__; we
        # overwrite it with the deep clone of self.state. The C helper
        # heap-allocates entities/teams/map and attaches them. The
        # __dealloc__ above releases them when the clone is GC'd.
        if not lw_state_clone(&c.state, &self.state):
            raise MemoryError("state clone allocation failed")
        c._has_state = True
        lw_fight_init(&c.fight, &c.generator)
        lw_fight_set_state(&c.fight, &c.state)
        # Outcome is a POD struct with the winner / duration / log
        # entries from the last run() call -- copy verbatim so
        # `clone.winner` and friends report the same values as the
        # parent. New atomic actions on the clone don't update outcome
        # (only run() writes to it), which is fine: outcome is a
        # post-fight snapshot, and clones we make for rollouts are
        # query-only on the outcome.
        memcpy(&c.outcome, &self.outcome, sizeof(LwOutcome))
        c._is_clone = True
        # Share the registries (Python lists). They're immutable after
        # setup, so cloning them would just waste memory.
        c._registered_weapons = self._registered_weapons
        c._registered_chips = self._registered_chips
        return c

    def finish_plant_awakening(self):
        """Finish a plant awakening copied into this clone."""
        self._raise_if_fatal()
        if not self._is_clone or not lw_state_finish_cloned_awakening(&self.state):
            raise RuntimeError("no cloned plant awakening is active")

    def extract_v_features(self, int active_idx, float[::1] out):
        """Fill ``out`` (a contiguous float32 buffer of length 256)
        with the V-network feature vector for ``active_idx``.

        Layout: 16 entity slots * 16 fields each. Slot 0 is always
        the active entity; remaining slots walk state.m_entities[]
        in insertion order, skipping the active. See lw_features.h
        for the full per-slot field list.

        Returns the number of slots actually populated (1..16).

        Caller pre-allocates the buffer:

            >>> import numpy as np
            >>> buf = np.zeros(256, dtype=np.float32)
            >>> n = eng.extract_v_features(0, buf)
        """
        if out.shape[0] < LW_FEAT_TOTAL:
            raise ValueError(
                f"out buffer too small: got {out.shape[0]}, need {LW_FEAT_TOTAL}"
            )
        return lw_features_extract_v(&self.state, active_idx,
                                       &out[0], out.shape[0])

    def feature_dim(self):
        """Return the size (in floats) of the V-feature buffer."""
        return LW_FEAT_TOTAL

    def extract_spatial_features(self, int active_idx, float[::1] out):
        """Fill ``out`` (a contiguous float32 buffer of length 2520)
        with the (C=4, H=18, W=35) spatial tensor: walkability +
        my/enemy/dead occupancy. Layout is NCHW (channel outermost),
        so the buffer can be reshaped to (4, 18, 35) and fed to a
        PyTorch Conv2d directly.

        Returns 1 on success, 0 on failure (e.g., bad active_idx).
        """
        if out.shape[0] < LW_FEAT_SPATIAL_TOTAL:
            raise ValueError(
                f"out buffer too small: got {out.shape[0]}, need {LW_FEAT_SPATIAL_TOTAL}"
            )
        return lw_features_extract_spatial(&self.state, active_idx,
                                             &out[0], out.shape[0])

    def spatial_dim(self):
        """Return the (C, H, W) shape of the spatial tensor."""
        return (LW_FEAT_SPATIAL_C, LW_FEAT_SPATIAL_H, LW_FEAT_SPATIAL_W)

    def set_ai_callback(self, callback):
        """Register a Python function called once per entity per turn.

        Signature: callback(entity_idx, turn) -> int (operation count).
        Inside the callback, use eng.fire_weapon(entity_idx, weapon_id,
        cell) etc. to apply actions.
        """
        self._ai_callback = callback

    def set_summon_ai_callback(self, callback):
        """Set callback(ai_id, entity_idx, turn) for ordinary summon turns."""
        if callback is not None and not callable(callback):
            raise TypeError("summon AI callback must be callable or None")
        self._summon_ai_callback = callback

    def set_plant_awaken_callback(self, callback):
        """Set callback(ai_id, plant_idx, trigger_idx, turn) for awakenings.

        Clones start without this callback; install one bound to the clone.
        """
        if callback is not None and not callable(callback):
            raise TypeError("plant awakening callback must be callable or None")
        self._plant_ai_callback = callback
        if callback is None:
            lw_generator_set_plant_ai_dispatch(&self.generator, NULL, NULL)
            lw_fight_set_plant_ai_dispatch(&self.fight, NULL, NULL)
        else:
            lw_generator_set_plant_ai_dispatch(&self.generator, _plant_trampoline, <void*>self)
            lw_fight_set_plant_ai_dispatch(&self.fight, _plant_trampoline, <void*>self)

    # ---- bootstrap registries ----
    def add_weapon(self, int item_id, str name, int cost,
                   int min_range, int max_range, int launch_type,
                   int area_id, bint los, int max_uses,
                   list effects, list passive_effects=None, int template_id=-1):
        """Register a weapon. template_id defaults to item_id.

        effects = list of (type,v1,v2,turns,targets,modifiers).
        """
        cdef LwWeapon* w = NULL
        cdef int n_eff = len(effects)
        cdef int n_pas = 0 if passive_effects is None else len(passive_effects)
        cdef LwEffectParameters* eff = NULL
        cdef LwEffectParameters* pas = NULL
        cdef bytes name_b = name.encode('utf-8')
        cdef bint managed = False
        cdef int i
        cdef tuple e
        try:
            w = <LwWeapon*>calloc(1, sizeof(LwWeapon))
            eff = <LwEffectParameters*>calloc(max(n_eff, 1), sizeof(LwEffectParameters))
            pas = <LwEffectParameters*>calloc(max(n_pas, 1), sizeof(LwEffectParameters))
            if w == NULL or eff == NULL or pas == NULL:
                raise MemoryError("weapon allocation failed")
            for i in range(n_eff):
                e = effects[i]
                eff[i].id        = e[0]
                eff[i].value1    = e[1]
                eff[i].value2    = e[2]
                eff[i].turns     = e[3]
                eff[i].targets   = e[4]
                eff[i].modifiers = e[5]
            for i in range(n_pas):
                e = passive_effects[i]
                pas[i].id        = e[0]
                pas[i].value1    = e[1]
                pas[i].value2    = e[2]
                pas[i].turns     = e[3]
                pas[i].targets   = e[4]
                pas[i].modifiers = e[5]
            lw_weapon_init(w, item_id, cost, min_range, max_range,
                        eff, n_eff, launch_type, area_id, 1 if los else 0,
                        item_id if template_id == -1 else template_id,
                        name_b, pas, n_pas, max_uses)
            if not lw_item_manage(<LwItem*>w):
                raise MemoryError("weapon name allocation failed")
            managed = True
            if not lw_weapons_add_weapon(w):
                raise MemoryError("weapon registration failed")
            self._registered_weapons.append((item_id, name))
        finally:
            free(eff)
            free(pas)
            if w != NULL:
                if managed:
                    lw_item_release(<LwItem*>w)
                else:
                    free(w)

    def add_chip(self, int item_id, str name, int cost,
                 int min_range, int max_range, int launch_type,
                 int area_id, bint los, int max_uses,
                 int cooldown, int initial_cooldown, bint team_cooldown,
                 int level, int chip_type,
                 list effects, int template_id=0):
        """Register one chip. effects = list of (type,v1,v2,turns,targets,modifiers).
        chip_type matches LwChipType enum (NONE=0, DAMAGE=1, HEAL=2,
        RETURN=3, PROTECTION=4, BOOST=5, POISON=6, SHACKLE=7, BULB=8,
        TACTIC=9) -- this is the JSON field "type".
        """
        cdef LwChip* c = NULL
        cdef int n_eff = len(effects)
        cdef LwEffectParameters* eff = NULL
        cdef bytes name_b = name.encode('utf-8')
        cdef bint managed = False
        cdef int i
        cdef tuple e
        try:
            c = <LwChip*>calloc(1, sizeof(LwChip))
            eff = <LwEffectParameters*>calloc(max(n_eff, 1), sizeof(LwEffectParameters))
            if c == NULL or eff == NULL:
                raise MemoryError("chip allocation failed")
            for i in range(n_eff):
                e = effects[i]
                eff[i].id        = e[0]
                eff[i].value1    = e[1]
                eff[i].value2    = e[2]
                eff[i].turns     = e[3]
                eff[i].targets   = e[4]
                eff[i].modifiers = e[5]
            lw_chip_init(c, item_id, cost, min_range, max_range,
                      eff, n_eff, launch_type, area_id, 1 if los else 0,
                      cooldown, 1 if team_cooldown else 0, initial_cooldown,
                      level, template_id, name_b, chip_type, max_uses)
            if not lw_item_manage(<LwItem*>c):
                raise MemoryError("chip name allocation failed")
            managed = True
            if not lw_chips_add_chip(c):
                raise MemoryError("chip registration failed")
            self._registered_chips.append((item_id, name))
        finally:
            free(eff)
            if c != NULL:
                if managed:
                    lw_item_release(<LwItem*>c)
                else:
                    free(c)

    # ---- scenario setup ----
    def add_farmer(self, int id, str name, str country):
        cdef LwFarmerInfo f
        memset(&f, 0, sizeof(LwFarmerInfo))
        f.id = id
        cdef bytes nb = name.encode('utf-8')
        cdef bytes cb = country.encode('utf-8')
        strncpy(f.name, nb, 63)
        strncpy(f.country, cb, 15)
        lw_scenario_add_farmer(&self.scenario, &f)

    def add_team(self, int id, str name):
        cdef LwTeamInfo t
        memset(&t, 0, sizeof(LwTeamInfo))
        t.id = id
        cdef bytes nb = name.encode('utf-8')
        strncpy(t.name, nb, 63)
        lw_scenario_add_team(&self.scenario, &t)

    def add_entity(self, int team, int fid, str name, int level,
                   int life, int tp, int mp, int strength, int agility,
                   int frequency, int wisdom, int resistance, int science,
                   int magic, int cores, int ram,
                   int farmer, int team_id,
                   list weapons, list chips=None,
                   int cell=-1, list start_effects=None):
        """Add a leek; each start effect is an (effect_type, value) pair."""
        cdef LwEntityInfo e
        lw_entity_info_init(&e)
        e.id = fid
        cdef bytes nb = name.encode('utf-8')
        strncpy(e.name, nb, 63)
        e.type = LW_ENTITY_TYPE_LEEK
        e.farmer = farmer
        e.team = team_id
        e.level = level
        e.life = life
        e.tp = tp
        e.mp = mp
        e.strength = strength
        e.agility = agility
        e.frequency = frequency
        e.wisdom = wisdom
        e.resistance = resistance
        e.science = science
        e.magic = magic
        e.cores = cores
        e.ram = ram
        if cell >= 0:
            e.cell = cell
            e.has_cell = 1
        cdef int i
        e.n_weapons = min(len(weapons), 16)
        for i in range(e.n_weapons):
            e.weapons[i] = weapons[i]
        if chips is not None:
            e.n_chips = min(len(chips), 32)
            for i in range(e.n_chips):
                e.chips[i] = chips[i]
        try:
            if start_effects is not None:
                for effect in start_effects:
                    if len(effect) != 2:
                        raise ValueError("each start effect must be a (type, value) pair")
                    if not lw_entity_info_add_start_effect(
                        &e, int(effect[0]), int(effect[1])
                    ):
                        raise MemoryError("could not store entity start effect")
            if not lw_scenario_add_entity(&self.scenario, team, &e):
                raise MemoryError("could not add entity to scenario")
        finally:
            lw_entity_info_dispose(&e)

    def add_summon_template(self, int id, str name,
                             tuple life, tuple strength, tuple wisdom,
                             tuple agility, tuple resistance, tuple science,
                             tuple magic, tuple tp, tuple mp,
                             list chip_ids, states=None, int awakening_zone=0):
        """Register a Bulb (summon) template. life=(min,max) etc.
        chip_ids = list of chip ids the bulb knows (must already be
        registered via add_chip).
        Raises ValueError for missing chips.
        Templates belong to the process-global registry, not the Engine.
        """
        cdef bytes nb = name.encode('utf-8')
        cdef LwBulbTemplate *t = <LwBulbTemplate*>calloc(1, sizeof(LwBulbTemplate))
        cdef int cid
        cdef int state_id
        cdef LwChip *c
        if t == NULL:
            raise MemoryError("template allocation failed")
        try:
            lw_bulb_template_init(t, id, nb,
                               int(life[0]), int(life[1]),
                               int(strength[0]), int(strength[1]),
                               int(wisdom[0]), int(wisdom[1]),
                               int(agility[0]), int(agility[1]),
                               int(resistance[0]), int(resistance[1]),
                               int(science[0]), int(science[1]),
                               int(magic[0]), int(magic[1]),
                               int(tp[0]), int(tp[1]),
                               int(mp[0]), int(mp[1]))
            for cid in chip_ids:
                c = lw_chips_get_chip(cid)
                if c == NULL:
                    raise ValueError(f"chip {cid} is not registered")
                if not lw_bulb_template_add_chip(t, c):
                    raise MemoryError("cannot allocate summon template chips")
            if states is not None:
                for state_id in states:
                    if not lw_bulb_template_add_state(t, state_id):
                        raise MemoryError("cannot allocate summon template states")
            lw_bulb_template_set_awakening_zone(t, awakening_zone)
            if not lw_bulbs_add_owned_invocation_template(t):
                raise MemoryError("summon template registration failed")
            t = NULL
        finally:
            if t != NULL:
                lw_bulb_template_dispose(t)
                free(t)

    def set_seed(self, int seed):
        self.scenario.seed = seed

    def set_max_turns(self, int n):
        self.scenario.max_turns = n

    def set_type(self, int t):
        """Fight type: 0=SOLO, 1=FARMER, 2=TEAM, 3=BATTLE_ROYALE."""
        self.scenario.type = t

    def set_custom_map(self, dict obstacles=None, list team1=None, list team2=None,
                        int map_id=0, int width=18, int height=18):
        """Configure a custom map: obstacles dict {cell_id: type_id},
        team1/team2 cell lists. Use map_id != 0 to mean "respect entity
        initial_cell during placement". The Python upstream uses id=0
        with team1/team2 lists for parity tests."""
        cdef int i
        memset(&self.scenario.map, 0, sizeof(LwCustomMap))
        self.scenario.map.present = 1
        self.scenario.map.id = map_id
        self.scenario.map.width = width
        self.scenario.map.height = height
        if obstacles:
            items = list(obstacles.items())
            n = min(len(items), 256)
            self.scenario.map.n_obstacles = n
            for i in range(n):
                k, v = items[i]
                self.scenario.map.obstacles_cell[i] = int(k)
                self.scenario.map.obstacles_type[i] = int(v)
        if team1:
            n = min(len(team1), 16)
            self.scenario.map.n_team1 = n
            for i in range(n):
                self.scenario.map.team1[i] = int(team1[i])
        if team2:
            n = min(len(team2), 16)
            self.scenario.map.n_team2 = n
            for i in range(n):
                self.scenario.map.team2[i] = int(team2[i])

    # ---- run ----
    def run(self):
        """Run the scenario; return its status or raise on a fatal engine error."""
        cdef int rc
        if self._is_clone:
            raise RuntimeError("cannot run a cloned engine")
        if self._running:
            raise RuntimeError("engine is already running")
        if self._has_state:
            lw_state_dispose(&self.state)
        self._has_state = True
        self._running = True
        try:
            rc = lw_generator_run_scenario(
                &self.generator, &self.scenario,
                NULL, NULL, NULL,
                &self.state, &self.fight, &self.outcome)
        finally:
            self._running = False
        self._raise_if_fatal()
        return rc

    # ---- low-level entity access (called from inside the AI callback) ----
    cdef void _raise_if_fatal(self) except *:
        if lw_state_get_fatal_error(&self.state):
            raise RuntimeError((<bytes>lw_state_get_fatal_error_message(&self.state)).decode("utf-8"))

    def n_entities(self):
        return self.state.n_entities

    cdef LwEntity* _entity_at(self, int idx) except NULL:
        cdef int n = lw_state_n_entities(&self.state)
        cdef LwEntity *e
        if idx < 0 or idx >= n:
            raise IndexError("entity_idx out of range")
        e = lw_state_get_entity_at(&self.state, idx)
        if e == NULL:
            raise RuntimeError("entity is NULL")
        return e

    def entity_alive(self, int idx):
        return bool(lw_glue_entity_alive(self._entity_at(idx)))
    def entity_hp(self, int idx):
        return lw_glue_entity_hp(self._entity_at(idx))
    def entity_fid(self, int idx):
        return lw_glue_entity_fid(self._entity_at(idx))
    def entity_team(self, int idx):
        return lw_glue_entity_team(self._entity_at(idx))
    def entity_cell(self, int idx):
        return lw_glue_entity_cell_id(self._entity_at(idx))
    def entity_used_tp(self, int idx):
        return lw_glue_entity_used_tp(self._entity_at(idx))
    def entity_used_mp(self, int idx):
        return lw_glue_entity_used_mp(self._entity_at(idx))

    def set_weapon(self, int entity_idx, int weapon_id):
        """Select an owned weapon, spending 1 TP and logging SET_WEAPON.

        Return False for insufficient TP, amputation, or an unknown/unowned
        weapon. Unknown and unowned weapons also produce a warning.
        """
        cdef LwEntity *entity
        cdef LwWeapon *weapon
        self._raise_if_fatal()
        entity = self._entity_at(entity_idx)
        weapon = lw_weapons_get_weapon(weapon_id)
        if weapon == NULL:
            warnings.warn(f"Unknown weapon {weapon_id}", RuntimeWarning, stacklevel=2)
            return False
        if not lw_entity_has_weapon(entity, weapon_id):
            warnings.warn(f"Entity does not own weapon {weapon_id}", RuntimeWarning, stacklevel=2)
            return False
        return bool(lw_state_set_weapon(&self.state, entity, weapon))

    def use_weapon_on_cell(self, int entity_idx, int target_cell):
        """Fire the selected weapon at another cell; return Attack.USE_*."""
        cdef int result
        cdef LwEntity *entity
        cdef LwMap *map_
        cdef LwCell *target
        self._raise_if_fatal()
        entity = self._entity_at(entity_idx)
        map_ = lw_state_get_map(&self.state)
        if map_ == NULL:
            raise RuntimeError("fight map is not initialized")
        target = lw_map_get_cell(map_, target_cell)
        if target == NULL or target_cell == lw_glue_entity_cell_id(entity):
            return -1
        result = lw_state_use_weapon(&self.state, entity, target)
        self._raise_if_fatal()
        return result

    def fire_weapon(self, int entity_idx, int weapon_id, int target_cell):
        """Legacy: silently equip and fire without the Java selection cost.

        Use set_weapon and use_weapon_on_cell for explicit selection.
        Returns Attack.USE_* result code:
            2 = USE_CRITICAL, 1 = USE_SUCCESS,
           -1 = INVALID_TARGET, -2 = NOT_ENOUGH_TP,
           -4 = INVALID_POSITION, etc.
        """
        cdef int result
        self._raise_if_fatal()
        result = lw_glue_apply_use_weapon(&self.state, entity_idx, weapon_id, target_cell)
        self._raise_if_fatal()
        return result

    def move_toward(self, int entity_idx, int target_cell, int max_mp=10):
        """Move the entity toward target_cell using up to max_mp MP.
        Returns the number of MP actually used (0 if no movement)."""
        cdef int result
        self._raise_if_fatal()
        result = lw_glue_move_toward_cell(&self.state, entity_idx, target_cell, max_mp)
        self._raise_if_fatal()
        return result

    def cell_distance(self, int cell_a, int cell_b):
        """Manhattan distance between two cell ids (Pathfinding.getCaseDistance)."""
        return lw_glue_cell_distance(&self.state, cell_a, cell_b)

    def cell_distance2(self, int cell_a, int cell_b):
        """Squared Euclidean distance between two cell ids
        (Map.getDistance2 -- used by fight_class.getNearestEnemy)."""
        return lw_glue_cell_distance2(&self.state, cell_a, cell_b)

    def use_chip(self, int entity_idx, int chip_id, int target_cell):
        """Apply a USE_CHIP action. Returns Attack.USE_* result code (same
        codes as fire_weapon). The chip must have been registered first
        via add_chip(...) AND attached to the entity via add_entity(chips=[...]).
        Summon-typed chips dispatch through Fight.summonEntity which sets
        up the bulb's AI / birthTurn / etc; other chips go through state.useChip.
        """
        cdef int result
        self._raise_if_fatal()
        result = lw_glue_apply_use_chip(&self.state, &self.fight, entity_idx, chip_id, target_cell)
        self._raise_if_fatal()
        return result

    def summon(self, int entity_idx, int chip_id, int target_cell, ai_id=None):
        """Use a summon chip, optionally assigning an opaque integer AI id."""
        cdef LwEntity *entity
        cdef LwChip *chip
        cdef LwMap *map_
        cdef LwCell *target
        cdef int64_t numeric_id
        cdef int result
        if ai_id is None:
            return self.use_chip(entity_idx, chip_id, target_cell)
        numeric_id = ai_id
        self._raise_if_fatal()
        entity = self._entity_at(entity_idx)
        chip = lw_entity_get_chip(entity, chip_id)
        if chip == NULL:
            return -1
        map_ = lw_state_get_map(&self.state)
        if map_ == NULL:
            raise RuntimeError("fight map is not initialized")
        target = lw_map_get_cell(map_, target_cell)
        if target == NULL:
            return -1
        result = lw_fight_summon_entity_with_ai_id(&self.fight, entity, target, chip, numeric_id)
        self._raise_if_fatal()
        return result

    # ---- read action stream ----
    def stream_dump(self):
        """Return the action stream as a list of dicts."""
        cdef list out = []
        cdef int n = self.outcome.fight.stream.n_entries
        cdef int i, j
        cdef LwActionLog *e
        for i in range(n):
            e = &self.outcome.fight.stream.entries[i]
            args = []
            for j in range(e.n_args):
                args.append(e.v[j])
            d = {'type': e.type, 'args': args}
            if e.extra_len > 0:
                extra = []
                for j in range(e.extra_len):
                    extra.append(self.outcome.fight.stream.extra[e.extra_offset + j])
                d['extra'] = extra
            out.append(d)
        return out

    @property
    def winner(self):
        return self.outcome.winner

    @property
    def duration(self):
        return self.outcome.duration

    @property
    def rng_state(self):
        return self.state.rng_n


def hello():
    """Sanity check that the module imports."""
    return 'leekwars_c ready'
