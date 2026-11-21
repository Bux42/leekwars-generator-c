import unittest
import warnings

from leekwars_c._engine import Engine


def make_engine(callback):
    engine = Engine()
    for item_id, template in ((99001, 2201), (99002, 2202)):
        engine.add_weapon(
            item_id=item_id, name="test", cost=3, min_range=1, max_range=30,
            launch_type=7, area_id=1, los=False, max_uses=-1,
            effects=[(1, 10, 0, 0, 1 | 8, 0)], template_id=template,
        )
    for team in range(2):
        engine.add_farmer(team + 1, "farmer", "fr")
        engine.add_team(team + 1, "team")
        engine.add_entity(
            team=team, fid=team + 1, name="leek", level=1,
            life=1000, tp=6, mp=0, strength=0, agility=0, frequency=100,
            wisdom=0, resistance=0, science=0, magic=0, cores=0, ram=0,
            farmer=team + 1, team_id=team + 1, weapons=[99001],
            cell=(72, 144)[team],
        )
    engine.set_seed(1)
    engine.set_max_turns(1)
    engine.set_custom_map(obstacles={}, team1=[72], team2=[144])
    engine.set_ai_callback(lambda idx, turn: callback(engine, idx, turn))
    return engine


class WeaponApiTests(unittest.TestCase):
    def test_explicit_selection_and_use(self):
        results = []

        def act(engine, idx, turn):
            if idx != 0:
                return 0
            target = engine.entity_cell(1)
            results.append(engine.use_weapon_on_cell(idx, target))
            with warnings.catch_warnings(record=True) as caught:
                warnings.simplefilter("always")
                results.append(engine.set_weapon(idx, 99003))
                results.append(engine.set_weapon(idx, 99002))
                results.append(len(caught))
            results.append(engine.set_weapon(idx, 99001))
            results.append(engine.set_weapon(idx, 99001))
            results.append(engine.use_weapon_on_cell(idx, engine.entity_cell(idx)))
            results.append(engine.use_weapon_on_cell(idx, target))
            results.append(engine.use_weapon_on_cell(idx, target))
            results.append(engine.set_weapon(idx, 99001))
            results.append(engine.set_weapon(idx, 99001))
            results.append(engine.entity_used_tp(idx))
            return 0

        engine = make_engine(act)
        self.assertEqual(engine.run(), 0)
        self.assertEqual(results, [-1, False, False, 2, True, True, -1, 1, -2, True, False, 6])
        weapon_actions = [a for a in engine.stream_dump() if a["type"] in (13, 16)]
        self.assertEqual([a["type"] for a in weapon_actions], [13, 13, 16, 13])
        self.assertEqual(weapon_actions[0]["args"], [2201])
        self.assertEqual(weapon_actions[1]["args"], [2201])

    def test_legacy_fire_weapon_still_equips_without_cost(self):
        results = []

        def act(engine, idx, turn):
            if idx == 0:
                results.append(engine.fire_weapon(idx, 99001, engine.entity_cell(1)))
                results.append(engine.entity_used_tp(idx))
            return 0

        engine = make_engine(act)
        self.assertEqual(engine.run(), 0)
        self.assertEqual(results, [1, 3])
        self.assertFalse(any(a["type"] == 13 for a in engine.stream_dump()))


if __name__ == "__main__":
    unittest.main()
