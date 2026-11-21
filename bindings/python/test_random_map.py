import unittest

from leekwars_c._engine import Engine


class RandomMapTests(unittest.TestCase):
    def test_seeded_two_entity_fight(self):
        for seed in (1, 2, 42):
            with self.subTest(seed=seed):
                engine = Engine()
                engine.add_weapon(
                    item_id=99001, name="test", cost=3, min_range=1, max_range=100,
                    launch_type=7, area_id=1, los=False, max_uses=-1,
                    effects=[(1, 10, 0, 0, 1 | 8, 0)],
                )
                for team in range(2):
                    ident = team + 1
                    engine.add_farmer(ident, "farmer", "fr")
                    engine.add_team(ident, "team")
                    engine.add_entity(
                        team=team, fid=ident, name="leek", level=1,
                        life=100, tp=6, mp=0, strength=0, agility=0, frequency=100,
                        wisdom=0, resistance=0, science=0, magic=0, cores=0, ram=0,
                        farmer=ident, team_id=ident, weapons=[99001],
                    )
                engine.set_seed(seed)
                engine.set_max_turns(1)
                actions = []

                def act(idx, turn):
                    if idx == 0:
                        with self.assertRaisesRegex(RuntimeError, "engine is already running"):
                            engine.run()
                        actions.append(engine.set_weapon(idx, 99001))
                        actions.append(engine.use_weapon_on_cell(idx, engine.entity_cell(1)))
                    return 0

                engine.set_ai_callback(act)
                self.assertEqual(engine.run(), 0)
                cells = [engine.entity_cell(i) for i in range(2)]
                self.assertTrue(all(cell >= 0 for cell in cells))
                self.assertNotEqual(cells[0], cells[1])
                self.assertEqual(actions, [True, 1])
                self.assertEqual(engine.entity_hp(1), 90)
                clone = engine.clone()
                self.assertEqual(engine.run(), 0)
                self.assertEqual(clone.entity_hp(1), 90)
                engine.set_ai_callback(None)
                del engine
                self.assertEqual(clone.entity_hp(1), 90)
                with self.assertRaisesRegex(RuntimeError, "cannot run a cloned engine"):
                    clone.run()


if __name__ == "__main__":
    unittest.main()
