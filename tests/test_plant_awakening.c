#include <stdio.h>
#include <stdlib.h>

#include "lw_bulb_template.h"
#include "lw_chip.h"
#include "lw_fight.h"
#include "lw_map.h"
#include "lw_state.h"

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "line %d: %s\n", __LINE__, #condition); \
        return 1; \
    } \
} while (0)

typedef struct {
    int calls;
    int trigger_fid;
    int trigger_cell;
    int can_act;
    int kill_trigger;
} Results;

static int awakening(LwState *state, LwEntity *plant, LwEntity *trigger, void *userdata) {
    Results *results = userdata;
    results->calls++;
    results->trigger_fid = trigger->fight_id;
    results->trigger_cell = trigger->cell->id;
    results->can_act = lw_state_can_act(state, plant);
    if (results->kill_trigger) trigger->life = 0;
    return 0;
}

static int movement_and_clone(void) {
    LwState *state = calloc(1, sizeof(*state));
    LwState *clone = calloc(1, sizeof(*clone));
    LwMap *map = calloc(1, sizeof(*map));
    LwEntity mover, plant;
    LwChip cooldown_chip;
    Results results = {0};
    CHECK(state && clone && map);
    lw_state_init(state);
    lw_map_init(map, 18, 18);
    map->state = state;
    state->map = map;
    lw_entity_init_id_name(&mover, 1, "mover");
    lw_entity_init_id_name(&plant, 2, "plant");
    mover.life = mover.m_total_life = plant.life = plant.m_total_life = 100;
    lw_stats_set_stat(&mover.m_base_stats, LW_STAT_MP, 20);
    lw_stats_set_stat(&plant.m_base_stats, LW_STAT_TP, 6);
    lw_state_add_entity(state, 0, &mover);
    lw_state_add_entity(state, 1, &plant);
    lw_order_add_entity(&state->order, &mover);
    plant.type = LW_ENTITY_TYPE_PLANT;
    plant.awakening_zone = 1;
    plant.has_summon_ai = 1;
    plant.summon_ai_id = 42;
    plant.used_tp = 4;
    lw_chip_init(&cooldown_chip, 9502, 1, 1, 1, NULL, 0, 7, 1, 0,
                 0, 0, 0, 1, 9502, "cooldown", 1, -1);
    lw_entity_add_cooldown(&plant, &cooldown_chip, 3);
    lw_entity_start_turn(&plant);
    CHECK(lw_entity_get_cooldown(&plant, 9502) == 3);
    lw_state_set_plant_awakening(state, awakening, &results);

    LwCell *start = lw_map_get_cell(map, 300);
    LwCell *path[4];
    path[0] = lw_cell_next(start, map, 1, 0);
    path[1] = lw_cell_next(path[0], map, 1, 0);
    path[2] = lw_cell_next(path[1], map, 1, 0);
    path[3] = lw_cell_next(path[2], map, 1, 0);
    LwCell *plant_cell = lw_cell_next(path[1], map, 0, 1);
    CHECK(path[0] && path[1] && path[2] && path[3] && plant_cell);
    lw_map_set_entity(map, &mover, start);
    lw_map_set_entity(map, &plant, plant_cell);
    CHECK(lw_state_move_entity_path(state, &mover, path, 4) == 4);
    CHECK(results.calls == 1 && results.trigger_fid == mover.fight_id);
    CHECK(results.trigger_cell == path[1]->id && results.can_act);
    CHECK(lw_entity_get_cooldown(&plant, 9502) == 2);
    CHECK(mover.cell == path[3] && mover.used_mp == 4 && plant.used_tp == 0);
    CHECK(state->actions.stream.n_entries == 4);
    CHECK(state->actions.stream.entries[0].type == LW_ACTION_MOVE_TO);
    CHECK(state->actions.stream.entries[1].type == LW_ACTION_PLANT_AWAKE);
    CHECK(state->actions.stream.entries[1].v[0] == plant.fight_id);
    CHECK(state->actions.stream.entries[1].v[1] == mover.fight_id);
    CHECK(state->actions.stream.entries[1].v[2] == 6);
    CHECK(state->actions.stream.entries[2].type == LW_ACTION_PLANT_ASLEEP);
    CHECK(state->actions.stream.entries[3].type == LW_ACTION_MOVE_TO);
    CHECK(!lw_state_can_act(state, &plant));

    lw_state_teleport_entity(state, &mover, start, &mover, 0);
    lw_state_teleport_entity(state, &mover, path[1], &mover, 0);
    CHECK(results.calls == 1);
    lw_state_clear_plant_triggers(state, &mover);
    lw_state_teleport_entity(state, &mover, start, &mover, 0);
    lw_state_slide_entity(state, &mover, path[3], &mover);
    CHECK(results.calls == 2 && results.trigger_cell == path[3]->id);
    CHECK(lw_entity_get_cooldown(&plant, 9502) == 1);

    CHECK(lw_state_clone(clone, state));
    LwEntity *clone_mover = clone->m_entities[0];
    LwEntity *clone_plant = clone->m_entities[1];
    CHECK(clone_plant->awakened_by != plant.awakened_by);
    CHECK(clone_plant->summon_ai_id == 42);
    lw_state_clear_plant_triggers(clone, clone_mover);
    CHECK(plant.n_awakened_by == 1 && clone_plant->n_awakened_by == 0);
    lw_state_check_plant_triggers(clone, clone_mover,
                                  lw_map_get_cell(clone->map, start->id),
                                  lw_map_get_cell(clone->map, path[1]->id));
    CHECK(lw_state_get_fatal_error(clone) == LW_STATE_ERROR_PLANT_DISPATCH);
    CHECK(results.calls == 2 && plant.n_awakened_by == 1);
    lw_state_dispose(clone);

    lw_state_clear_plant_triggers(state, &mover);
    lw_map_move_entity(map, &mover, start);
    mover.used_mp = 0;
    results.kill_trigger = 1;
    CHECK(lw_state_move_entity_path(state, &mover, path, 4) == 2);
    CHECK(lw_entity_is_dead(&mover) && mover.cell == path[1]);
    CHECK(mover.used_mp == 4 && results.calls == 3);
    CHECK(!lw_entity_has_cooldown(&plant, 9502));
    lw_state_dispose(state);
    lw_entity_dispose(&mover);
    lw_entity_dispose(&plant);
    free(map);
    free(clone);
    free(state);
    return 0;
}

