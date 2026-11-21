#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "lw_entity.h"
#include "lw_fight.h"
#include "lw_state.h"

int main(void) {
    LwState *state = calloc(1, sizeof(*state));
    LwFight fight = {0};
    LwEntity entity;
    if (!state) return 1;
    lw_entity_init_id_name(&entity, -1, "entity");
    state->m_entities[0] = &entity;
    state->n_entities = 1;
    fight.state = state;
    if (lw_fight_get_entity(&fight, -1) != &entity ||
        lw_fight_get_entity(&fight, INT64_C(4294967295)) != NULL) {
        fprintf(stderr, "entity lookup truncated a 64-bit ID\n");
        free(state);
        return 1;
    }
    free(state);
    return 0;
}
