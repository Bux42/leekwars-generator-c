/*
 * lw_weapons.c -- 1:1 port of weapons/Weapons.java + the Weapon
 * constructor (weapons/Weapon.java).
 *
 * Java: public class Weapons {
 *           private static Map<Integer, Weapon> weapons = new TreeMap<>();
 *           public static void addWeapon(Weapon weapon) {
 *               weapons.put(weapon.getId(), weapon);
 *               Items.addWeapon(weapon.getId());
 *           }
 *           public static Weapon getWeapon(int id) { return weapons.get(id); }
 *           public static Weapon getWeapon(String name) { ... }
 *           public static Map<Integer, Weapon> getTemplates() { return weapons; }
 *       }
 *
 * Reference: java_reference/src/main/java/com/leekwars/generator/weapons/Weapons.java
 *            java_reference/src/main/java/com/leekwars/generator/weapons/Weapon.java
 */
#include "lw_weapon.h"

#include "lw_attack.h"
#include "lw_item.h"
#include "lw_constants.h"

#include <stddef.h>
#include <string.h>


/* Items.addWeapon(id) -- the items registry is owned by the Items
 * loader (not yet ported). Forward-declare so the link resolves once
 * lw_items.c lands. */
void lw_items_add_weapon(int id);


/* ---- Weapon constructor (Weapon.java:16-30) ---------------------- */

/* public Weapon(int id, int cost, int minRange, int maxRange,
 *               ArrayNode effects, byte launchType, byte area,
 *               boolean los, int template, String name,
 *               ArrayNode passiveEffects, int maxUses) */
void lw_weapon_init(LwWeapon *self,
                    int id, int cost, int min_range, int max_range,
                    const LwEffectParameters *effects, int n_effects,
                    int8_t launch_type, int area, int los,
                    int template_, const char *name,
                    const LwEffectParameters *passive_effects, int n_passive_effects,
                    int max_uses) {

    /* super(id, cost, name, template, new Attack(minRange, maxRange,
     *      launchType, area, los, effects, Attack.TYPE_WEAPON, id, maxUses)); */
    lw_attack_init(&self->attack_storage,
                   min_range, max_range, launch_type, area, los,
                   effects, n_effects,
                   LW_ATTACK_TYPE_WEAPON, id, max_uses);
    lw_item_init(&self->base, id, cost, name, template_, &self->attack_storage);

    /* this.attack.setItem(this); */
    lw_attack_set_item(&self->attack_storage, &self->base);

    /* Populate passiveEffects[] from the JSON. */
    self->n_passive_effects = 0;
    for (int i = 0; i < n_passive_effects; i++) {
        if (self->n_passive_effects >= LW_WEAPON_MAX_PASSIVES) break;
        const LwEffectParameters *e = &passive_effects[i];
        lw_effect_parameters_init(&self->passive_effects[self->n_passive_effects],
                                  e->id, e->value1, e->value2,
                                  e->turns, e->targets, e->modifiers);
        self->n_passive_effects++;
    }
}


/* ---- Weapons static registry (Weapons.java) ---------------------- *
 *
 * Java's TreeMap<Integer, Weapon> sorts by key ascending; we keep the
 * weapons array sorted by id (insertion-sorted on add) so iteration via
 * getTemplates() preserves the Java order. Lookup is binary-search.
 *
 * The backing array grows as items are registered.
 */
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>

static LwWeapon **s_weapons;
static int       s_n_weapons = 0;
static int       s_weapons_capacity;


/* public static void addWeapon(Weapon weapon) {
 *     weapons.put(weapon.getId(), weapon);
 *     Items.addWeapon(weapon.getId());
 * }
 */
int lw_weapons_add_weapon(LwWeapon *weapon) {
    if (weapon == NULL) return 0;
    int id = lw_weapon_get_id(weapon);

    /* TreeMap.put: replace if key exists. */
    for (int i = 0; i < s_n_weapons; i++) {
        if (lw_weapon_get_id(s_weapons[i]) == id) {
            LwWeapon *old = s_weapons[i];
            lw_item_retain(&weapon->base);
            s_weapons[i] = weapon;
            lw_item_release(&old->base);
            lw_items_add_weapon(id);
            return 1;
        }
    }
    if (s_n_weapons == s_weapons_capacity) {
        if (s_weapons_capacity > INT_MAX / 2) return 0;
        int capacity = s_weapons_capacity ? s_weapons_capacity * 2 : 16;
        if ((size_t)capacity > SIZE_MAX / sizeof(*s_weapons)) return 0;
        LwWeapon **weapons = realloc(s_weapons, (size_t)capacity * sizeof(*weapons));
        if (!weapons) return 0;
        s_weapons = weapons;
        s_weapons_capacity = capacity;
    }

    /* Insertion sort by id ascending (TreeMap iteration order). */
    int pos = s_n_weapons;
    while (pos > 0 && lw_weapon_get_id(s_weapons[pos - 1]) > id) {
        s_weapons[pos] = s_weapons[pos - 1];
        pos--;
    }
    s_weapons[pos] = weapon;
    lw_item_retain(&weapon->base);
    s_n_weapons++;

    lw_items_add_weapon(id);
    return 1;
}


/* public static Weapon getWeapon(int id) { return weapons.get(id); }
 *
 * Linear scan over the (sorted) array.  Bounded n -> fine. */
LwWeapon *lw_weapons_get_weapon(int id) {
    for (int i = 0; i < s_n_weapons; i++) {
        if (lw_weapon_get_id(s_weapons[i]) == id) return s_weapons[i];
    }
    return NULL;
}


/* public static Weapon getWeapon(String name) {
 *     return weapons.values().stream().filter(w -> w.getName().equals(name))
 *                                     .findFirst().get();
 * }
 *
 * Java throws NoSuchElementException on miss; we return NULL. */
LwWeapon *lw_weapons_get_weapon_by_name(const char *name) {
    if (name == NULL) return NULL;
    for (int i = 0; i < s_n_weapons; i++) {
        const char *n = lw_weapon_get_name(s_weapons[i]);
        if (n != NULL && strcmp(n, name) == 0) return s_weapons[i];
    }
    return NULL;
}


/* public static Map<Integer, Weapon> getTemplates() { return weapons; } */
LwWeapon **lw_weapons_get_templates(int *out_n) {
    if (out_n) *out_n = s_n_weapons;
    return s_weapons;
}

void lw_weapons_clear(void) {
    for (int i = 0; i < s_n_weapons; i++) lw_item_release(&s_weapons[i]->base);
    free(s_weapons);
    s_weapons = NULL;
    s_n_weapons = 0;
    s_weapons_capacity = 0;
}
