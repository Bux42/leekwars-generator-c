#include <stdio.h>
#include <stdlib.h>

#include "lw_attack.h"
#include "lw_entity.h"
#include "lw_map.h"
#include "lw_state.h"

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "line %d: %s\n", __LINE__, #condition); \
        return 1; \
    } \
} while (0)

static int repel(int critical, int blocked_step, int rooted, int expected_distance) {
    LwState *state = calloc(1, sizeof(*state));
    LwMap *map = calloc(1, sizeof(*map));
    LwEntity caster, target;
    LwAttack attack;
    LwEffectParameters effect;
    CHECK(state && map);
    lw_state_init(state);
    lw_map_init(map, 18, 18);
    state->map = map;
    map->state = state;
    lw_entity_init_id_name(&caster, 1, "caster");
    lw_entity_init_id_name(&target, 2, "target");
    lw_entity_set_state(&caster, state, 0);
    lw_entity_set_state(&target, state, 1);
    lw_entity_set_team(&target, 1);
    caster.life = caster.m_total_life = target.life = target.m_total_life = 100;
    state->m_entities[state->n_entities++] = &caster;
    state->m_entities[state->n_entities++] = &target;
    LwCell *caster_cell = lw_map_get_cell(map, 300);
    LwCell *target_cell = lw_cell_next(caster_cell, map, 1, 0);
    CHECK(target_cell);
    LwCell *expected = target_cell;
    LwCell *current = target_cell;
    for (int step = 1; step <= 5; ++step) {
        current = lw_cell_next(current, map, 1, 0);
        CHECK(current);
        if (step == expected_distance) expected = current;
        if (step == blocked_step) current->walkable = 0;
    }
    lw_map_set_entity(map, &caster, caster_cell);
    lw_map_set_entity(map, &target, target_cell);
    if (rooted) target.states |= 1u << LW_ENTITY_STATE_ROOTED;
    lw_effect_parameters_init(&effect, LW_EFFECT_TYPE_REPEL, 4, 0, 0,
                              LW_TARGET_ENEMIES | LW_TARGET_NON_SUMMONS, 0);
    lw_attack_init(&attack, 1, 30, 7, 1, 0, &effect, 1, LW_ATTACK_TYPE_CHIP, 123, -1);
    LwEntity *affected[4];
    lw_attack_apply_on_cell(&attack, state, &caster, target_cell, critical, affected, 4);
    CHECK(lw_entity_get_cell(&target) == expected);
    free(map);
    lw_state_dispose(state);
    free(state);
    return 0;
}

int main(void) {
    CHECK(repel(0, 0, 0, 4) == 0);
    CHECK(repel(1, 0, 0, 5) == 0);
    CHECK(repel(0, 2, 0, 1) == 0);
    CHECK(repel(0, 0, 1, 0) == 0);
    return 0;
}