static int planted_and_no_ai(int colossus) {
    LwState *state = calloc(1, sizeof(*state));
    LwMap *map = calloc(1, sizeof(*map));
    LwEntity owner, other;
    LwFight fight;
    LwBulbTemplate template_;
    LwChip chip;
    LwEffectParameters effect;
    CHECK(state && map);
    lw_state_init(state);
    lw_map_init(map, 18, 18);
    state->map = map;
    map->state = state;
    lw_entity_init_id_name(&owner, 1, "owner");
    lw_entity_init_id_name(&other, 2, "other");
    owner.life = owner.m_total_life = 100;
    other.life = other.m_total_life = 100;
    lw_stats_set_stat(&owner.m_base_stats, LW_STAT_TP, 6);
    lw_state_add_entity(state, colossus ? 1 : 0, &owner);
    lw_state_add_entity(state, colossus ? 0 : 1, &other);
    if (colossus) {
        state->type = LW_FIGHT_TYPE_COLOSSUS;
        state->colossus_multiplier = 3;
    }
    lw_order_add_entity(&state->order, &owner);
    lw_order_add_entity(&state->order, &other);
    LwCell *start = lw_map_get_cell(map, 300);
    LwCell *target = lw_cell_next(start, map, 1, 0);
    CHECK(target);
    lw_map_set_entity(map, &owner, start);
    lw_bulb_template_init(&template_, 9500, "plant", 100, 100,
                          0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6, 6, 0, 0);
    CHECK(lw_bulb_template_add_state(&template_, LW_ENTITY_STATE_ROOTED));
    lw_bulb_template_set_awakening_zone(&template_, 1);
    CHECK(lw_bulbs_add_invocation_template(&template_));
    lw_effect_parameters_init(&effect, LW_EFFECT_TYPE_SUMMON, 9500, 0, 0, 0, 0);
    lw_chip_init(&chip, 9501, 1, 1, 30, &effect, 1, 7, 1, 0,
                 0, 0, 0, 1, 9501, "summon", LW_CHIP_TYPE_BULB, -1);
    lw_fight_init(&fight, NULL);
    lw_fight_set_state(&fight, state);
    CHECK(lw_fight_summon_entity(&fight, &owner, target, &chip, NULL) > 0);
    LwEntity *plant = lw_state_get_last_entity(state);
    CHECK(plant && plant->type == LW_ENTITY_TYPE_PLANT && lw_entity_is_summon(plant));
    CHECK(lw_entity_has_state(plant, LW_ENTITY_STATE_ROOTED));
    CHECK(lw_entity_get_total_life(plant) == (colossus ? 300 : 100));
    CHECK(lw_entity_get_tp(plant) == (colossus ? 18 : 6));
    CHECK(plant->n_awakened_by == 1 && !plant->has_summon_ai);
    int found_awake = 0, found_asleep = 0;
    int invocation_at = -1, rooted_at = -1, multiplier_at = -1;
    for (int i = 0; i < state->actions.stream.n_entries; i++) {
        LwActionLog *entry = &state->actions.stream.entries[i];
        int type = entry->type;
        if (type == LW_ACTION_SUMMON) invocation_at = i;
        if (type == LW_ACTION_ADD_CHIP_EFFECT && entry->v[4] == LW_EFFECT_TYPE_ADD_STATE) rooted_at = i;
        if (type == LW_ACTION_ADD_CHIP_EFFECT && entry->v[4] == LW_EFFECT_TYPE_MULTIPLY_STATS) multiplier_at = i;
        if (type == LW_ACTION_PLANT_AWAKE) {
            found_awake++;
            CHECK(entry->v[2] == (colossus ? 18 : 6));
        }
        if (type == LW_ACTION_PLANT_ASLEEP) found_asleep++;
    }
    CHECK(found_awake == 1 && found_asleep == 1);
    CHECK(invocation_at >= 0 && rooted_at > invocation_at);
    CHECK(colossus ? multiplier_at > rooted_at : multiplier_at == -1);
    plant->life = 0;
    lw_state_on_player_die(state, plant, &owner, NULL);
    lw_entity_die(plant);
    CHECK(!lw_entity_has_state(plant, LW_ENTITY_STATE_ROOTED));
    int before_resurrection = state->actions.stream.n_entries;
    lw_state_resurrect(state, &owner, plant, target, 0, 1);
    CHECK(lw_entity_has_state(plant, LW_ENTITY_STATE_ROOTED));
    CHECK(state->order.leeks[0] == &owner && state->order.leeks[1] == plant &&
          state->order.leeks[2] == &other);
    CHECK(state->actions.stream.entries[before_resurrection].type == LW_ACTION_RESURRECT);
    CHECK(state->actions.stream.entries[before_resurrection + 1].type == LW_ACTION_ADD_CHIP_EFFECT);
    lw_bulbs_clear();
    lw_bulb_template_dispose(&template_);
    CHECK(plant->n_permanent_states == 1 && plant->permanent_states[0] == LW_ENTITY_STATE_ROOTED);
    lw_state_dispose(state);
    lw_entity_dispose(&owner);
    lw_entity_dispose(&other);
    free(map);
    free(state);
    return 0;
}

int main(void) {
    CHECK(movement_and_clone() == 0);
    CHECK(planted_and_no_ai(0) == 0);
    CHECK(planted_and_no_ai(1) == 0);
    return 0;
}
