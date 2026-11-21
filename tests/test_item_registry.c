#include <stdio.h>
#include <stdlib.h>

#include "lw_chip.h"
#include "lw_weapon.h"

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "line %d: %s\n", __LINE__, #condition); \
        return 1; \
    } \
} while (0)

int main(void) {
    LwChip *chips = calloc(520, sizeof(*chips));
    LwWeapon *weapons = calloc(260, sizeof(*weapons));
    LwChip chip_replacement = {0};
    LwWeapon weapon_replacement = {0};
    CHECK(chips && weapons);
    for (int i = 0; i < 520; i++) {
        chips[i].base.id = i;
        CHECK(lw_chips_add_chip(&chips[i]));
    }
    for (int i = 0; i < 260; i++) {
        weapons[i].base.id = i;
        CHECK(lw_weapons_add_weapon(&weapons[i]));
    }
    CHECK(lw_chips_get_chip(519) == &chips[519]);
    CHECK(lw_weapons_get_weapon(259) == &weapons[259]);
    chip_replacement.base.id = 5;
    weapon_replacement.base.id = 5;
    CHECK(lw_chips_add_chip(&chip_replacement));
    CHECK(lw_weapons_add_weapon(&weapon_replacement));
    CHECK(lw_chips_get_chip(5) == &chip_replacement);
    CHECK(lw_weapons_get_weapon(5) == &weapon_replacement);
    lw_chips_clear();
    lw_weapons_clear();
    CHECK(chips[5].base.id == 5 && weapons[5].base.id == 5);
    free(chips);
    free(weapons);
    return 0;
}
