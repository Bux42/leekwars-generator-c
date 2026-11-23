/* Step-by-step execution against the callback loop, on a scripted fight. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lw_chip.h"
#include "lw_effect_params.h"
#include "lw_entity.h"
#include "lw_fight.h"
#include "lw_generator.h"
#include "lw_outcome.h"
#include "lw_scenario.h"
#include "lw_state.h"

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "line %d: %s\n", __LINE__, #condition); \
        return 1; \
    } \
} while (0)

typedef struct {
    LwState state;
    LwFight fight;
    LwOutcome outcome;
} Session;

static LwChip damage_chip;

/* Walk to the other entity and strike it. One action per call, 0 when done. */
static int act(LwFight *fight, LwEntity *actor, int step) {
    LwState *state = lw_fight_get_state(fight);
    LwEntity *enemy = state->m_entities[state->m_entities[0] == actor ? 1 : 0];
    if (lw_entity_is_dead(enemy) || step > 3) return 0;
    if (step == 0) {
        lw_state_move_toward(state, actor, lw_entity_get_fid(enemy), -1);
        return 1;
    }
    return lw_fight_use_chip(fight, actor, lw_entity_get_cell(enemy), &damage_chip) > 0;
}

static int dispatch(LwFight *fight, LwEntity *actor, void *ai_file, int turn, void *userdata) {
    int step = 0;
    (void)ai_file; (void)turn; (void)userdata;
    while (act(fight, actor, step)) step++;
    return 0;
}

static void play(LwFight *fight, int step) {
    LwEntity *actor = lw_fight_step_current(fight);
    while (!lw_fight_step_is_over(fight)) {
        if (!actor) {
            if (lw_fight_step_begin_turn(fight, &actor)) exit(2);
            step = 0;
            if (!actor) continue;
        }
        while (act(fight, actor, step)) step++;
        if (lw_fight_step_end_turn(fight)) exit(3);
        actor = NULL;
    }
    if (lw_fight_step_finish(fight, 0)) exit(4);
}

static int same(Session *a, Session *b) {
    const LwActionStream *x = &a->state.actions.stream, *y = &b->state.actions.stream;
    CHECK(a->state.rng_n == b->state.rng_n);
    CHECK(a->fight.m_winteam == b->fight.m_winteam);
    CHECK(a->state.order.turn == b->state.order.turn);
    CHECK(a->state.n_entities == b->state.n_entities);
    for (int i = 0; i < a->state.n_entities; i++)
        CHECK(a->state.m_entities[i]->life == b->state.m_entities[i]->life);
    CHECK(x->n_entries == y->n_entries && x->n_extra == y->n_extra);
    CHECK(!memcmp(x->extra, y->extra, (size_t)x->n_extra * sizeof(x->extra[0])));
    for (int i = 0; i < x->n_entries; i++) {
        CHECK(x->entries[i].type == y->entries[i].type);
        CHECK(x->entries[i].n_args == y->entries[i].n_args);
        CHECK(!memcmp(x->entries[i].v, y->entries[i].v,
                      (size_t)x->entries[i].n_args * sizeof(int)));
    }
    return 0;
}

static int run(int seed, int life, int max_turns) {
    Session *loop = calloc(1, sizeof(*loop)), *step = calloc(1, sizeof(*step));
    Session *copy = calloc(1, sizeof(*copy));
    LwScenario *scenario = malloc(sizeof(*scenario));
    LwGenerator generator;
    LwEntityInfo info;
    LwEntity *actor, *other;
    CHECK(loop && step && copy && scenario);
    lw_scenario_init(scenario);
    scenario->seed = seed;
    scenario->max_turns = max_turns;
    for (int team = 0; team < 2; team++) {
        lw_entity_info_init(&info);
        info.id = team + 1;
        info.life = life;
        info.tp = 10;
        info.mp = 5;
        info.agility = 200;
        info.ram = 6;
        info.level = 100;
        info.chips[0] = 9700;
        info.n_chips = 1;
        CHECK(lw_scenario_add_entity(scenario, team, &info));
    }

    lw_generator_init(&generator);
    lw_generator_set_ai_dispatch(&generator, dispatch, NULL);
    CHECK(!lw_generator_run_scenario(&generator, scenario, NULL, NULL, NULL,
                                     &loop->state, &loop->fight, &loop->outcome));

    lw_generator_init(&generator);
    CHECK(!lw_generator_prepare_scenario(&generator, scenario, NULL, NULL, NULL,
                                         &step->state, &step->fight, &step->outcome));
    CHECK(!lw_fight_step_start(&step->fight));
    CHECK(!lw_fight_step_current(&step->fight));
    CHECK(lw_fight_step_end_turn(&step->fight) == -1);
    CHECK(!lw_fight_step_begin_turn(&step->fight, &actor) && actor);
    CHECK(lw_fight_step_current(&step->fight) == actor);
    CHECK(lw_fight_step_begin_turn(&step->fight, &other) == -1 && !other);
    CHECK(lw_fight_step_finish(&step->fight, 0) == -1);

    /* Suspend in the middle of the turn; the clone must be independent. */
    CHECK(act(&step->fight, actor, 0));
    CHECK(lw_fight_step_clone(&copy->fight, &copy->state, &step->fight));
    CHECK(lw_fight_step_current(&copy->fight) != actor);
    CHECK(lw_entity_get_fid(lw_fight_step_current(&copy->fight)) == lw_entity_get_fid(actor));
    play(&copy->fight, 1);
    CHECK(same(loop, copy) == 0);
    CHECK(lw_fight_step_current(&step->fight) == actor && step->state.order.turn == 1);
    play(&step->fight, 1);
    CHECK(same(loop, step) == 0);
    CHECK(lw_fight_step_begin_turn(&step->fight, &actor) == -1);

    lw_state_dispose(&loop->state);
    lw_state_dispose(&step->state);
    lw_state_dispose(&copy->state);
    lw_scenario_dispose(scenario);
    free(loop);
    free(step);
    free(copy);
    free(scenario);
    return 0;
}

int main(void) {
    LwEffectParameters effect;
    lw_effect_parameters_init(&effect, LW_EFFECT_TYPE_DAMAGE, 20, 10, 0,
                              LW_TARGET_ENEMIES | LW_TARGET_NON_SUMMONS, 0);
    lw_chip_init(&damage_chip, 9700, 3, 1, 6, &effect, 1, LW_ATTACK_LAUNCH_TYPE_CIRCLE,
                 1, 1, 0, 0, 0, 1, 9700, "damage", LW_CHIP_TYPE_DAMAGE, -1);
    CHECK(lw_chips_add_chip(&damage_chip));
    for (int seed = 1; seed <= 50; seed++) {
        CHECK(run(seed, 300, 64) == 0);     /* fought to the death */
        CHECK(run(seed, 100000, 64) == 0);  /* draw at the turn limit */
        CHECK(run(seed, 100000, 3) == 0);   /* shorter limit */
    }
    lw_chips_clear();
    return 0;
}
