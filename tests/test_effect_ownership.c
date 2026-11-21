#include <stdio.h>
#include <stdlib.h>

#include "lw_effect.h"
#include "lw_entity.h"
#include "lw_state.h"

LwEffect *lw_state_alloc_effect(LwState *state);

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "line %d: %s\n", __LINE__, #condition); \
        return 1; \
    } \
} while (0)

int main(void) {
    LwState *state = calloc(1, sizeof(*state));
    LwState *clone = calloc(1, sizeof(*clone));
    LwEntity caster, target;
    CHECK(state && clone);
    lw_state_init(state);
    lw_entity_init_id_name(&caster, 1, "caster");
    lw_entity_init_id_name(&target, 2, "target");
    lw_state_add_entity(state, 0, &caster);
    lw_state_add_entity(state, 0, &target);

    LwEffect *first = lw_state_alloc_effect(state);
    CHECK(first);
    first->id = 17;
    first->caster = &caster;
    first->target = &target;
    lw_entity_add_effect(&target, first);
    lw_entity_add_launched_effect(&caster, first);
    for (int i = 0; i < 4100; i++) {
        LwEffect *effect = lw_state_alloc_effect(state);
        CHECK(effect);
        effect->id = 20;
    }
    CHECK(first->id == 17);
    CHECK(lw_state_clone(clone, state));
    CHECK(clone->m_entities[1]->effects[0] ==
          clone->m_entities[0]->launched_effects[0]);
    CHECK(clone->m_entities[1]->effects[0] != first);
    lw_state_dispose(state);
    CHECK(target.n_effects == 0 && caster.n_launched_effects == 0);
    CHECK(clone->m_entities[1]->effects[0]->id == 17);
    lw_state_dispose(clone);
    free(clone);
    free(state);
    return 0;
}
