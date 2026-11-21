#include <stdio.h>
#include <stdlib.h>

#include "lw_effect.h"
#include "lw_entity.h"
#include "lw_fight.h"
#include "lw_generator.h"
#include "lw_map.h"
#include "lw_order.h"
#include "lw_actions.h"
#include "lw_statistics.h"
#include "lw_state.h"
#include "lw_weapon.h"

static int emit(const char *name, LwState *state, int before, const int *values, int n_values) {
    LwActionStream *stream = &state->actions.stream;
    if (lw_state_get_fatal_error(state)) {
        fprintf(stderr, "%s: %s\n", name, lw_state_get_fatal_error_message(state));
        return 0;
    }
    if (stream->n_entries >= LW_LOG_MAX_ENTRIES || stream->n_extra >= LW_LOG_MAX_EXTRA) {
        fprintf(stderr, "%s: action stream reached its capacity\n", name);
        return 0;
    }
    printf("C_PARITY {\"case\":\"%s\",\"values\":[", name);
    for (int i = 0; i < n_values; i++) printf("%s%d", i ? "," : "", values[i]);
    printf("],\"actions\":[");
    for (int i = before; i < stream->n_entries; i++) {
        LwActionLog *entry = &stream->entries[i];
        printf("%s[%d", i == before ? "" : ",", entry->type);
        for (int j = 0; j < entry->n_args; j++) printf(",%d", entry->v[j]);
        if (entry->extra_offset >= 0) {
            printf(",[");
            for (int j = 0; j < entry->extra_len; j++)
                printf("%s%d", j ? "," : "", stream->extra[entry->extra_offset + j]);
            printf("]");
        }
        printf("]");
    }
    printf("],\"ops\":{");
    for (int i = 0; i < lw_actions_get_operation_count(&state->actions); i++) {
        int entity_id;
        int64_t operations;
        if (!lw_actions_get_operation(&state->actions, i, &entity_id, &operations)) {
            fprintf(stderr, "%s: invalid operation metadata entry %d\n", name, i);
            return 0;
        }
        printf("%s\"%d\":%lld", i ? "," : "", entity_id, (long long)operations);
    }
    printf("}}\n");
    return 1;
}

static int effect(LwState *state, LwEntity *target, LwEntity *caster,
                  int id, int turns, double value, int modifiers) {
    return lw_effect_create_effect(state, id, turns, 1, value, 0, 0, target,
                                   caster, NULL, 0, 1, 0, 1, 0, modifiers);
}

static int superinfection(const char *name, int finite, int invincible) {
    LwState *state = calloc(1, sizeof(*state));
    LwEntity target, caster;
    if (!state) return 0;
    lw_state_init(state);
    lw_entity_init_id_name(&target, 1, "target");
    lw_entity_init_id_name(&caster, 2, "caster");
    target.life = target.m_total_life = caster.life = caster.m_total_life = 1000;
    lw_state_add_entity(state, 0, &target);
    lw_state_add_entity(state, 1, &caster);
    if (finite) {
        effect(state, &target, &caster, LW_EFFECT_TYPE_POISON, 2, 30, LW_MODIFIER_IRREDUCTIBLE);
        effect(state, &target, &caster, LW_EFFECT_TYPE_POISON, 1, 50, 0);
    }
    effect(state, &target, &caster, LW_EFFECT_TYPE_POISON, -1, 40, 0);
    if (invincible) effect(state, &target, &caster, LW_EFFECT_TYPE_ADD_STATE, -1, 3, 0);
    int before = state->actions.stream.n_entries;
    int first = effect(state, &target, &caster, LW_EFFECT_TYPE_SUPERINFECTION, 0, 25, 0);
    int second = effect(state, &target, &caster, LW_EFFECT_TYPE_SUPERINFECTION, 0, 25, 0);
    int values[] = {first, second, target.life, target.m_total_life,
                    target.n_effects, caster.n_launched_effects,
                    target.n_effects ? target.effects[0]->turns : -2};
    int ok = emit(name, state, before, values, (int)(sizeof(values) / sizeof(values[0])));
    lw_state_dispose(state);
    lw_entity_dispose(&target);
    lw_entity_dispose(&caster);
    free(state);
    return ok;
}

