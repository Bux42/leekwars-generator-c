#include <stdio.h>
#include <stdlib.h>

#include "lw_effect.h"
#include "lw_state.h"
#include "lw_weapon.h"

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "line %d: %s\n", __LINE__, #condition); \
        return 1; \
    } \
} while (0)

static int passive(int modifiers) {
    LwState *state = calloc(1, sizeof(*state));
    LwEntity target;
    LwWeapon weapon;
    LwEffectParameters effect;
    CHECK(state != NULL);
    lw_state_init(state);
    lw_entity_init_id_name(&target, 1, "target");
    target.state = state;
    target.life = target.m_total_life = 1000;
    lw_effect_parameters_init(&effect, LW_EFFECT_TYPE_DAMAGE_TO_RESISTANCE, 25, 0, 2, 0, modifiers);
    lw_weapon_init(&weapon, 123, 1, 1, 1, NULL, 0, 1, 1, 1, 123,
                   "test", &effect, 1, 0);
    lw_entity_add_weapon(&target, &weapon);

    lw_entity_on_direct_damage(&target, 100);
    CHECK(lw_entity_get_resistance(&target) == 25);
    CHECK(target.n_effects == 1 && target.effects[0]->turns == 2);
    CHECK(target.effects[0]->id == LW_EFFECT_TYPE_RAW_BUFF_RESISTANCE);
    CHECK(target.n_launched_effects == 1);
    lw_entity_on_direct_damage(&target, 21);
    CHECK(lw_entity_get_resistance(&target) == (modifiers ? 30 : 5));
    CHECK(target.n_effects == 1 && target.n_launched_effects == 1);
    lw_entity_start_turn(&target);
    lw_entity_start_turn(&target);
    CHECK(lw_entity_get_resistance(&target) == 0 && target.n_effects == 0);
    target.life = 0;
    lw_entity_on_direct_damage(&target, 100);
    CHECK(target.n_effects == 0);
    lw_state_dispose(state);
    free(state);
    return 0;
}

int main(void) {
    CHECK(passive(0) == 0);
    CHECK(passive(LW_MODIFIER_STACKABLE) == 0);
    return 0;
}
