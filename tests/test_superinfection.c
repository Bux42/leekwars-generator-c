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

static int effect(Fixture *f, int id, int turns, double value,
                  int critical, int modifiers) {
    return lw_effect_create_effect(&f->state, id, turns, 1, value, 0,
                                   critical, &f->target, &f->caster, NULL,
                                   0, 1, 0, 1, 0, modifiers);
}

static int finite_and_infinite_poisons(void) {
    Fixture *f = fixture_new();
    CHECK(f != NULL);
    CHECK(effect(f, LW_EFFECT_TYPE_POISON, 2, 30, 0, LW_MODIFIER_IRREDUCTIBLE) == 30);
    CHECK(effect(f, LW_EFFECT_TYPE_POISON, 1, 50, 0, 0) == 50);
    CHECK(effect(f, LW_EFFECT_TYPE_POISON, -1, 40, 0, 0) == 40);
    LwEffect *first = f->target.effects[0];
    LwEffect *infinite = f->target.effects[2];
    int actions = f->state.actions.stream.n_entries;
    CHECK(effect(f, LW_EFFECT_TYPE_SUPERINFECTION, 0, 25, 0, 0) == 20);
    CHECK(f->target.life == 980 && f->target.m_total_life == 998);
    CHECK(f->target.n_effects == 2 && f->caster.n_launched_effects == 2);
    CHECK(first->turns == 1 && first->value == 30);
    CHECK(infinite->turns == -1 && infinite->value == 40);
    CHECK(f->state.actions.stream.n_entries == actions + 3);
    CHECK(f->state.actions.stream.entries[actions].type == LW_ACTION_UPDATE_EFFECT_TURNS);
    CHECK(f->state.actions.stream.entries[actions].v[0] == first->log_id);
    CHECK(f->state.actions.stream.entries[actions].v[1] == 1);
    CHECK(f->state.actions.stream.entries[actions + 1].type == LW_ACTION_REMOVE_EFFECT);
    CHECK(f->state.actions.stream.entries[actions + 2].type == LW_ACTION_POISON_DAMAGE);
    CHECK(effect(f, LW_EFFECT_TYPE_SUPERINFECTION, 0, 25, 0, 0) == 8);
    CHECK(f->target.life == 972 && f->target.n_effects == 1);
    CHECK(f->caster.n_launched_effects == 1 && infinite->turns == -1);
    CHECK(effect(f, LW_EFFECT_TYPE_SUPERINFECTION, 0, 25, 0, 0) == 0);
    lw_state_dispose(&f->state);
    free(f);
    return 0;
}

static int critical_and_invincibility(void) {
    Fixture *f = fixture_new();
    CHECK(f != NULL);
    CHECK(effect(f, LW_EFFECT_TYPE_POISON, 3, 200, 0, 0) == 200);
    CHECK(effect(f, LW_EFFECT_TYPE_SUPERINFECTION, 0, 25, 1, 0) == 65);
    CHECK(f->target.life == 935 && f->target.m_total_life == 987);
    f->target.states |= 1u << LW_ENTITY_STATE_INVINCIBLE;
    CHECK(effect(f, LW_EFFECT_TYPE_SUPERINFECTION, 0, 25, 0, 0) == 0);
    CHECK(f->target.life == 935 && f->target.effects[0]->turns == 1);
    f->target.states &= ~(1u << LW_ENTITY_STATE_INVINCIBLE);
    CHECK(effect(f, LW_EFFECT_TYPE_SUPERINFECTION, 0, 200, 0, 0) == 200);
    CHECK(f->target.life == 735 && f->target.n_effects == 0);
    lw_state_dispose(&f->state);
    free(f);
    return 0;
}

int main(void) {
    CHECK(finite_and_infinite_poisons() == 0);
    CHECK(critical_and_invincibility() == 0);
    return 0;
}
