import unittest

from leekwars_c._engine import Engine


def setup_fight(awakening_zone=2, initial_cooldown=0, team_cooldown=False):
    engine = Engine()
    engine.add_chip(
        item_id=99401, name="plant attack", cost=1, min_range=1,
        max_range=100, launch_type=7, area_id=1, los=False, max_uses=-1,
        cooldown=0, initial_cooldown=initial_cooldown, team_cooldown=team_cooldown, level=1,
        chip_type=1, effects=[(1, 10, 0, 0, 11, 0)],
    )
    engine.add_summon_template(
        99400, "plant", (100, 100), *((0, 0),) * 6, (6, 6), (0, 0),
        [99401], states=[9], awakening_zone=awakening_zone,
    )
    engine.add_chip(
        item_id=99402, name="summon plant", cost=1, min_range=1,
        max_range=100, launch_type=7, area_id=1, los=False, max_uses=-1,
        cooldown=0, initial_cooldown=0, team_cooldown=False, level=1,
        chip_type=8, effects=[(14, 99400, 0, 0, 1, 0)],
    )
    for team in range(2):
        ident = team + 1
        engine.add_farmer(ident, "farmer", "fr")
        engine.add_team(ident, "team")
        engine.add_entity(
            team=team, fid=ident, name="leek", level=1, life=100,
            tp=6, mp=6, strength=0, agility=0, frequency=100, wisdom=0,
            resistance=0, science=0, magic=0, cores=0, ram=1,
            farmer=ident, team_id=ident, weapons=[], chips=[99402],
            cell=(72, 144)[team],
        )
    engine.set_custom_map(obstacles={}, team1=[72], team2=[144])
    engine.set_max_turns(1)
    return engine


