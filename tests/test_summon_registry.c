#include <stdio.h>
#include <stdlib.h>

#include "lw_bulb_template.h"
#include "lw_chip.h"

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "line %d: %s\n", __LINE__, #condition); \
        return 1; \
    } \
} while (0)

int main(void) {
    LwBulbTemplate templates[65] = {0};
    LwChip chip = {0};
    for (int i = 0; i < 65; i++) {
        templates[i].m_id = i;
        CHECK(lw_bulbs_add_invocation_template(&templates[i]));
    }
    for (int i = 0; i < 65; i++) {
        CHECK(lw_bulbs_get_invocation_template(i) == &templates[i]);
    }

    for (int i = 0; i < 80; i++) {
        CHECK(lw_bulb_template_add_chip(&templates[0], &chip));
    }
    CHECK(lw_bulb_template_get_chips_count(&templates[0]) == 80);
    CHECK(lw_bulb_template_get_chip_at(&templates[0], 79) == &chip);
    CHECK(!lw_bulb_template_add_chip(&templates[0], NULL));

    LwBulbTemplate *owned = calloc(1, sizeof(*owned));
    CHECK(owned != NULL);
    owned->m_id = 0;
    CHECK(lw_bulb_template_add_chip(owned, &chip));
    CHECK(lw_bulbs_add_owned_invocation_template(owned));
    CHECK(lw_bulbs_get_invocation_template(0) == owned);
    CHECK(lw_bulbs_add_owned_invocation_template(owned));
    CHECK(lw_bulbs_get_invocation_template(0) == owned);
    CHECK(lw_bulbs_add_invocation_template(&templates[0]));
    CHECK(lw_bulbs_get_invocation_template(0) == &templates[0]);
    owned = calloc(1, sizeof(*owned));
    CHECK(owned != NULL);
    owned->m_id = 100;
    CHECK(lw_bulb_template_add_chip(owned, &chip));
    CHECK(lw_bulbs_add_owned_invocation_template(owned));
    lw_bulbs_clear();
    CHECK(lw_bulbs_get_invocation_template(0) == NULL);
    CHECK(templates[1].m_id == 1);
    lw_bulb_template_dispose(&templates[0]);
    return 0;
}
