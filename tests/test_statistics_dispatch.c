#include <stdio.h>
#include <stdlib.h>

#include "lw_entity.h"
#include "lw_effect.h"
#include "lw_fight.h"
#include "lw_actions.h"
#include "lw_state.h"

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "line %d: %s\n", __LINE__, #condition); \
        return 1; \
    } \
} while (0)

typedef struct {
    LwEntity *target;
    LwEntity *attacker;
    int calls;
    int values[2];
    LwDamageType types[2];
    int tp;
    int mp;
    int healed;
    int characteristics;
    int initialized;
    int times_calls;
    int64_t elapsed_ns;
    int64_t operations;
    int errors;
    int end_fight_calls;
    int listener_calls;
    LwFight *listener_fight;
    int operation_count;
    int operation_ids[2];
    int64_t operation_values[2];
} Results;

static void listener_new_turn(LwFightListener *listener, LwFight *fight) {
    Results *r = listener->user_data;
    r->listener_calls++;
    r->listener_fight = fight;
}

static void init_entity(LwStatisticsManager *m, LwEntity *entity) {
    Results *r = m->user_data;
    if (entity == r->target) r->initialized++;
}

static void damage(LwStatisticsManager *m, LwEntity *target, LwEntity *attacker,
                   int amount, LwDamageType type, LwEffect *effect) {
    Results *r = m->user_data;
    if (target != r->target || attacker != r->attacker || effect != NULL || r->calls >= 2) {
        r->calls = -100;
        return;
    }
    r->values[r->calls] = amount;
    r->types[r->calls++] = type;
}

static void use_tp(LwStatisticsManager *m, int tp) {
    Results *r = m->user_data;
    r->tp += tp;
}

static void use_mp(LwStatisticsManager *m, int mp) {
    Results *r = m->user_data;
    r->mp += mp;
}

static void heal(LwStatisticsManager *m, LwEntity *healer, LwEntity *target, int amount) {
    Results *r = m->user_data;
    r->healed = healer == r->attacker && target == r->target ? amount : -1;
}

static void characteristics(LwStatisticsManager *m, LwEntity *target) {
    Results *r = m->user_data;
    if (target == r->target) r->characteristics++;
}

static void add_times(LwStatisticsManager *m, LwEntity *entity, int64_t elapsed_ns,
                      int64_t operations) {
    Results *r = m->user_data;
    if (entity == r->target) {
        r->times_calls++;
        r->elapsed_ns = elapsed_ns;
        r->operations = operations;
    }
}

static void error(LwStatisticsManager *m, LwEntity *entity) {
    Results *r = m->user_data;
    if (entity == r->target) r->errors++;
}

static void end_fight(LwStatisticsManager *m, LwEntity **entities, int count) {
    Results *r = m->user_data;
    if (count == 2 && entities[0] == r->target && entities[1] == r->attacker)
        r->end_fight_calls++;
}

static int operation_count(LwStatisticsManager *m) {
    Results *r = m->user_data;
    return r->operation_count;
}

static int operation_at(LwStatisticsManager *m, int index, int *entity_id,
                        int64_t *operations) {
    Results *r = m->user_data;
    if (index < 0 || index >= r->operation_count) return 0;
    *entity_id = r->operation_ids[index];
    *operations = r->operation_values[index];
    return 1;
}

int main(void) {
    LwState *state = calloc(1, sizeof(*state));
    LwEntity target, attacker;
    LwFight fight = {0};
    LwStatisticsManager manager = {0};
    LwActions actions, copied_actions;
    Results results = {0};
    CHECK(state != NULL);
    lw_state_init(state);
    lw_entity_init_id_name(&target, 1, "target");
    lw_entity_init_id_name(&attacker, 2, "attacker");
    target.state = state;
    target.life = target.m_total_life = 100;
    results.target = &target;
    results.attacker = &attacker;
    manager.user_data = &results;
    manager.damage = damage;
    manager.use_tp = use_tp;
    manager.use_mp = use_mp;
    manager.heal = heal;
    manager.characteristics = characteristics;
    manager.init = init_entity;
    manager.add_times = add_times;
    manager.error = error;
    manager.end_fight = end_fight;
    manager.get_operations_by_entity_count = operation_count;
    manager.get_operations_by_entity_at = operation_at;
    results.operation_count = 2;
    results.operation_ids[0] = 17;
    results.operation_ids[1] = 23;
    results.operation_values[0] = 456;
    results.operation_values[1] = 789;
    lw_state_set_statistics_manager(state, &manager);

    lw_actions_init(&actions);
    CHECK(lw_actions_add_ops_and_times(&actions, &manager) == 0);
    CHECK(lw_actions_get_operation_count(&actions) == 2);
    int operation_id = -1;
    int64_t operations = -1;
    CHECK(lw_actions_get_operation(&actions, 1, &operation_id, &operations));
    CHECK(operation_id == 23 && operations == 789);
    lw_actions_copy(&copied_actions, &actions);
    CHECK(lw_actions_get_operation_count(&copied_actions) == 2);
    CHECK(lw_actions_get_operation(&copied_actions, 0, &operation_id, &operations));
    CHECK(operation_id == 17 && operations == 456);
    results.operation_count = LW_ACTIONS_MAX_OP_ENTRIES + 1;
    CHECK(lw_actions_add_ops_and_times(&actions, &manager) != 0);
    CHECK(lw_actions_get_operation_count(&actions) == 2);

    lw_stats_init_entity(&manager, &target);
    lw_stats_characteristics(&manager, &target);
    lw_stats_add_times(&manager, &target, 1234, 56);
    lw_stats_error(&manager, &target);
    LwEntity *end_entities[] = {&target, &attacker};
    lw_stats_end_fight(&manager, end_entities, 2);
    lw_stats_set_generator_fight(&manager, &fight);
    CHECK(results.initialized == 1 && results.characteristics == 1);
    CHECK(results.times_calls == 1 && results.elapsed_ns == 1234 && results.operations == 56);
    CHECK(results.errors == 1 && results.end_fight_calls == 1);
    CHECK(manager.generator_fight == &fight);
    LwFightListener listener = {listener_new_turn, &results};
    lw_fight_listener_new_turn(&listener, &fight);
    CHECK(results.listener_calls == 1 && results.listener_fight == &fight);

    lw_entity_remove_life(&target, 20, 3, &attacker, LW_DAMAGE_TYPE_DIRECT, NULL, NULL);
    CHECK(results.calls == 2);
    CHECK(results.values[0] == 20 && results.types[0] == LW_DAMAGE_TYPE_DIRECT);
    CHECK(results.values[1] == 3 && results.types[1] == LW_DAMAGE_TYPE_NOVA);
    lw_entity_use_tp(&target, 4);
    lw_entity_use_mp(&target, 2);
    CHECK(results.tp == 4 && results.mp == 2);
    lw_entity_add_life(&target, &attacker, 10);
    CHECK(results.healed == 10 && results.characteristics == 2);
    lw_state_dispose(state);
    free(state);
    return 0;
}