static int awaken(LwState *state, LwEntity *plant, LwEntity *trigger, void *userdata) {
    int *calls = userdata;
    (void)state;
    (void)plant;
    calls[0]++;
    calls[1] = trigger->cell->id;
    return 0;
}

static int plant_movement(const char *name, int movement) {
    LwState *state = calloc(1, sizeof(*state));
    LwMap *map = calloc(1, sizeof(*map));
    LwEntity target, caster, plant;
    int calls[] = {0, -1};
    if (!state || !map) {
        free(state);
        free(map);
        return 0;
    }
    lw_state_init(state);
    lw_map_init(map, 18, 18);
    state->map = map;
    map->state = state;
    lw_entity_init_id_name(&target, 1, "target");
    lw_entity_init_id_name(&caster, 2, "caster");
    lw_entity_init_id_name(&plant, 3, "plant");
    target.life = target.m_total_life = caster.life = caster.m_total_life = 1000;
    plant.life = plant.m_total_life = 100;
    lw_stats_set_stat(&target.m_base_stats, LW_STAT_MP, 20);
    lw_stats_set_stat(&plant.m_base_stats, LW_STAT_TP, 6);
    lw_state_add_entity(state, 0, &target);
    lw_state_add_entity(state, 1, &caster);
    lw_state_add_entity(state, 1, &plant);
    plant.awakening_zone = 1;
    plant.used_tp = 4;
    LwCell *start = lw_map_get_cell(map, 300);
    LwCell *path[4];
    path[0] = lw_cell_next(start, map, 1, 0);
    path[1] = lw_cell_next(path[0], map, 1, 0);
    path[2] = lw_cell_next(path[1], map, 1, 0);
    path[3] = lw_cell_next(path[2], map, 1, 0);
    lw_map_set_entity(map, &target, start);
    lw_map_set_entity(map, &plant, lw_cell_next(path[1], map, 0, 1));
    lw_state_set_plant_awakening(state, awaken, calls);
    int before = state->actions.stream.n_entries;
    int moved = 0;
    if (movement == 0) moved = lw_state_move_entity_path(state, &target, path, 4);
    if (movement == 1) lw_state_slide_entity(state, &target, path[3], &target);
    if (movement == 2) lw_state_teleport_entity(state, &target, path[1], &target, 0);
    if (movement == 3) lw_state_teleport_entity(state, &target, path[0], &target, 0);
    int values[] = {moved, target.cell->id, lw_entity_get_mp(&target),
                    lw_entity_get_tp(&plant), calls[0], calls[1]};
    int ok = emit(name, state, before, values, (int)(sizeof(values) / sizeof(values[0])));
    lw_state_dispose(state);
    lw_entity_dispose(&target);
    lw_entity_dispose(&caster);
    lw_entity_dispose(&plant);
    free(map);
    free(state);
    return ok;
}

typedef struct {
    int count;
    int entity_ids[2];
    int64_t operations[2];
} OperationData;

static int operation_count(LwStatisticsManager *manager) {
    OperationData *data = manager->user_data;
    return data->count;
}

static int operation_at(LwStatisticsManager *manager, int index, int *entity_id,
                        int64_t *operations) {
    OperationData *data = manager->user_data;
    if (index < 0 || index >= data->count) return 0;
    *entity_id = data->entity_ids[index];
    *operations = data->operations[index];
    return 1;
}

static int operations_metadata(void) {
    LwState *state = calloc(1, sizeof(*state));
    LwEntity first, second;
    OperationData data = {0};
    LwStatisticsManager statistics = {0};
    if (!state) return 0;
    lw_state_init(state);
    lw_entity_init_id_name(&first, 1, "first");
    lw_entity_init_id_name(&second, 2, "second");
    lw_state_add_entity(state, 0, &first);
    lw_state_add_entity(state, 1, &second);
    data.count = 2;
    data.entity_ids[0] = lw_entity_get_fid(&first);
    data.entity_ids[1] = lw_entity_get_fid(&second);
    data.operations[0] = 456;
    data.operations[1] = 789;
    statistics.user_data = &data;
    statistics.get_operations_by_entity_count = operation_count;
    statistics.get_operations_by_entity_at = operation_at;
    int rc = lw_actions_add_ops_and_times(&state->actions, &statistics);
    int values[] = {rc, lw_actions_get_operation_count(&state->actions)};
    int ok = rc == 0 &&
        emit("operations_metadata", state, 0, values, (int)(sizeof(values) / sizeof(values[0])));
    lw_state_dispose(state);
    lw_entity_dispose(&first);
    lw_entity_dispose(&second);
    free(state);
    return ok;
}

