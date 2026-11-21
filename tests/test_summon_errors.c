#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lw_chip.h"
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
    lw_effect_parameters_init(&summon, LW_EFFECT_TYPE_SUMMON, 999999, 0, 0, 0, 0);
    lw_chip_init(&chip, 123, 2, 1, 30, &summon, 1, 7, 1, 0,
                 0, 0, 0, 1, 123, "test", LW_CHIP_TYPE_BULB, -1);

    uint64_t rng = state->rng_n;
    int actions = state->actions.stream.n_entries;
    CHECK(lw_state_summon_entity(state, &caster, lw_map_get_cell(map, 73), &chip)
          == LW_STATE_ERROR_SUMMON_TEMPLATE);
    CHECK(lw_state_get_fatal_error(state) == LW_STATE_ERROR_SUMMON_TEMPLATE);
    CHECK(strstr(lw_state_get_fatal_error_message(state), "999999") != NULL);
    CHECK(state->rng_n == rng && state->actions.stream.n_entries == actions);
    CHECK(state->n_entities == 1 && team.n_entities == 1 && state->order.n_leeks == 1);
    CHECK(state->m_next_entity_id == 0 && caster.used_tp == 0);
    CHECK(lw_map_get_entity(map, lw_map_get_cell(map, 73)) == -1);
    free(map);
    lw_state_dispose(state);
    free(state);
    return 0;
}
