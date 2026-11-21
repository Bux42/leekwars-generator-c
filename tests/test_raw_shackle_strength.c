#include <stdio.h>
#include <stdlib.h>

#include "lw_effect.h"
#include "lw_state.h"

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "line %d: %s\n", __LINE__, #condition); \
        return 1; \
    } \
} while (0)

static int apply(LwState *state, LwEntity *target, LwEntity *caster,
                 double value1, double value2, double jet, double aoe,
                 int critical, int modifiers) {
    return lw_effect_create_effect(state, LW_EFFECT_TYPE_RAW_SHACKLE_STRENGTH,
                                   3, aoe, value1, value2,
                                   critical, target, caster, NULL, jet,
                                   1, 0, 1, 0, modifiers);
}

int main(void) {
    LwState *state = calloc(1, sizeof(*state));
    LwEntity target, caster;
    CHECK(state != NULL);
    lw_state_init(state);
    lw_entity_init_default(&target);
    lw_entity_init_default(&caster);
    target.state = state;
    caster.state = state;
    lw_stats_set_stat(&target.m_base_stats, LW_STAT_STRENGTH, 200);
    lw_stats_set_stat(&caster.m_base_stats, LW_STAT_MAGIC, 200);

    CHECK(apply(state, &target, &caster, 100, 0, 0, 1, 0, 0) == 100);
    CHECK(lw_entity_get_strength(&target) == 100);
    CHECK(target.n_effects == 1 && caster.n_launched_effects == 1);
    CHECK(state->actions.stream.n_entries == 1);
    CHECK(state->actions.stream.entries[0].type == LW_ACTION_ADD_CHIP_EFFECT);
    CHECK(state->actions.stream.entries[0].v[4] == 65);
    CHECK(state->actions.stream.entries[0].v[5] == 100);
    lw_entity_remove_shackles(&target);
    CHECK(lw_entity_get_strength(&target) == 200);
    CHECK(target.n_effects == 0 && caster.n_launched_effects == 0);

    CHECK(apply(state, &target, &caster, 10, 4, 0.5, 0.5, 1,
                LW_MODIFIER_IRREDUCTIBLE) == 8);
    CHECK(lw_entity_get_strength(&target) == 192);
    lw_entity_remove_shackles(&target);
    CHECK(lw_entity_get_strength(&target) == 192);
    CHECK(target.n_effects == 1 && caster.n_launched_effects == 1);
    lw_entity_reduce_effects(&target, 1.0, &caster);
    CHECK(lw_entity_get_strength(&target) == 192);

    CHECK(apply(state, &target, &caster, 0, 0, 0, 1, 0, 0) == 0);
    CHECK(target.n_effects == 1);
    CHECK(lw_effect_create_effect(state, LW_EFFECT_TYPE_SHACKLE_STRENGTH,
                                  3, 1, 10, 0, 0, &target, &caster, NULL,
                                  0, 1, 0, 1, 0, LW_MODIFIER_IRREDUCTIBLE) == 30);
    CHECK(lw_entity_get_strength(&target) == 162);
    lw_entity_remove_shackles(&target);
    CHECK(lw_entity_get_strength(&target) == 162 && target.n_effects == 2);
    CHECK(lw_effect_create_effect(state, LW_EFFECT_TYPE_DAMAGE_TO_RESISTANCE, 3, 1, 100, 0, 0, &target,
                                  &caster, NULL, 0, 1, 0, 1, 0, 0) == 0);
    CHECK(target.n_effects == 2);
    lw_state_dispose(state);
    free(state);
    return 0;
}