static int raw_shackle_strength(void) {
    LwState *state = calloc(1, sizeof(*state));
    LwEntity target, caster;
    if (!state) return 0;
    lw_state_init(state);
    lw_entity_init_id_name(&target, 1, "target");
    lw_entity_init_id_name(&caster, 2, "caster");
    target.life = target.m_total_life = 1000;
    caster.life = caster.m_total_life = 1000;
    lw_stats_set_stat(&target.m_base_stats, LW_STAT_STRENGTH, 200);
    lw_stats_set_stat(&caster.m_base_stats, LW_STAT_MAGIC, 200);
    lw_state_add_entity(state, 0, &target);
    lw_state_add_entity(state, 1, &caster);
    int before = state->actions.stream.n_entries;
    int amount = lw_effect_create_effect(
        state, LW_EFFECT_TYPE_RAW_SHACKLE_STRENGTH, 3, 1, 100, 0, 0,
        &target, &caster, NULL, 0, 1, 0, 1, 0, 0);
    int values[] = {amount, lw_entity_get_strength(&target),
                    target.n_effects, caster.n_launched_effects};
    int ok = emit("raw_shackle_strength", state, before, values,
                  (int)(sizeof(values) / sizeof(values[0])));
    lw_state_dispose(state);
    lw_entity_dispose(&target);
    lw_entity_dispose(&caster);
    free(state);
    return ok;
}

static int damage_to_resistance(const char *name, int modifiers) {
    LwState *state = calloc(1, sizeof(*state));
    LwEntity target, caster;
    LwWeapon weapon;
    LwEffectParameters passive;
    if (!state) return 0;
    lw_state_init(state);
    lw_entity_init_id_name(&target, 1, "target");
    lw_entity_init_id_name(&caster, 2, "caster");
    target.life = target.m_total_life = caster.life = caster.m_total_life = 1000;
    lw_state_add_entity(state, 0, &target);
    lw_state_add_entity(state, 1, &caster);
    lw_effect_parameters_init(&passive, LW_EFFECT_TYPE_DAMAGE_TO_RESISTANCE,
                              25, 0, 2, 0, modifiers);
    lw_weapon_init(&weapon, 123, 1, 1, 1, NULL, 0, 1, 1, 1, 123,
                   "test", &passive, 1, 0);
    lw_entity_add_weapon(&target, &weapon);
    int before = state->actions.stream.n_entries;
    lw_entity_on_direct_damage(&target, 100);
    int first = lw_entity_get_resistance(&target);
    lw_entity_on_direct_damage(&target, 21);
    int second = lw_entity_get_resistance(&target);
    int count = target.n_effects;
    lw_entity_start_turn(&target);
    lw_entity_start_turn(&target);
    int values[] = {first, second, count, lw_entity_get_resistance(&target),
                    target.n_effects};
    int ok = emit(name, state, before, values, (int)(sizeof(values) / sizeof(values[0])));
    lw_state_dispose(state);
    lw_entity_dispose(&target);
    lw_entity_dispose(&caster);
    free(state);
    return ok;
}

