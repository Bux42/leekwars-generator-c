/* Full tables must abort the fight, not drop entries silently. */
#include <stdio.h>
#include <stdlib.h>

#include "lw_chip.h"
#include "lw_effect.h"
#include "lw_entity.h"
#include "lw_order.h"
#include "lw_state.h"
#include "lw_team.h"
#include "lw_weapon.h"

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "line %d: %s\n", __LINE__, #condition); \
        return 1; \
    } \
} while (0)

typedef struct {
    LwState state;
    LwEntity entities[LW_STATE_MAX_ENTITIES + 1];
    LwChip chips[64];
} Fixture;

static Fixture *make_fixture(void) {
    Fixture *f = calloc(1, sizeof(*f));
    if (!f) exit(2);
    lw_state_init(&f->state);
    for (int i = 0; i <= LW_STATE_MAX_ENTITIES; i++) {
        lw_entity_init_id_name(&f->entities[i], i + 1, "entity");
        f->entities[i].life = f->entities[i].m_total_life = 100;
    }
    for (int i = 0; i < 64; i++)
        lw_chip_init(&f->chips[i], 9800 + i, 1, 1, 1, NULL, 0, 7, 1, 0,
                     0, 0, 0, 1, 9800 + i, "chip", 1, -1);
    lw_state_add_entity(&f->state, 0, &f->entities[0]);
    return f;
}

static void free_fixture(Fixture *f) {
    lw_state_dispose(&f->state);
    free(f);
}

static int cooldowns(int team) {
    Fixture *f = make_fixture();
    int limit = team ? LW_TEAM_MAX_COOLDOWNS : LW_ENTITY_MAX_COOLDOWNS;
    for (int i = 0; i <= limit; i++) {
        CHECK(!lw_state_get_fatal_error(&f->state));
        f->chips[i].team_cooldown = team;
        lw_state_add_cooldown(&f->state, &f->entities[0], &f->chips[i], 3);
    }
    CHECK(lw_state_get_fatal_error(&f->state) == LW_STATE_ERROR_CAPACITY);
    CHECK(lw_state_get_fatal_error_message(&f->state)[0]);
    free_fixture(f);

    /* Refreshing a known cooldown in a full table is not an overflow. */
    f = make_fixture();
    for (int i = 0; i < limit; i++) {
        f->chips[i].team_cooldown = team;
        lw_state_add_cooldown(&f->state, &f->entities[0], &f->chips[i], 3);
    }
    lw_state_add_cooldown(&f->state, &f->entities[0], &f->chips[0], 5);
    CHECK(!lw_state_get_fatal_error(&f->state));
    CHECK(lw_state_has_cooldown(&f->state, &f->entities[0], &f->chips[0]));
    free_fixture(f);
    return 0;
}

static int item_uses(void) {
    Fixture *f = make_fixture();
    for (int i = 0; i < LW_ENTITY_MAX_ITEM_USES; i++) lw_entity_add_item_use(&f->entities[0], i);
    lw_entity_add_item_use(&f->entities[0], 0);
    CHECK(!lw_state_get_fatal_error(&f->state));
    CHECK(lw_entity_get_item_uses(&f->entities[0], 0) == 2);
    lw_entity_add_item_use(&f->entities[0], LW_ENTITY_MAX_ITEM_USES);
    CHECK(lw_state_get_fatal_error(&f->state) == LW_STATE_ERROR_CAPACITY);
    free_fixture(f);
    return 0;
}

static int effects(int launched) {
    Fixture *f = make_fixture();
    LwEffect *effect = calloc(1, sizeof(*effect));
    LwEntity *e = &f->entities[0];
    int limit = launched ? LW_ENTITY_MAX_LAUNCHED_EFFECTS : LW_ENTITY_MAX_EFFECTS;
    CHECK(effect);
    lw_effect_init(effect);
    for (int i = 0; i <= limit; i++) {
        CHECK(!lw_state_get_fatal_error(&f->state));
        if (launched) lw_entity_add_launched_effect(e, effect);
        else lw_entity_add_effect(e, effect);
    }
    CHECK(lw_state_get_fatal_error(&f->state) == LW_STATE_ERROR_CAPACITY);
    CHECK((launched ? e->n_launched_effects : e->n_effects) == limit);
    e->n_effects = e->n_launched_effects = 0;
    free(effect);
    free_fixture(f);
    return 0;
}

