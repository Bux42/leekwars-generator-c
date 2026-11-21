import unittest

from leekwars_c._engine import Engine


class StartEffectsTests(unittest.TestCase):
    def test_start_effect_is_applied_during_fight_initialization(self):
        engine = Engine()
        for team in range(2):
            engine.add_farmer(team + 1, "farmer", "fr")
            engine.add_team(team + 1, "team")
            engine.add_entity(
                team=team, fid=team + 1, name="leek", level=1,
                life=1000, tp=6, mp=0, strength=200, agility=0, frequency=100,
                wisdom=0, resistance=0, science=0, magic=0, cores=0, ram=0,
                farmer=team + 1, team_id=team + 1, weapons=[],
                cell=(72, 144)[team],
                start_effects=[(65, 50)] if team == 0 else None,
            )

        engine.set_seed(1)
        engine.set_max_turns(0)
        engine.set_custom_map(obstacles={}, team1=[72], team2=[144])
        engine.set_ai_callback(lambda _idx, _turn: 0)

        self.assertEqual(engine.run(), 0)
        self.assertTrue(any(
            action["type"] == 302 and 65 in action["args"]
            for action in engine.stream_dump()
        ))


if __name__ == "__main__":
    unittest.main()
