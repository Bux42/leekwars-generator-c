package test;

import java.util.List;
import org.junit.Test;

import com.leekwars.generator.bulbs.BulbTemplate;
import com.leekwars.generator.effect.Effect;
import com.leekwars.generator.action.Actions;
import com.leekwars.generator.attack.EntityState;
import com.leekwars.generator.leek.Leek;
import com.leekwars.generator.state.Entity;
import com.leekwars.generator.test.LocalTrophyManager;
import com.leekwars.generator.turret.Turret;
import com.leekwars.generator.util.Json;
import com.leekwars.generator.weapons.Weapon;

public class CParityFixtures extends FightTestBase {
    private Leek target;
    private Leek caster;

    @Override protected void createLeeks() {
        target = makeLeek(1, "target");
        caster = makeLeek(2, "caster");
        fight.getState().addEntity(0, target);
        fight.getState().addEntity(1, caster);
    }

    private Leek makeLeek(int id, String name) {
        return new Leek(id, name, 0, 10, 1000, 6, 20, 0, 0, 10, 0,
            0, 0, 0, 8, 30, 0, false, 0, 0, "", 0, "", "", "", 0);
    }

    private int effect(int id, int turns, double value, int modifiers) {
        return Effect.createEffect(fight.getState(), id, turns, 1, value, 0,
            false, target, caster, null, 0, true, 0, 1, 0, modifiers);
    }

    private void emit(String name, int before, int... values) {
        var result = Json.createObject();
        result.put("case", name);
        var outputValues = Json.createArray();
        for (int value : values) outputValues.add(value);
        result.set("values", outputValues);
        var sourceActions = fight.getState().getActions().toJSON().get("actions");
        var actions = Json.createArray();
        for (int i = before; i < sourceActions.size(); i++) actions.add(sourceActions.get(i));
        result.set("actions", actions);
        result.set("ops", fight.getState().getActions().toJSON().get("ops"));
        System.out.println("C_PARITY " + result);
    }

    private void superinfection(String name, boolean finite, boolean invincible) throws Exception {
        initFightOnly();
        if (finite) {
            effect(Effect.TYPE_POISON, 2, 30, Effect.MODIFIER_IRREDUCTIBLE);
            effect(Effect.TYPE_POISON, 1, 50, 0);
        }
        effect(Effect.TYPE_POISON, -1, 40, 0);
        if (invincible) effect(Effect.TYPE_ADD_STATE, -1, 3, 0);
        int before = fight.getState().getActions().toJSON().get("actions").size();
        int first = effect(Effect.TYPE_SUPERINFECTION, 0, 25, 0);
        int second = effect(Effect.TYPE_SUPERINFECTION, 0, 25, 0);
        int firstTurns = target.getEffects().isEmpty() ? -2 : target.getEffects().get(0).getTurns();
        emit(name, before, first, second, target.getLife(), target.getTotalLife(),
            target.getEffects().size(), caster.getLaunchedEffects().size(), firstTurns);
    }

    @Test public void superinfectionMixed() throws Exception {
        superinfection("superinfection_mixed", true, false);
    }

    @Test public void superinfectionInfiniteOnly() throws Exception {
        superinfection("superinfection_infinite", false, false);
    }

    @Test public void superinfectionInvincible() throws Exception {
        superinfection("superinfection_invincible", true, true);
    }

    private void plantMovement(String name, int movement) throws Exception {
        initFightOnly();
        var stats = Json.createObject();
        for (String stat : List.of("life", "strength", "wisdom", "agility",
                "resistance", "science", "magic", "tp", "mp")) {
            int value = stat.equals("life") ? 100 : stat.equals("tp") ? 6 : 0;
            var pair = Json.createArray();
            pair.add(value);
            pair.add(value);
            stats.set(stat, pair);
        }
        var states = Json.createArray();
        states.add(9);
        var template = new BulbTemplate(9500, "plant", Json.createArray(), stats, states, 1);
        var plant = template.createInvocation(caster, 3, 1, false);
        var state = fight.getState();
        state.addEntity(1, plant);
        var map = state.getMap();
        var start = map.getCell(300);
        var p0 = start.next(map, 1, 0);
        var p1 = p0.next(map, 1, 0);
        var p2 = p1.next(map, 1, 0);
        var p3 = p2.next(map, 1, 0);
        map.setEntity(target, start);
        map.setEntity(plant, p1.next(map, 0, 1));
        plant.useTP(4);
        int[] calls = {0, -1};
        state.setPlantAwakening((p, trigger) -> {
            calls[0]++;
            calls[1] = trigger.getCell().getId();
        });
        int before = state.getActions().toJSON().get("actions").size();
        int moved = 0;
        if (movement == 0) moved = state.moveEntity(target, List.of(p0, p1, p2, p3));
        if (movement == 1) state.slideEntity(target, p3, target);
        if (movement == 2) state.teleportEntity(target, p1, target, 0);
        if (movement == 3) state.teleportEntity(target, p0, target, 0);
        emit(name, before, moved, target.getCell().getId(), target.getMP(),
            plant.getTP(), calls[0], calls[1]);
    }

    @Test public void plantWalk() throws Exception { plantMovement("plant_walk", 0); }
    @Test public void plantSlide() throws Exception { plantMovement("plant_slide", 1); }
    @Test public void plantTeleport() throws Exception { plantMovement("plant_teleport", 2); }
    @Test public void plantNoEntry() throws Exception { plantMovement("plant_no_entry", 3); }