static int entities(void) {
    Fixture *f = make_fixture();
    for (int i = 1; i < LW_STATE_MAX_ENTITIES; i++)
        lw_state_add_entity(&f->state, i % 2, &f->entities[i]);
    CHECK(!lw_state_get_fatal_error(&f->state));
    CHECK(f->state.n_entities == LW_STATE_MAX_ENTITIES);
    lw_state_add_entity(&f->state, 0, &f->entities[LW_STATE_MAX_ENTITIES]);
    CHECK(lw_state_get_fatal_error(&f->state) == LW_STATE_ERROR_CAPACITY);
    free_fixture(f);
    return 0;
}

static int teams(void) {
    Fixture *f = make_fixture();
    lw_state_add_entity(&f->state, LW_STATE_MAX_TEAMS - 1, &f->entities[1]);
    CHECK(!lw_state_get_fatal_error(&f->state));
    lw_state_add_entity(&f->state, LW_STATE_MAX_TEAMS, &f->entities[2]);
    CHECK(lw_state_get_fatal_error(&f->state) == LW_STATE_ERROR_CAPACITY);
    free_fixture(f);
    return 0;
}

static int order(void) {
    Fixture *f = make_fixture();
    LwOrder o;
    lw_order_init(&o);
    for (int i = 0; i < LW_ORDER_MAX_ENTITIES; i++)
        CHECK(lw_order_add_entity(&o, &f->entities[i]));
    CHECK(!lw_order_add_entity(&o, &f->entities[0]));
    CHECK(!lw_order_add_entity_at(&o, 0, &f->entities[0]));
    CHECK(!lw_order_add_summon(&o, &f->entities[0], &f->entities[1]));
    CHECK(o.n_leeks == LW_ORDER_MAX_ENTITIES && o.position == 0);
    free_fixture(f);
    return 0;
}

static int loadout(void) {
    Fixture *f = make_fixture();
    LwEntity *e = &f->entities[0];
    LwWeapon weapon;
    lw_stats_set_stat(&e->m_base_stats, LW_STAT_RAM, 64);
    for (int i = 0; i < LW_ENTITY_MAX_CHIPS; i++) lw_entity_add_chip(e, &f->chips[i]);
    lw_entity_add_chip(e, &f->chips[0]);    /* already equipped */
    CHECK(!lw_state_get_fatal_error(&f->state) && e->n_chips == LW_ENTITY_MAX_CHIPS);
    lw_entity_add_chip(e, &f->chips[LW_ENTITY_MAX_CHIPS]);
    CHECK(lw_state_get_fatal_error(&f->state) == LW_STATE_ERROR_CAPACITY);
    free_fixture(f);

    f = make_fixture();
    lw_weapon_init(&weapon, 9900, 1, 1, 1, NULL, 0, 7, 1, 1, 9900, "weapon", NULL, 0, -1);
    for (int i = 0; i < LW_ENTITY_MAX_WEAPONS; i++) lw_entity_add_weapon(&f->entities[0], &weapon);
    CHECK(!lw_state_get_fatal_error(&f->state));
    lw_entity_add_weapon(&f->entities[0], &weapon);
    CHECK(lw_state_get_fatal_error(&f->state) == LW_STATE_ERROR_CAPACITY);
    free_fixture(f);
    return 0;
}

int main(void) {
    CHECK(cooldowns(0) == 0);
    CHECK(cooldowns(1) == 0);
    CHECK(item_uses() == 0);
    CHECK(effects(0) == 0);
    CHECK(effects(1) == 0);
    CHECK(entities() == 0);
    CHECK(teams() == 0);
    CHECK(order() == 0);
    CHECK(loadout() == 0);
    return 0;
}
