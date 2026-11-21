#include <stdio.h>
#include <stdlib.h>

#include "lw_bulb_template.h"
#include "lw_chip.h"
#include "lw_effect.h"
#include "lw_map.h"
#include "lw_state.h"
#include "lw_team.h"

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "line %d: %s\n", __LINE__, #condition); \
        return 1; \
    } \
} while (0)

int main(void) {
    LwState *state = calloc(1, sizeof(*state));
    LwMap *map = calloc(1, sizeof(*map));
    LwEntity caster;
    LwTeam team;
    LwChip chip;
    LwEffectParameters summon;
    LwBulbTemplate bulb;
    CHECK(state && map);
    lw_state_init(state);
    lw_map_init(map, 18, 18);
    map->state = state;
    state->map = map;
    lw_team_init(&team);
    state->teams[0] = &team;
    state->n_teams = 1;
    lw_entity_init_id_name(&caster, 1, "caster");
    lw_entity_set_state(&caster, state, 0);
    caster.life = caster.m_total_life = 100;
    lw_stats_set_stat(&caster.m_base_stats, LW_STAT_TP, 6);
    lw_team_add_entity(&team, &caster);
    state->m_entities[state->n_entities++] = &caster;
    lw_order_add_entity(&state->order, &caster);
    lw_map_set_entity(map, &caster, lw_map_get_cell(map, 72));
    lw_effect_parameters_init(&summon, LW_EFFECT_TYPE_SUMMON, 9000, 0, 0, 0, 0);
    lw_chip_init(&chip, 123, 2, 1, 30, &summon, 1, 7, 1, 0,
                 0, 0, 0, 1, 123, "test", LW_CHIP_TYPE_BULB, -1);
    lw_bulb_template_init(&bulb, 9000, "test", 100, 100, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0, 6, 6, 3, 3);
    lw_bulbs_add_invocation_template(&bulb);
    CHECK(lw_effect_create_effect(state, LW_EFFECT_TYPE_ADD_STATE, 3, 1,
                                  LW_ENTITY_STATE_STERILE, 0, 0, &caster, &caster,
                                  NULL, 0, 1, 0, 1, 0, 0) == LW_ENTITY_STATE_STERILE);
    LwCell *target = lw_map_get_cell(map, 73);
    caster.used_tp = 5;
    CHECK(lw_state_summon_entity(state, &caster, target, &chip) == LW_ATTACK_USE_NOT_ENOUGH_TP);
    caster.used_tp = 0;
    int actions = state->actions.stream.n_entries;
    uint64_t rng = state->rng_n;
    CHECK(lw_state_summon_entity(state, &caster, target, &chip) == LW_ATTACK_USE_INVALID_STATE);
    CHECK(caster.used_tp == 0 && state->rng_n == rng && state->n_entities == 1);
    CHECK(state->actions.stream.n_entries == actions);
    lw_entity_reduce_effects_total(&caster, 1, &caster);
    CHECK(lw_state_summon_entity(state, &caster, target, &chip) > 0);
    CHECK(caster.used_tp == 2 && state->n_entities == 2);
    lw_state_dispose(state);
    lw_bulbs_clear();
    free(map);
    free(state);
    return 0;
}
