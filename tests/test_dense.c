#include <stdio.h>
#include <stdlib.h>

#include "lw_chip.h"
#include "lw_effect.h"
#include "lw_map.h"
#include "lw_state.h"

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "line %d: %s\n", __LINE__, #condition); \
        return 1; \
    } \
} while (0)

typedef struct {
    LwState state;
    LwMap map;
    LwEntity caster;
    LwEntity target;
} Fixture;

static Fixture *fixture_new(void) {
    Fixture *f = calloc(1, sizeof(*f));
    if (!f) return NULL;
    lw_state_init(&f->state);
    lw_map_init(&f->map, 18, 18);
    f->map.state = &f->state;
    f->state.map = &f->map;
    lw_entity_init_id_name(&f->caster, 1, "caster");
    lw_entity_init_id_name(&f->target, 2, "target");
    lw_entity_set_state(&f->caster, &f->state, 0);
    lw_entity_set_state(&f->target, &f->state, 1);
    lw_entity_set_team(&f->caster, 0);
    lw_entity_set_team(&f->target, 1);
    f->caster.life = f->caster.m_total_life = 100;
    f->target.life = f->target.m_total_life = 100;
    lw_stats_set_stat(&f->caster.m_base_stats, LW_STAT_TP, 6);
    f->state.m_entities[f->state.n_entities++] = &f->caster;
    f->state.m_entities[f->state.n_entities++] = &f->target;
    lw_order_add_entity(&f->state.order, &f->caster);
    lw_map_set_entity(&f->map, &f->caster, lw_map_get_cell(&f->map, 72));
    lw_map_set_entity(&f->map, &f->target, lw_map_get_cell(&f->map, 144));
    return f;
}

static void chip_init(LwChip *chip, int id, int effect_id, int targets) {
    LwEffectParameters effect;
    lw_effect_parameters_init(&effect, effect_id, 0, 0, 0, targets, 0);
    lw_chip_init(chip, id, 2, 1, 30, &effect, 1, 7, 1, 0,
                 0, 0, 0, 1, id, "test", LW_CHIP_TYPE_TACTIC, -1);
}

static int add_dense(Fixture *f, LwEntity *target) {
    return lw_effect_create_effect(&f->state, LW_EFFECT_TYPE_ADD_STATE, 3, 1,
                                   LW_ENTITY_STATE_DENSE, 0, 0, target, &f->caster,
                                   NULL, 0, 1, 0, 1, 0, 0);
}

static int dense_caster_cannot_teleport(void) {
    Fixture *f = fixture_new();
    LwChip chip;
    CHECK(f);
    chip_init(&chip, 1, LW_EFFECT_TYPE_TELEPORT, 0);
    CHECK(add_dense(f, &f->caster) == LW_ENTITY_STATE_DENSE);
    int actions = f->state.actions.stream.n_entries;
    uint64_t rng = f->state.rng_n;
    LwCell *start = lw_entity_get_cell(&f->caster);
    CHECK(lw_state_use_chip(&f->state, &f->caster, lw_map_get_cell(&f->map, 73), &chip)
          == LW_ATTACK_USE_INVALID_STATE);
    CHECK(lw_entity_get_cell(&f->caster) == start && f->caster.used_tp == 0);
    CHECK(f->state.rng_n == rng && f->state.actions.stream.n_entries == actions);
    lw_state_dispose(&f->state);
    free(f);
    return 0;
}

static int dense_target_only_blocks_applicable_permutation(void) {
    Fixture *f = fixture_new();
    LwChip allowed, blocked;
    CHECK(f);
    chip_init(&blocked, 2, LW_EFFECT_TYPE_PERMUTATION, LW_TARGET_ENEMIES | LW_TARGET_NON_SUMMONS);
    chip_init(&allowed, 3, LW_EFFECT_TYPE_PERMUTATION, LW_TARGET_ALLIES | LW_TARGET_NON_SUMMONS);
    CHECK(add_dense(f, &f->target) == LW_ENTITY_STATE_DENSE);
    int actions = f->state.actions.stream.n_entries;
    uint64_t rng = f->state.rng_n;
    LwCell *target_cell = lw_entity_get_cell(&f->target);
    CHECK(lw_state_use_chip(&f->state, &f->caster, target_cell, &blocked) == LW_ATTACK_USE_INVALID_STATE);
    CHECK(f->caster.used_tp == 0 && f->state.rng_n == rng);
    CHECK(f->state.actions.stream.n_entries == actions);
    CHECK(lw_state_use_chip(&f->state, &f->caster, target_cell, &allowed) > 0);
    CHECK(f->caster.used_tp == 2 && lw_entity_get_cell(&f->target) == target_cell);
    lw_state_dispose(&f->state);
    free(f);
    return 0;
}

int main(void) {
    CHECK(dense_caster_cannot_teleport() == 0);
    CHECK(dense_target_only_blocks_applicable_permutation() == 0);
    return 0;
}
