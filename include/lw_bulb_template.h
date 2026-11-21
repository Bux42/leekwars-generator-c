/*
 * lw_bulb_template.h -- 1:1 port of bulbs/BulbTemplate.java + bulbs/Bulbs.java
 *
 * BulbTemplate is the immutable summon descriptor (min/max stats + chip
 * list); Bulbs is the global registry (TreeMap<Integer,BulbTemplate>) used
 * by EffectSummon to instantiate a bulb at fight time.
 *
 * In C we collapse the two into one header: LwBulbTemplate (value-type
 * struct) + a small global registry indexed by template id.
 *
 * Reference:
 *   java_reference/src/main/java/com/leekwars/generator/bulbs/BulbTemplate.java
 *   java_reference/src/main/java/com/leekwars/generator/bulbs/Bulbs.java
 */
#ifndef LW_BULB_TEMPLATE_H
#define LW_BULB_TEMPLATE_H

#include "lw_entity.h"


/* Forward decl -- chips/Chip.java. */
struct LwChip;


#define LW_BULB_TEMPLATE_NAME_MAX     64


typedef struct LwBulbTemplate {

    int    m_id;                                            /* private final int mId */
    char   m_name[LW_BULB_TEMPLATE_NAME_MAX];               /* private final String mName */

    /* private final ArrayList<Chip> mChips */
    struct LwChip** m_chips;
    int    n_chips;
    int    chips_capacity;
    int   *m_states;
    int    n_states;
    int    states_capacity;
    int    awakening_zone;

    int    m_min_life,        m_max_life;
    int    m_min_strength,    m_max_strength;
    int    m_min_wisdom,      m_max_wisdom;
    int    m_min_agility,     m_max_agility;
    int    m_min_resistance,  m_max_resistance;
    int    m_min_science,     m_max_science;
    int    m_min_magic,       m_max_magic;
    int    m_min_tp,          m_max_tp;
    int    m_min_mp,          m_max_mp;

} LwBulbTemplate;


/* Java: public BulbTemplate(int id, String name, ArrayNode chips, ObjectNode characteristics)
 *
 * In C the JSON parsing happens in the binding layer; the init function
 * takes the already-extracted min/max bounds. Dispose a previously initialized
 * template before reinitializing it. */
void lw_bulb_template_init(LwBulbTemplate *self, int id, const char *name,
                           int min_life, int max_life,
                           int min_strength, int max_strength,
                           int min_wisdom, int max_wisdom,
                           int min_agility, int max_agility,
                           int min_resistance, int max_resistance,
                           int min_science, int max_science,
                           int min_magic, int max_magic,
                           int min_tp, int max_tp,
                           int min_mp, int max_mp);

/* Java: void mChips.add(template) -- helper to fill the chip list after init. */
/* Releases the chip pointer array, not the chips themselves. Unregister a
 * borrowed template before disposing it. */
void lw_bulb_template_dispose(LwBulbTemplate *self);
/* Returns 0 for invalid input or allocation failure. */
int lw_bulb_template_add_chip(LwBulbTemplate *self, struct LwChip *chip);
/* Invalid state ordinals are ignored, as in Java. Returns 0 on allocation failure. */
int lw_bulb_template_add_state(LwBulbTemplate *self, int state);
void lw_bulb_template_set_awakening_zone(LwBulbTemplate *self, int zone);


/* Java: public int getId() / getName() */
int          lw_bulb_template_get_id  (const LwBulbTemplate *self);
const char*  lw_bulb_template_get_name(const LwBulbTemplate *self);


/* Java: public static int base(int base, int bonus, double coeff, double multiplier) {
 *           return (int) ((base + Math.floor((bonus - base) * coeff)) * multiplier);
 *       }
 */
int lw_bulb_template_base(int base, int bonus, double coeff, double multiplier);


/* Java: public Bulb createInvocation(Entity owner, int id, int level, boolean critical) {
 *           ... build a Bulb with stat fields scaled by owner.level ...
 *       }
 *
 * Initialises the caller-provided LwEntity in-place as a bulb of this
 * template, scaled to `owner.level` and possibly buffed by the critical
 * multiplier. Returns 1 on success, 0 if template has no slot.
 */
int lw_bulb_template_create_invocation(const LwBulbTemplate *self,
                                       LwEntity *out_entity,
                                       LwEntity *owner, int id,
                                       int level, int critical);


int lw_bulb_template_get_chips_count(const LwBulbTemplate *self);
struct LwChip* lw_bulb_template_get_chip_at(const LwBulbTemplate *self, int index);


/* Min/max accessors (Java: getMinLife / getMaxLife / ...) */
int lw_bulb_template_get_min_life      (const LwBulbTemplate *self);
int lw_bulb_template_get_max_life      (const LwBulbTemplate *self);
int lw_bulb_template_get_min_strength  (const LwBulbTemplate *self);
int lw_bulb_template_get_max_strength  (const LwBulbTemplate *self);
int lw_bulb_template_get_min_wisdom    (const LwBulbTemplate *self);
int lw_bulb_template_get_max_wisdom    (const LwBulbTemplate *self);
int lw_bulb_template_get_min_agility   (const LwBulbTemplate *self);
int lw_bulb_template_get_max_agility   (const LwBulbTemplate *self);
int lw_bulb_template_get_min_resistance(const LwBulbTemplate *self);
int lw_bulb_template_get_max_resistance(const LwBulbTemplate *self);
int lw_bulb_template_get_min_science   (const LwBulbTemplate *self);
int lw_bulb_template_get_max_science   (const LwBulbTemplate *self);
int lw_bulb_template_get_min_magic     (const LwBulbTemplate *self);
int lw_bulb_template_get_max_magic     (const LwBulbTemplate *self);
int lw_bulb_template_get_min_tp        (const LwBulbTemplate *self);
int lw_bulb_template_get_max_tp        (const LwBulbTemplate *self);
int lw_bulb_template_get_min_mp        (const LwBulbTemplate *self);
int lw_bulb_template_get_max_mp        (const LwBulbTemplate *self);


/* ---- Bulbs registry (Java: bulbs/Bulbs.java) ----------------------- */

/* Java: public static BulbTemplate getInvocationTemplate(int id) {
 *           return sTemplates.get(id);
 *       }
 */
LwBulbTemplate* lw_bulbs_get_invocation_template(int id);

/* Java: public static void addInvocationTemplate(BulbTemplate invocation) {
 *           sTemplates.put(invocation.getId(), invocation);
 *       }
 *
 * The borrowed variant stores a caller-owned pointer. The owned variant
 * takes ownership only on success and disposes and frees it on replacement
 * or clear. Registered borrowed templates must remain valid until replaced
 * or cleared.
 * Both return 0 on invalid input or allocation failure. */
int lw_bulbs_add_invocation_template(LwBulbTemplate *invocation);
int lw_bulbs_add_owned_invocation_template(LwBulbTemplate *invocation);

/* Reset the registry (test harness use). */
void lw_bulbs_clear(void);


#endif /* LW_BULB_TEMPLATE_H */
