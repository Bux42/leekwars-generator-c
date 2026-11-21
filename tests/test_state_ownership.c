#include <stdio.h>
#include <stdlib.h>

#include "lw_entity.h"
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
    LwState *clone = calloc(1, sizeof(*clone));
    LwMap *map = calloc(1, sizeof(*map));
    LwEntity borrowed;
    LwEntity *owned = calloc(1, sizeof(*owned));
    CHECK(state && clone && map && owned);
    lw_state_init(state);
    lw_map_init(map, 18, 18);
    state->map = map;
    lw_entity_init_id_name(&borrowed, 1, "borrowed");
    CHECK(lw_entity_add_start_effect(&borrowed, LW_EFFECT_TYPE_RAW_SHACKLE_STRENGTH, 123));
    lw_state_add_entity(state, 0, &borrowed);
    borrowed.type = LW_ENTITY_TYPE_BULB;
    borrowed.m_skin = 13;
    borrowed.m_has_moved = 1;
    borrowed.n_item_uses = 1;
    borrowed.item_uses[0].key = 42;
    borrowed.item_uses[0].value = 2;
    borrowed.item_uses[0].present = 1;
    lw_entity_init_id_name(owned, 2, "owned");
    lw_state_adopt_entity(state, owned);
    lw_state_add_entity(state, 0, owned);
    lw_state_remove_invocation(state, owned, 1);
    CHECK(state->n_entities == 1);

    CHECK(lw_state_clone(clone, state));
    CHECK(clone->n_entities == 1);
    CHECK(clone->m_entities[0]->type == LW_ENTITY_TYPE_BULB);
    CHECK(clone->m_entities[0]->m_skin == 13);
    CHECK(clone->m_entities[0]->m_has_moved == 1);
    CHECK(lw_entity_get_item_uses(clone->m_entities[0], 42) == 2);
    CHECK(clone->m_entities[0]->n_start_effects == 1);
    CHECK(clone->m_entities[0]->start_effects[0].type == LW_EFFECT_TYPE_RAW_SHACKLE_STRENGTH);
    CHECK(clone->m_entities[0]->start_effects[0].value == 123);
    lw_state_dispose(state);
    lw_entity_dispose(&borrowed);
    CHECK(borrowed.m_id == 1);
    CHECK(clone->n_entities == 1 && clone->m_entities[0] != &borrowed);
    CHECK(clone->m_entities[0]->start_effects[0].value == 123);
    lw_state_dispose(state);
    lw_state_free_clone(clone);
    free(map);
    free(clone);
    free(state);
    return 0;
}
