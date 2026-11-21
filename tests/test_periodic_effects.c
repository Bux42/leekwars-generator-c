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

typedef struct {
    LwState state;
    LwEntity caster;
    LwEntity target;
} Fixture;

static Fixture *fixture_new(void) {
    Fixture *f = calloc(1, sizeof(*f));
    if (!f) return NULL;
    lw_state_init(&f->state);
    lw_entity_init_id_name(&f->caster, 1, "caster");
    lw_entity_init_id_name(&f->target, 2, "target");
    f->caster.state = &f->state;
    f->target.state = &f->state;
    f->caster.life = f->caster.m_total_life = 1000;
    f->target.life = f->target.m_total_life = 1000;
    return f;
}

static int add_effect(Fixture *f, int id, int turns, LwEntity *target) {
    return lw_effect_create_effect(&f->state, id, turns, 1, 30, 0, 0,
                                   target, &f->caster, NULL, 0, 1, 0, 1, 0, 0);
}

static int poison_ticks_on_target_turns(void) {
    Fixture *f = fixture_new();
    CHECK(f != NULL);
    CHECK(add_effect(f, LW_EFFECT_TYPE_POISON, 2, &f->target) == 30);
    LwEffect *poison = f->target.effects[0];
    lw_entity_start_turn(&f->caster);
    CHECK(poison->turns == 2 && f->target.life == 1000);
    lw_entity_start_turn(&f->target);
    CHECK(poison->turns == 1 && f->target.life == 970);
    lw_entity_start_turn(&f->caster);
    CHECK(poison->turns == 1);
    lw_entity_start_turn(&f->target);
    CHECK(f->target.life == 940);
    CHECK(f->target.n_effects == 0 && f->caster.n_launched_effects == 0);
    lw_state_dispose(&f->state);
    free(f);
    return 0;
}

static int self_and_infinite_poison(void) {
    Fixture *f = fixture_new();
    CHECK(f != NULL);
    CHECK(add_effect(f, LW_EFFECT_TYPE_POISON, 2, &f->caster) == 30);
    lw_entity_start_turn(&f->caster);
    CHECK(f->caster.life == 970 && f->caster.effects[0]->turns == 1);
    lw_entity_start_turn(&f->caster);
    CHECK(f->caster.life == 940);
    CHECK(f->caster.n_effects == 0 && f->caster.n_launched_effects == 0);
    CHECK(add_effect(f, LW_EFFECT_TYPE_POISON, -1, &f->target) == 30);
    lw_entity_start_turn(&f->caster);
    lw_entity_start_turn(&f->target);
    lw_entity_start_turn(&f->target);
    CHECK(f->target.life == 940 && f->target.effects[0]->turns == -1);
    lw_state_dispose(&f->state);
    free(f);
    return 0;
}

static int heal_aftereffect_and_buff(void) {
    Fixture *f = fixture_new();
    CHECK(f != NULL);
    f->target.life = 500;
    CHECK(add_effect(f, LW_EFFECT_TYPE_HEAL, 2, &f->target) == 30);
    CHECK(add_effect(f, LW_EFFECT_TYPE_AFTEREFFECT, 2, &f->target) == 30);
    CHECK(add_effect(f, LW_EFFECT_TYPE_BUFF_STRENGTH, 1, &f->target) == 30);
    lw_entity_start_turn(&f->caster);
    CHECK(f->target.n_effects == 2 && f->caster.n_launched_effects == 2);
    CHECK(f->target.effects[0]->turns == 2 && f->target.effects[1]->turns == 2);
    lw_entity_start_turn(&f->target);
    CHECK(f->target.effects[0]->turns == 1 && f->target.effects[1]->turns == 1);
    lw_entity_start_turn(&f->target);
    CHECK(f->target.n_effects == 0 && f->caster.n_launched_effects == 0);
    lw_state_dispose(&f->state);
    free(f);
    return 0;
}

static int aftereffect_ticks_while_invincible(void) {
    Fixture *f = fixture_new();
    CHECK(f != NULL);
    CHECK(add_effect(f, LW_EFFECT_TYPE_AFTEREFFECT, 3, &f->target) == 30);
    LwEffect *effect = f->target.effects[0];
    int life = f->target.life;
    int total_life = f->target.m_total_life;
    f->target.states |= 1u << LW_ENTITY_STATE_INVINCIBLE;
    lw_entity_start_turn(&f->target);
    CHECK(f->target.life == life && f->target.m_total_life == total_life);
    CHECK(effect->turns == 2 && effect->value == 30);
    f->target.states &= ~(1u << LW_ENTITY_STATE_INVINCIBLE);
    lw_entity_start_turn(&f->target);
    CHECK(f->target.life == life - 30 && effect->turns == 1);
    lw_state_dispose(&f->state);
    free(f);
    return 0;
}

typedef struct {
    LwDamageType poison;
    LwDamageType aftereffect;
    int poison_calls;
    int aftereffect_calls;
} DamageResults;

static void record_damage(LwStatisticsManager *manager, LwEntity *target,
                          LwEntity *attacker, int damage, LwDamageType type,
                          LwEffect *effect) {
    DamageResults *results = manager->user_data;
    (void)target;
    (void)attacker;
    (void)damage;
    if (!effect || type == LW_DAMAGE_TYPE_NOVA) return;
    if (effect->id == LW_EFFECT_TYPE_POISON) {
        results->poison = type;
        results->poison_calls++;
    } else if (effect->id == LW_EFFECT_TYPE_AFTEREFFECT) {
        results->aftereffect = type;
        results->aftereffect_calls++;
    }
}

static int periodic_damage_types(void) {
    Fixture *f = fixture_new();
    DamageResults results = {0};
    LwStatisticsManager manager = {0};
    CHECK(f != NULL);
    manager.user_data = &results;
    manager.damage = record_damage;
    lw_state_set_statistics_manager(&f->state, &manager);
    CHECK(add_effect(f, LW_EFFECT_TYPE_POISON, 1, &f->target) == 30);
    CHECK(add_effect(f, LW_EFFECT_TYPE_AFTEREFFECT, 1, &f->target) == 30);
    results.poison_calls = results.aftereffect_calls = 0;
    int start = f->state.actions.stream.n_entries;
    lw_entity_start_turn(&f->target);
    CHECK(results.poison_calls == 1 && results.aftereffect_calls == 1);
    CHECK(results.poison == LW_DAMAGE_TYPE_POISON);
    CHECK(results.aftereffect == LW_DAMAGE_TYPE_AFTEREFFECT);
    CHECK(results.poison != results.aftereffect);
    int damage_actions = 0;
    for (int i = start; i < f->state.actions.stream.n_entries; i++) {
        if (f->state.actions.stream.entries[i].type == LW_ACTION_POISON_DAMAGE) damage_actions++;
    }
    CHECK(damage_actions == 2);
    lw_state_dispose(&f->state);
    free(f);
    return 0;
}

int main(void) {
    CHECK(poison_ticks_on_target_turns() == 0);
    CHECK(self_and_infinite_poison() == 0);
    CHECK(heal_aftereffect_and_buff() == 0);
    CHECK(aftereffect_ticks_while_invincible() == 0);
    CHECK(periodic_damage_types() == 0);
    return 0;
}
