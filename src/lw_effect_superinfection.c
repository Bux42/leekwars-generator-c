#include "lw_effect.h"
#include "lw_actions.h"
#include "lw_attack.h"
#include "lw_state.h"
#include "lw_util.h"

void lw_effect_superinfection_apply(LwEffect *self, struct LwState *state) {
    double ratio = lw_min_d(1.0, ((self->value1 + self->jet * self->value2) / 100.0)
                                    * self->critical_power);
    int poisons = 0;
    for (int i = 0; i < self->target->n_effects; ++i) {
        LwEffect *effect = self->target->effects[i];
        if (effect->id != LW_EFFECT_TYPE_POISON || effect->turns == -1) continue;
        poisons += effect->value;
        if (lw_entity_lose_effect_turn(self->target, effect)) {
            --i;
        } else {
            lw_actions_log_update_effect_turns(lw_state_get_actions(state),
                                               effect->log_id, effect->turns);
        }
    }

    int damages = lw_java_round(poisons * ratio);
    if (self->target->life < damages) damages = self->target->life;
    if (lw_entity_has_state(self->target, LW_ENTITY_STATE_INVINCIBLE)) damages = 0;
    if (damages > 0) {
        double rate = LW_EROSION_POISON_F + (self->critical ? LW_EROSION_CRITICAL_BONUS_F : 0);
        int erosion = lw_java_round(damages * rate);
        lw_actions_log_damage(lw_state_get_actions(state), LW_DAMAGE_TYPE_POISON,
                              self->target, damages, erosion);
        lw_entity_remove_life(self->target, damages, erosion, self->caster,
                              LW_DAMAGE_TYPE_POISON, self, lw_effect_get_item(self));
        lw_entity_on_poison_damage(self->target, damages);
        lw_entity_on_nova_damage(self->target, erosion);
    }
    self->value = damages;
}