static int periodic_turns(const char *name, int effect_type, int initial_life) {
    LwState *state = calloc(1, sizeof(*state));
    LwEntity target, caster;
    if (!state) return 0;
    lw_state_init(state);
    lw_entity_init_id_name(&target, 1, "target");
    lw_entity_init_id_name(&caster, 2, "caster");
    target.life = initial_life;
    target.m_total_life = caster.life = caster.m_total_life = 1000;
    lw_state_add_entity(state, 0, &target);
    lw_state_add_entity(state, 1, &caster);
    effect(state, &target, &caster, effect_type, 2, 30, 0);
    int before = state->actions.stream.n_entries;
    lw_entity_start_turn(&caster);
    if (target.n_effects == 0) {
        fprintf(stderr, "%s: effect disappeared before target turn\n", name);
        lw_state_dispose(state);
        lw_entity_dispose(&target);
        lw_entity_dispose(&caster);
        free(state);
        return 0;
    }
    int after_caster = target.life, first_turns = target.effects[0]->turns;
    lw_entity_start_turn(&target);
    if (target.n_effects == 0) {
        fprintf(stderr, "%s: effect disappeared after first target turn\n", name);
        lw_state_dispose(state);
        lw_entity_dispose(&target);
        lw_entity_dispose(&caster);
        free(state);
        return 0;
    }
    int after_first = target.life, second_turns = target.effects[0]->turns;
    lw_entity_start_turn(&caster);
    lw_entity_start_turn(&target);
    int values[] = {after_caster, first_turns, after_first, second_turns,
                    target.life, target.n_effects, caster.n_launched_effects};
    int ok = emit(name, state, before, values, (int)(sizeof(values) / sizeof(values[0])));
    lw_state_dispose(state);
    lw_entity_dispose(&target);
    lw_entity_dispose(&caster);
    free(state);
    return ok;
}

static int start_effects_resurrection(void) {
    LwState *state = calloc(1, sizeof(*state));
    LwEntity target, caster;
    if (!state) return 0;
    lw_state_init(state);
    state->seed = 1;
    state->context = LW_CONTEXT_TEST;
    lw_entity_init_id_name(&target, 1, "target");
    lw_entity_init_id_name(&caster, 2, "caster");
    target.life = target.m_total_life = 1000;
    caster.life = caster.m_total_life = 1000;
    lw_stats_set_stat(&target.m_base_stats, LW_STAT_STRENGTH, 200);
    lw_entity_add_start_effect(&target, LW_EFFECT_TYPE_RAW_SHACKLE_STRENGTH, 50);
    lw_state_add_entity(state, 0, &target);
    lw_state_add_entity(state, 1, &caster);
    lw_state_init_fight(state);
    lw_state_record_initial_state(state);
    if (lw_state_get_fatal_error(state) || target.cell == NULL) {
        fprintf(stderr, "start_effects_resurrection: fight initialization failed\n");
        lw_state_dispose(state);
        lw_entity_dispose(&target);
        lw_entity_dispose(&caster);
        free(state);
        return 0;
    }
    int before = state->actions.stream.n_entries - 1;
    int initial_strength = lw_entity_get_strength(&target);
    LwCell *cell = target.cell;
    lw_state_on_player_die(state, &target, &caster, NULL);
    int dead_strength = lw_entity_get_strength(&target);
    int dead_effects = target.n_effects;
    lw_state_resurrect(state, &caster, &target, cell, 0, 1);
    int values[] = {initial_strength, dead_strength, dead_effects,
                    lw_entity_get_strength(&target), target.n_effects};
    int ok = emit("start_effects_resurrection", state, before, values,
                  (int)(sizeof(values) / sizeof(values[0])));
    lw_state_dispose(state);
    lw_entity_dispose(&target);
    lw_entity_dispose(&caster);
    free(state);
    return ok;
}

