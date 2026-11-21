"""Run a minimal 1v1 fight using the C engine's Python binding."""

from leekwars_c._engine import Engine


def main():
    engine = Engine()
    weapon_id = 37
    engine.add_weapon(
        item_id=weapon_id, name="Example weapon", cost=3,
        min_range=1, max_range=30, launch_type=7, area_id=1,
        los=False, max_uses=-1,
        effects=[(1, 100.0, 0.0, 0, 1 | 8, 0)],
    )

    for team in range(2):
        ident = team + 1
        cell = (72, 144)[team]
        engine.add_farmer(ident, f"Farmer {ident}", "fr")
        engine.add_team(ident, f"Team {ident}")
        engine.add_entity(
            team=team, fid=ident, name=f"Leek {ident}", level=1,
            life=100, tp=6, mp=0, strength=0, agility=0, frequency=100,
            wisdom=0, resistance=0, science=0, magic=0, cores=0, ram=0,
            farmer=ident, team_id=ident, weapons=[weapon_id], cell=cell,
        )

    engine.set_seed(1)
    engine.set_max_turns(10)
    engine.set_custom_map(obstacles={}, team1=[72], team2=[144])

    def shoot(entity, turn):
        target = 1 - entity
        if not engine.entity_alive(target):
            return 0
        if not engine.set_weapon(entity, weapon_id):
            return 0
        result = engine.use_weapon_on_cell(entity, engine.entity_cell(target))
        return int(result > 0)

    engine.set_ai_callback(shoot)
    result = engine.run()
    if result != 0:
        raise RuntimeError(f"Fight failed with error code {result}")
    print(f"Winner: {engine.winner}")
    for action in engine.stream_dump():
        print(action)


if __name__ == "__main__":
    main()
