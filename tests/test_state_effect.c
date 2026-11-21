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

static int add_state(LwState *state, LwEntity *target, LwEntity *caster,
                     int entity_state, int modifiers) {
    return lw_effect_create_effect(state, LW_EFFECT_TYPE_ADD_STATE, 3, 1,
                                   entity_state, 0, 0, target, caster, NULL,
                                   0, 1, 0, 1, 0, modifiers);
}

int main(void) {
    LwState *state = calloc(1, sizeof(*state));
    LwEntity target, caster;
    LwWeapon weapon;
    CHECK(state != NULL);
    lw_state_init(state);
    lw_entity_init_id_name(&target, 1, "target");
    lw_entity_init_id_name(&caster, 2, "caster");
    target.state = caster.state = state;
    target.life = target.m_total_life = 100;
    caster.life = caster.m_total_life = 100;

    CHECK(add_state(state, &target, &caster, LW_ENTITY_STATE_STERILE, 0) == LW_ENTITY_STATE_STERILE);
    CHECK(add_state(state, &target, &caster, LW_ENTITY_STATE_STERILE, 0) == LW_ENTITY_STATE_STERILE);
    CHECK(target.n_effects == 1 && caster.n_launched_effects == 1);
    CHECK(target.effects[0]->value == LW_ENTITY_STATE_STERILE && lw_entity_has_state(&target, LW_ENTITY_STATE_STERILE));
    lw_entity_reduce_effects(&target, 0.4, &caster);
    CHECK(target.effects[0]->value == LW_ENTITY_STATE_STERILE && lw_entity_has_state(&target, LW_ENTITY_STATE_STERILE));
    lw_entity_reduce_effects(&target, 0.5, &caster);
    CHECK(target.effects[0]->value == LW_ENTITY_STATE_STERILE && lw_entity_has_state(&target, LW_ENTITY_STATE_STERILE));
    lw_entity_reduce_effects(&target, 0.52, &caster);
    CHECK(target.n_effects == 0 && !lw_entity_has_state(&target, LW_ENTITY_STATE_STERILE));

    CHECK(add_state(state, &target, &caster, LW_ENTITY_STATE_INVINCIBLE,
                    LW_MODIFIER_IRREDUCTIBLE) == LW_ENTITY_STATE_INVINCIBLE);
    lw_entity_reduce_effects(&target, 1, &caster);
    CHECK(lw_entity_has_state(&target, LW_ENTITY_STATE_INVINCIBLE));
    lw_entity_reduce_effects_total(&target, 1, &caster);
    CHECK(!lw_entity_has_state(&target, LW_ENTITY_STATE_INVINCIBLE));

    lw_weapon_init(&weapon, 123, 1, 1, 1, NULL, 0, 1, 1, 1, 123, "test", NULL, 0, 0);
    lw_stats_set_stat(&target.m_base_stats, LW_STAT_TP, 6);
    lw_order_add_entity(&state->order, &target);
    lw_entity_set_weapon(&target, &weapon);
    CHECK(add_state(state, &target, &caster, LW_ENTITY_STATE_AMPUTATED, 0) == LW_ENTITY_STATE_AMPUTATED);
    CHECK(lw_entity_get_weapon(&target) == NULL);
    int actions_before = state->actions.stream.n_entries;
    CHECK(lw_state_set_weapon(state, &target, &weapon) == 0);
    CHECK(lw_state_use_weapon(state, &target, NULL) == LW_ATTACK_USE_INVALID_STATE);
    CHECK(lw_entity_get_weapon(&target) == NULL && lw_entity_get_tp(&target) == 6);
    CHECK(state->actions.stream.n_entries == actions_before);

    CHECK(add_state(state, &target, &caster, LW_ENTITY_STATE_UNHEALABLE, 0) == LW_ENTITY_STATE_UNHEALABLE);
    target.life = 50;
    int actions = state->actions.stream.n_entries;
    CHECK(lw_effect_create_effect(state, LW_EFFECT_TYPE_VITALITY, 0, 1, 30, 0, 0,
                                  &target, &caster, NULL, 0, 1, 0, 1, 0, 0) == 30);
    CHECK(target.life == 50 && target.m_total_life == 130);
    CHECK(state->actions.stream.entries[actions].type == LW_ACTION_NOVA_VITALITY);
    lw_entity_reduce_effects_total(&target, 1, &caster);
    actions = state->actions.stream.n_entries;
    CHECK(lw_effect_create_effect(state, LW_EFFECT_TYPE_VITALITY, 0, 1, 30, 0, 0,
                                  &target, &caster, NULL, 0, 1, 0, 1, 0, 0) == 30);
    CHECK(target.life == 80 && target.m_total_life == 160);
    CHECK(state->actions.stream.entries[actions].type == LW_ACTION_VITALITY);
    lw_state_dispose(state);
    free(state);
    return 0;
}