    @Test public void operationsMetadata() throws Exception {
        initFightOnly();
        var statistics = new LocalTrophyManager();
        statistics.addTimes(target, 0, 456);
        statistics.addTimes(caster, 0, 789);
        var actions = new Actions();
        actions.addOpsAndTimes(statistics);
        var result = Json.createObject();
        result.put("case", "operations_metadata");
        var values = Json.createArray();
        values.add(0);
        values.add(actions.toJSON().get("ops").size());
        result.set("values", values);
        result.set("actions", Json.createArray());
        result.set("ops", actions.toJSON().get("ops"));
        System.out.println("C_PARITY " + result);
    }

    @Test public void rawShackleStrength() throws Exception {
        initFightOnly();
        target.setStrength(200);
        caster.setMagic(200);
        int before = fight.getState().getActions().toJSON().get("actions").size();
        int amount = Effect.createEffect(fight.getState(), Effect.TYPE_RAW_SHACKLE_STRENGTH,
            3, 1, 100, 0, false, target, caster, null, 0, true, 0, 1, 0, 0);
        emit("raw_shackle_strength", before, amount, target.getStat(Entity.STAT_STRENGTH),
            target.getEffects().size(), caster.getLaunchedEffects().size());
    }

    private void damageToResistance(String name, int modifiers) throws Exception {
        initFightOnly();
        var passive = Json.createObject();
        passive.put("id", Effect.TYPE_DAMAGE_TO_RESISTANCE);
        passive.put("value1", 25);
        passive.put("value2", 0);
        passive.put("turns", 2);
        passive.put("targets", 0);
        passive.put("modifiers", modifiers);
        var passives = Json.createArray();
        passives.add(passive);
        target.addWeapon(new Weapon(123, 1, 1, 1, Json.createArray(),
            (byte) 0, (byte) 1, true, 123, "test", passives, 1, false));
        int before = fight.getState().getActions().toJSON().get("actions").size();
        target.onDirectDamage(100);
        int first = target.getStat(Entity.STAT_RESISTANCE);
        target.onDirectDamage(21);
        int second = target.getStat(Entity.STAT_RESISTANCE);
        int count = target.getEffects().size();
        target.startTurn();
        target.startTurn();
        emit(name, before, first, second, count, target.getStat(Entity.STAT_RESISTANCE),
            target.getEffects().size());
    }

    @Test public void damageToResistanceReplace() throws Exception {
        damageToResistance("damage_to_resistance_replace", 0);
    }

    @Test public void damageToResistanceStack() throws Exception {
        damageToResistance("damage_to_resistance_stack", Effect.MODIFIER_STACKABLE);
    }

    private void periodicTurns(String name, int effectType, int initialLife) throws Exception {
        initFightOnly();
        target.setLife(initialLife);
        effect(effectType, 2, 30, 0);
        int before = fight.getState().getActions().toJSON().get("actions").size();
        caster.startTurn();
        int afterCaster = target.getLife();
        int firstTurns = target.getEffects().get(0).getTurns();
        target.startTurn();
        int afterFirst = target.getLife();
        int secondTurns = target.getEffects().get(0).getTurns();
        caster.startTurn();
        target.startTurn();
        emit(name, before, afterCaster, firstTurns, afterFirst, secondTurns,
            target.getLife(), target.getEffects().size(), caster.getLaunchedEffects().size());
    }

    @Test public void periodicPoison() throws Exception {
        periodicTurns("periodic_poison", Effect.TYPE_POISON, 1000);
    }

    @Test public void periodicAftereffect() throws Exception {
        periodicTurns("periodic_aftereffect", Effect.TYPE_AFTEREFFECT, 1000);
    }

    @Test public void periodicHeal() throws Exception {
        periodicTurns("periodic_heal", Effect.TYPE_HEAL, 500);
    }

    @Test public void startEffectsResurrection() throws Exception {
        initFightOnly();
        target.setStrength(200);
        target.setStartEffects(List.of(new int[] {Effect.TYPE_RAW_SHACKLE_STRENGTH, 50}));
        var state = fight.getState();
        state.recordInitialState();
        int before = state.getActions().toJSON().get("actions").size() - 1;
        var cell = target.getCell();
        int initialStrength = target.getStat(Entity.STAT_STRENGTH);
        state.onPlayerDie(target, caster, null);
        int deadStrength = target.getStat(Entity.STAT_STRENGTH);
        int deadEffects = target.getEffects().size();
        state.resurrect(caster, target, cell, false, true);
        emit("start_effects_resurrection", before, initialStrength, deadStrength,
            deadEffects, target.getStat(Entity.STAT_STRENGTH), target.getEffects().size());
    }

    @Test public void idleTurnFlow() throws Exception {
        var state = fight.getState();
        state.seed(1);
        initFightOnly();
        state.recordInitialState();
        int before = state.getActions().toJSON().get("actions").size();
        fight.startTurn();
        fight.startTurn();
        var current = state.getOrder().current();
        emit("idle_turn_flow", before, state.getOrder().getTurn(),
            current == null ? -1 : current.getFId(), target.getLife(), caster.getLife());
    }

    @Test public void turretStartFight() throws Exception {
        var state = fight.getState();
        var turret = new Turret();
        turret.setId(3);
        state.addEntity(0, turret);
        initFightOnly();
        int before = state.getActions().toJSON().get("actions").size();
        turret.startFight();
        emit("turret_start_fight", before, before,
            turret.hasState(EntityState.STATIC) ? 1 : 0);
        state.recordInitialState();
    }
}
