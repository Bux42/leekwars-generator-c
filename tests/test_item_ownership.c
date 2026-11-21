#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lw_bulb_template.h"
#include "lw_chip.h"
#include "lw_effect.h"
#include "lw_state.h"
#include "lw_weapon.h"

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "line %d: %s\n", __LINE__, #condition); \
        return 1; \
    } \
} while (0)

int main(void) {
    LwState *state = calloc(1, sizeof(*state));
    LwState *clone = calloc(1, sizeof(*clone));
    LwChip *chip = calloc(1, sizeof(*chip));
    LwChip replacement = {0};
    LwWeapon *weapon = calloc(1, sizeof(*weapon));
    LwWeapon replacement_weapon = {0};
    LwEntity entity;
    LwBulbTemplate template_;
    char name[] = "original";
    CHECK(state && clone && chip && weapon);
    lw_state_init(state);
    lw_entity_init_id_name(&entity, 1, "entity");
    entity.life = entity.m_total_life = 100;
    lw_stats_set_stat(&entity.m_base_stats, LW_STAT_RAM, 6);
    lw_state_add_entity(state, 0, &entity);
    lw_chip_init(chip, 10, 1, 1, 10, NULL, 0, 7, 1, 0,
                 0, 0, 0, 1, 10, name, LW_CHIP_TYPE_BOOST, -1);
    CHECK(lw_item_manage(&chip->base));
    name[0] = 'X';
    CHECK(strcmp(lw_chip_get_name(chip), "original") == 0);
    CHECK(lw_chips_add_chip(chip));
    lw_item_release(&chip->base);
    lw_bulb_template_init(&template_, 1, "bulb", 1, 1, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1);
    CHECK(lw_bulb_template_add_chip(&template_, chip));
    lw_entity_add_chip(&entity, chip);
    lw_weapon_init(weapon, 20, 1, 1, 10, NULL, 0, 7, 1, 0,
                   20, "weapon", NULL, 0, -1);
    CHECK(lw_item_manage(&weapon->base));
    CHECK(lw_weapons_add_weapon(weapon));
    lw_item_release(&weapon->base);
    lw_entity_add_weapon(&entity, weapon);
    lw_entity_set_weapon(&entity, weapon);
    CHECK(lw_effect_create_effect(state, LW_EFFECT_TYPE_BUFF_STRENGTH,
                                  2, 1, 10, 0, 0, &entity, &entity,
                                  lw_chip_get_attack(chip), 0, 1, 0, 1, 0, 0) > 0);
    CHECK(lw_state_clone(clone, state));
    lw_item_retain(&chip->base);
    lw_item_retain(&weapon->base);
    replacement.base.id = 10;
    CHECK(lw_chips_add_chip(&replacement));
    replacement_weapon.base.id = 20;
    CHECK(lw_weapons_add_weapon(&replacement_weapon));
    CHECK(lw_bulb_template_get_chip_at(&template_, 0) == chip);
    lw_bulb_template_dispose(&template_);
    lw_state_dispose(state);
    lw_entity_dispose(&entity);
    CHECK(strcmp(lw_chip_get_name(chip), "original") == 0);
    lw_state_dispose(clone);
    CHECK(chip->base.references == 1);
    CHECK(weapon->base.references == 1);
    lw_item_release(&chip->base);
    lw_item_release(&weapon->base);
    lw_chips_clear();
    lw_weapons_clear();
    free(clone);
    free(state);
    return 0;
}
