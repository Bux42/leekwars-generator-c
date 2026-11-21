#include <stdio.h>
#include <stdlib.h>

#include "lw_effect.h"
#include "lw_map.h"
#include "lw_state.h"

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "line %d: %s\n", __LINE__, #condition); \
        return 1; \
    } \
} while (0)

int main(void) {
    LwState *state = calloc(1, sizeof(*state));
    LwMap *map = calloc(1, sizeof(*map));
    LwEntity rooted, other;
    CHECK(state && map);
    lw_state_init(state);
    lw_map_init(map, 18, 18);
    map->state = state;
    state->map = map;
    lw_entity_init_id_name(&rooted, 1, "rooted");
    lw_entity_init_id_name(&other, 2, "other");
    lw_entity_set_state(&rooted, state, 0);
    lw_entity_set_state(&other, state, 1);
    rooted.life = rooted.m_total_life = other.life = other.m_total_life = 100;
    lw_stats_set_stat(&rooted.m_base_stats, LW_STAT_MP, 5);
    LwCell *start = lw_map_get_cell(map, 72);
    LwCell *destination = lw_map_get_cell(map, 73);
    LwCell *other_start = lw_map_get_cell(map, 144);
    lw_map_set_entity(map, &rooted, start);
    lw_map_set_entity(map, &other, other_start);
    CHECK(lw_effect_create_effect(state, LW_EFFECT_TYPE_ADD_STATE, 3, 1,
                                  LW_ENTITY_STATE_ROOTED, 0, 0, &rooted, &other,
                                  NULL, 0, 1, 0, 1, 0, 0) == LW_ENTITY_STATE_ROOTED);
    int actions = state->actions.stream.n_entries;
    LwCell *path[] = {destination};
    CHECK(lw_state_move_entity_path(state, &rooted, path, 1) == 0);
    lw_state_move_entity_cell(state, &rooted, destination);
    lw_state_slide_entity(state, &rooted, destination, &other);
    CHECK(lw_entity_get_cell(&rooted) == start && rooted.used_mp == 0);
    CHECK(state->actions.stream.n_entries == actions);
    lw_state_invert_entities(state, &other, &rooted);
    CHECK(lw_entity_get_cell(&rooted) == other_start);
    CHECK(lw_entity_get_cell(&other) == start);
    free(map);
    lw_state_dispose(state);
    free(state);
    return 0;
}
