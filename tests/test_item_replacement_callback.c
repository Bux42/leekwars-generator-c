#include <stdio.h>
#include <stdlib.h>

#include "lw_chip.h"
#include "lw_entity.h"
#include "lw_map.h"
#include "lw_state.h"

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "line %d: %s\n", __LINE__, #condition); \
        return 1; \
    } \
} while (0)

static void replace_chip(LwStatisticsManager *manager, LwEntity *caster,
                         LwChip *chip, LwCell *cell, LwEntity **targets,
                         int n_targets, LwEntity *cell_entity) {
    (void)caster; (void)chip; (void)cell; (void)targets;
    (void)n_targets; (void)cell_entity;
    lw_chips_add_chip(manager->user_data);
}

int main(void) {
    LwState *state = calloc(1, sizeof(*state));
    LwMap *map = calloc(1, sizeof(*map));
    LwChip *chip = calloc(1, sizeof(*chip));
    LwChip replacement = {0};
    LwEntity caster;
    LwStatisticsManager stats = {0};
    CHECK(state && map && chip);
    lw_state_init(state);
    lw_map_init(map, 18, 18);
    map->state = state;
    state->map = map;
    lw_entity_init_id_name(&caster, 1, "caster");
    caster.life = caster.m_total_life = 100;
    lw_stats_set_stat(&caster.m_base_stats, LW_STAT_TP, 6);
    lw_state_add_entity(state, 0, &caster);
    lw_order_add_entity(&state->order, &caster);
    lw_map_set_entity(map, &caster, lw_map_get_cell(map, 72));
    lw_chip_init(chip, 77, 2, 1, 30, NULL, 0, 7, 1, 0,
                 1, 0, 0, 1, 77, "old", LW_CHIP_TYPE_BOOST, -1);
    CHECK(lw_item_manage(&chip->base));
    CHECK(lw_chips_add_chip(chip));
    lw_item_release(&chip->base);
    replacement.base.id = 77;
    stats.user_data = &replacement;
    stats.use_chip = replace_chip;
    lw_state_set_statistics_manager(state, &stats);
    CHECK(lw_state_use_chip(state, &caster, lw_map_get_cell(map, 73), chip) > 0);
    CHECK(caster.used_tp == 2);
    CHECK(lw_chips_get_chip(77) == &replacement);
    lw_state_dispose(state);
    lw_entity_dispose(&caster);
    lw_chips_clear();
    free(map);
    free(state);
    return 0;
}