class PlantAwakeningTests(unittest.TestCase):
    def test_summoned_plant_acts_on_trigger(self):
        engine = setup_fight()
        calls = []

        def awake(ai_id, plant, trigger, turn):
            calls.append((ai_id, plant, trigger, turn,
                          engine.use_chip(plant, 99401, engine.entity_cell(trigger))))
            return 0

        def act(idx, turn):
            if idx == 0:
                self.assertEqual(engine.summon(idx, 99402, 73, ai_id=123), 1)
            return 0

        engine.set_plant_awaken_callback(awake)
        engine.set_ai_callback(act)
        self.assertEqual(engine.run(), 0)
        self.assertEqual(calls, [(123, 2, 0, 1, 1)])
        self.assertLess(engine.entity_hp(0), 100)
        actions = [entry["type"] for entry in engine.stream_dump()]
        awake_at = actions.index(17)
        self.assertIn(12, actions[awake_at + 1:actions.index(18, awake_at)])

    def test_no_ai_plant_wakes_without_calling_dispatcher(self):
        engine = setup_fight()
        calls = []
        engine.set_plant_awaken_callback(lambda *args: calls.append(args))
        engine.set_ai_callback(lambda idx, turn: engine.use_chip(idx, 99402, 73) if idx == 0 else 0)
        self.assertEqual(engine.run(), 0)
        self.assertEqual(calls, [])
        actions = [entry["type"] for entry in engine.stream_dump()]
        self.assertIn(17, actions)
        self.assertIn(18, actions)

    def test_clone_requires_its_own_callback(self):
        engine = setup_fight()
        original_calls = []
        clone_calls = []
        observations = []

        def awake(ai_id, plant, trigger, turn):
            original_calls.append((ai_id, plant, trigger))
            return 0

        def act(idx, turn):
            if idx != 0:
                return 0
            self.assertEqual(engine.summon(idx, 99402, 73, ai_id=456), 1)
            missing = engine.clone()
            with self.assertRaisesRegex(RuntimeError, "plant AI dispatcher"):
                missing.move_toward(1, 73, 6)
            clone = engine.clone()

            def clone_awake(ai_id, plant, trigger, clone_turn):
                clone_calls.append((ai_id, plant, trigger))
                clone.use_chip(plant, 99401, clone.entity_cell(trigger))
                return 0

            clone.set_plant_awaken_callback(clone_awake)
            clone.move_toward(1, 73, 6)
            observations.append((clone.entity_hp(1), engine.entity_hp(1)))
            return 0

        engine.set_plant_awaken_callback(awake)
        engine.set_ai_callback(act)
        self.assertEqual(engine.run(), 0)
        self.assertEqual(original_calls, [(456, 2, 0)])
        self.assertEqual(clone_calls, [(456, 2, 1)])
        self.assertLess(observations[0][0], observations[0][1])
        self.assertEqual(observations[0][1], 100)

    def test_clone_during_awakening_can_act_and_finish(self):
        engine = setup_fight()
        observations = []
        clones = []

        def awake(ai_id, plant, trigger, turn):
            clone = engine.clone()
            clones.append(clone)
            attack_result = clone.use_chip(plant, 99401, clone.entity_cell(trigger))
            clone.finish_plant_awakening()
            clone_calls = []
            clone.set_plant_awaken_callback(
                lambda clone_id, clone_plant, clone_trigger, clone_turn:
                clone_calls.append((clone_id, clone_plant, clone_trigger)))
            clone.move_toward(1, 73, 6)
            observations.append((attack_result, clone.entity_hp(trigger),
                                 engine.entity_hp(trigger), clone_calls))
            return 0

        engine.set_plant_awaken_callback(awake)
        engine.set_ai_callback(
            lambda idx, turn: engine.summon(idx, 99402, 73, ai_id=654)
            if idx == 0 else 0)
        self.assertEqual(engine.run(), 0)
        self.assertEqual(len(observations), 1)
        attack_result, clone_hp, original_hp, clone_calls = observations[0]
        self.assertEqual(attack_result, 1)
        self.assertLess(clone_hp, original_hp)
        self.assertEqual(original_hp, 100)
        self.assertEqual(clone_calls, [(654, 2, 1)])
        with self.assertRaisesRegex(RuntimeError, "no cloned plant awakening"):
            clones[0].finish_plant_awakening()

    def test_initial_cooldown_ticks_on_plant_awakening(self):
        engine = setup_fight(initial_cooldown=1)
        results = []

        def awake(ai_id, plant, trigger, turn):
            results.append(engine.use_chip(plant, 99401, engine.entity_cell(trigger)))
            return 0

        def act(idx, turn):
            if idx == 0:
                engine.summon(idx, 99402, 73, ai_id=321)
                engine.move_toward(1, 73, 6)
            return 0

        engine.set_plant_awaken_callback(awake)
        engine.set_ai_callback(act)
        self.assertEqual(engine.run(), 0)
        self.assertEqual(results, [-3, 1])

    def test_summon_respects_existing_initial_cooldown(self):
        cases = [(False, 2, 1), (True, 2, -3), (True, 3, 1)]
        for team_cooldown, summon_turn, expected in cases:
            with self.subTest(team_cooldown=team_cooldown, summon_turn=summon_turn):
                engine = setup_fight(initial_cooldown=1, team_cooldown=team_cooldown)
                engine.set_max_turns(summon_turn)
                results = []
                engine.set_plant_awaken_callback(
                    lambda ai_id, plant, trigger, turn:
                    results.append(engine.use_chip(plant, 99401, engine.entity_cell(trigger))))
                engine.set_ai_callback(
                    lambda idx, turn: engine.summon(idx, 99402, 73, ai_id=321)
                    if idx == 0 and turn == summon_turn else 0)
                self.assertEqual(engine.run(), 0)
                self.assertEqual(results, [expected])

    def test_ordinary_summon_uses_its_ai_id(self):
        engine = setup_fight(awakening_zone=0)
        engine.set_max_turns(2)
        summons = []
        engine.set_summon_ai_callback(
            lambda ai_id, idx, turn: summons.append((ai_id, idx, turn)))
        engine.set_ai_callback(
            lambda idx, turn: engine.summon(idx, 99402, 73, ai_id=789)
            if idx == 0 and turn == 1 else 0)
        self.assertEqual(engine.run(), 0)
        self.assertTrue(summons)
        self.assertEqual(summons[0][:2], (789, 2))

    def test_ordinary_summon_missing_dispatcher_is_fatal(self):
        engine = setup_fight(awakening_zone=0)
        engine.set_max_turns(2)
        engine.set_ai_callback(
            lambda idx, turn: engine.summon(idx, 99402, 73, ai_id=789)
            if idx == 0 and turn == 1 else 0)
        with self.assertRaisesRegex(RuntimeError, "summon AI dispatcher"):
            engine.run()


if __name__ == "__main__":
    unittest.main()
