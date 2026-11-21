import unittest

from leekwars_c._engine import Engine


class SummonErrorTests(unittest.TestCase):
    def test_summon_on_clone_does_not_change_original(self):
        engine = Engine()
        stats = [(1, 1)] * 9
        engine.add_summon_template(99300, "clone summon", *stats, [])
        engine.add_chip(
            item_id=99301, name="summon", cost=1, min_range=1,
            max_range=100, launch_type=7, area_id=1, los=False,
            max_uses=-1, cooldown=0, initial_cooldown=0,
            team_cooldown=False, level=1, chip_type=8,
            effects=[(14, 99300, 0, 0, 1, 0)],
        )
        for team in range(2):
            ident = team + 1
            engine.add_farmer(ident, "farmer", "fr")
            engine.add_team(ident, "team")
            engine.add_entity(
                team=team, fid=ident, name="leek", level=1, life=100,
                tp=6, mp=0, strength=0, agility=0, frequency=100, wisdom=0,
                resistance=0, science=0, magic=0, cores=0, ram=1,
                farmer=ident, team_id=ident, weapons=[], chips=[99301],
                cell=(72, 144)[team],
            )
        engine.set_custom_map(obstacles={}, team1=[72], team2=[144])
        engine.set_max_turns(1)
        observed = []

        def act(idx, turn):
            if idx == 0:
                clone = engine.clone()
                observed.append((clone.use_chip(idx, 99301, 73),
                                 clone.n_entities(), engine.n_entities()))
            return 0

        engine.set_ai_callback(act)
        self.assertEqual(engine.run(), 0)
        self.assertEqual(observed, [(1, 3, 2)])

    def test_summon_template_registration_rejects_bad_chip_lists(self):
        engine = Engine()
        stats = [(1, 1)] * 9
        engine.add_summon_template(99100, "valid", *stats, [])
        with self.assertRaisesRegex(ValueError, "chip 999998 is not registered"):
            engine.add_summon_template(99100, "invalid", *stats, [999998])
        engine.add_chip(
            item_id=99101, name="summon", cost=1, min_range=1,
            max_range=100, launch_type=7, area_id=1, los=False,
            max_uses=-1, cooldown=0, initial_cooldown=0,
            team_cooldown=False, level=1, chip_type=8,
            effects=[(14, 99100, 0, 0, 1, 0)],
        )
        engine.add_summon_template(99102, "many chips", *stats, [99101] * 80)
        for team in range(2):
            ident = team + 1
            engine.add_farmer(ident, "farmer", "fr")
            engine.add_team(ident, "team")
            engine.add_entity(
                team=team, fid=ident, name="leek", level=1, life=100,
                tp=6, mp=0, strength=0, agility=0, frequency=100, wisdom=0,
                resistance=0, science=0, magic=0, cores=0, ram=1,
                farmer=ident, team_id=ident, weapons=[], chips=[99101],
                cell=(72, 144)[team],
            )
        engine.set_custom_map(obstacles={}, team1=[72], team2=[144])
        engine.set_max_turns(1)
        observed = []

        def act(idx, turn):
            if idx == 0:
                engine.add_chip(
                    item_id=99101, name="replacement", cost=1, min_range=1,
                    max_range=100, launch_type=7, area_id=1, los=False,
                    max_uses=-1, cooldown=0, initial_cooldown=0,
                    team_cooldown=False, level=1, chip_type=1,
                    effects=[(1, 50, 0, 0, 1, 0)],
                )
                engine.add_chip(
                    item_id=99103, name="unequipped", cost=1, min_range=1,
                    max_range=100, launch_type=7, area_id=1, los=False,
                    max_uses=-1, cooldown=0, initial_cooldown=0,
                    team_cooldown=False, level=1, chip_type=1,
                    effects=[(1, 50, 0, 0, 1, 0)],
                )
                observed.append(engine.use_chip(idx, 99103, 73))
                observed.append((engine.use_chip(idx, 99101, 73), engine.n_entities()))
            return 0

        engine.set_ai_callback(act)
        self.assertEqual(engine.run(), 0)
        self.assertEqual(observed, [-1, (1, 3)])

    def test_summon_registry_accepts_more_than_64_templates(self):
        engine = Engine()
        stats = [(1, 1)] * 9
        for ident in range(99200, 99265):
            engine.add_summon_template(ident, "template", *stats, [])

    def test_missing_template_aborts_even_if_callback_catches_error(self):
        for catch_error in (False, True):
            with self.subTest(catch_error=catch_error):
                engine = Engine()
                engine.add_chip(
                    item_id=99003, name="summon", cost=1, min_range=1,
                    max_range=100, launch_type=7, area_id=1, los=False,
                    max_uses=-1, cooldown=0, initial_cooldown=0,
                    team_cooldown=False, level=1, chip_type=8,
                    effects=[(14, 999999, 0, 0, 1, 0)],
                )
                for team in range(2):
                    ident = team + 1
                    engine.add_farmer(ident, "farmer", "fr")
                    engine.add_team(ident, "team")
                    engine.add_entity(
                        team=team, fid=ident, name="leek", level=1,
                        life=100, tp=6, mp=0, strength=0, agility=0, frequency=100,
                        wisdom=0, resistance=0, science=0, magic=0, cores=0, ram=1,
                        farmer=ident, team_id=ident, weapons=[], chips=[99003],
                        cell=(72, 144)[team],
                    )
                engine.set_custom_map(obstacles={}, team1=[72], team2=[144])
                engine.set_max_turns(1)
                calls = []

                def act(idx, turn):
                    calls.append(idx)
                    if idx == 0:
                        if catch_error:
                            with self.assertRaisesRegex(RuntimeError, "999999"):
                                engine.use_chip(idx, 99003, 73)
                            with self.assertRaisesRegex(RuntimeError, "999999"):
                                engine.move_toward(idx, 144)
                        else:
                            engine.use_chip(idx, 99003, 73)
                    return 0

                engine.set_ai_callback(act)
                with self.assertRaisesRegex(RuntimeError, "summon template 999999"):
                    engine.run()
                self.assertEqual(calls, [0])
                self.assertEqual(engine.n_entities(), 2)
                self.assertEqual(engine.entity_used_tp(0), 0)


if __name__ == "__main__":
    unittest.main()