static int idle_turn_flow(void) {
    LwState *state = calloc(1, sizeof(*state));
    LwEntity target, caster;
    LwGenerator generator;
    LwFight fight;
    if (!state) return 0;
    lw_state_init(state);
    lw_generator_init(&generator);
    lw_entity_init_id_name(&target, 1, "target");
    lw_entity_init_id_name(&caster, 2, "caster");
    LwEntity *entities[] = {&target, &caster};
    for (int i = 0; i < 2; i++) {
        LwEntity *entity = entities[i];
        entity->life = entity->m_total_life = 1000;
        lw_stats_set_stat(&entity->m_base_stats, LW_STAT_LIFE, 1000);
        lw_stats_set_stat(&entity->m_base_stats, LW_STAT_TP, 6);
        lw_stats_set_stat(&entity->m_base_stats, LW_STAT_MP, 20);
        lw_stats_set_stat(&entity->m_base_stats, LW_STAT_FREQUENCY, 10);
    }
    lw_state_add_entity(state, 0, &target);
    lw_state_add_entity(state, 1, &caster);
    lw_state_seed(state, 1);
    lw_fight_init(&fight, &generator);
    lw_fight_set_state(&fight, state);
    if (lw_fight_init_fight(&fight) != 0 || lw_state_get_fatal_error(state)) {
        fprintf(stderr, "idle_turn_flow: fight initialization failed\n");
        lw_state_dispose(state);
        lw_entity_dispose(&target);
        lw_entity_dispose(&caster);
        free(state);
        return 0;
    }
    lw_state_record_initial_state(state);
    int before = state->actions.stream.n_entries;
    if (lw_fight_start_turn(&fight) != 0 || lw_fight_start_turn(&fight) != 0) {
        fprintf(stderr, "idle_turn_flow: turn execution failed\n");
        lw_state_dispose(state);
        lw_entity_dispose(&target);
        lw_entity_dispose(&caster);
        free(state);
        return 0;
    }
    struct LwEntity *current = lw_order_current(&state->order);
    int values[] = {lw_order_get_turn(&state->order),
                    current ? lw_entity_get_fid(current) : -1,
                    target.life, caster.life};
    int ok = emit("idle_turn_flow", state, before, values,
                  (int)(sizeof(values) / sizeof(values[0])));
    lw_state_dispose(state);
    lw_entity_dispose(&target);
    lw_entity_dispose(&caster);
    free(state);
    return ok;
}

static int turret_start_fight(void) {
    LwState *state = calloc(1, sizeof(*state));
    LwEntity target, caster, turret;
    LwGenerator generator;
    LwFight fight;
    if (!state) return 0;
    lw_state_init(state);
    lw_generator_init(&generator);
    lw_entity_init_id_name(&target, 1, "target");
    lw_entity_init_id_name(&caster, 2, "caster");
    lw_entity_init_id_name(&turret, 3, "");
    turret.type = LW_ENTITY_TYPE_TURRET;
    target.life = target.m_total_life = caster.life = caster.m_total_life = 1000;
    lw_state_add_entity(state, 0, &target);
    lw_state_add_entity(state, 1, &caster);
    lw_state_add_entity(state, 0, &turret);
    lw_state_seed(state, 1);
    lw_fight_init(&fight, &generator);
    lw_fight_set_state(&fight, state);
    if (lw_fight_init_fight(&fight) != 0 || lw_state_get_fatal_error(state)) {
        fprintf(stderr, "turret_start_fight: fight initialization failed\n");
        lw_state_dispose(state);
        lw_entity_dispose(&target);
        lw_entity_dispose(&caster);
        lw_entity_dispose(&turret);
        free(state);
        return 0;
    }
    int before = state->actions.stream.n_entries;
    lw_entity_start_fight(&turret);
    int values[] = {before, lw_entity_has_state(&turret, LW_ENTITY_STATE_STATIC)};
    int ok = emit("turret_start_fight", state, before, values, 2);
    lw_state_record_initial_state(state);
    lw_state_dispose(state);
    lw_entity_dispose(&target);
    lw_entity_dispose(&caster);
    lw_entity_dispose(&turret);
    free(state);
    return ok;
}

int main(void) {
    if (!superinfection("superinfection_mixed", 1, 0) ||
        !superinfection("superinfection_infinite", 0, 0) ||
        !superinfection("superinfection_invincible", 1, 1) ||
        !plant_movement("plant_walk", 0) ||
        !plant_movement("plant_slide", 1) ||
        !plant_movement("plant_teleport", 2) ||
        !plant_movement("plant_no_entry", 3) ||
        !operations_metadata() ||
        !raw_shackle_strength() ||
        !damage_to_resistance("damage_to_resistance_replace", 0) ||
        !damage_to_resistance("damage_to_resistance_stack", LW_MODIFIER_STACKABLE) ||
        !periodic_turns("periodic_poison", LW_EFFECT_TYPE_POISON, 1000) ||
        !periodic_turns("periodic_aftereffect", LW_EFFECT_TYPE_AFTEREFFECT, 1000) ||
        !periodic_turns("periodic_heal", LW_EFFECT_TYPE_HEAL, 500) ||
        !start_effects_resurrection() ||
        !idle_turn_flow() ||
        !turret_start_fight()) {
        fputs("C parity probe failed\n", stderr);
        return 1;
    }
    return 0;
}
