"""Compare deterministic C fixtures with the pinned Java generator."""

import argparse
import json
import subprocess
import sys
from pathlib import Path

JAVA_COMMIT = "9edc5b96cf5ae41d1b0b6eb7b482db50d98fca77"
LEEKSCRIPT_COMMIT = "57186ebdba8cec28e78c24a868fe0cc4c3dd876b"
CASES = {
    "superinfection_mixed",
    "superinfection_infinite",
    "superinfection_invincible",
    "plant_walk",
    "plant_slide",
    "plant_teleport",
    "plant_no_entry",
    "operations_metadata",
    "raw_shackle_strength",
    "damage_to_resistance_replace",
    "damage_to_resistance_stack",
    "periodic_poison",
    "periodic_aftereffect",
    "periodic_heal",
    "start_effects_resurrection",
    "idle_turn_flow",
    "turret_start_fight",
}
PREFIX = "C_PARITY "


def run(command, cwd):
    result = subprocess.run(command, cwd=cwd, text=True, capture_output=True)
    if result.returncode:
        raise RuntimeError(
            f"Command failed ({result.returncode}): {command!r}\n"
            f"{result.stdout}\n{result.stderr}"
        )
    return result.stdout


def verify_checkout(path, commit):
    actual = run(["git", "rev-parse", "HEAD"], path).strip()
    if actual != commit:
        raise RuntimeError(f"{path}: expected commit {commit}, got {actual}")
    run(["git", "diff", "--quiet"], path)
    run(["git", "diff", "--cached", "--quiet"], path)
    untracked = run(
        ["git", "ls-files", "--others", "--exclude-standard", "--", "src/main"],
        path,
    )
    if untracked.strip():
        raise RuntimeError(f"{path}: untracked production source files:\n{untracked}")


def parse_results(output):
    results = {}
    for line in output.splitlines():
        line = line.strip()
        if not line.startswith(PREFIX):
            continue
        record = json.loads(line[len(PREFIX):])
        if not isinstance(record, dict) or set(record) != {"case", "values", "actions", "ops"}:
            raise ValueError(f"Invalid parity record: {record!r}")
        name = record["case"]
        if not isinstance(name, str) or name in results:
            raise ValueError(f"Invalid or duplicate fixture name: {name!r}")
        if (not isinstance(record["values"], list) or
                not isinstance(record["actions"], list) or not isinstance(record["ops"], dict)):
            raise ValueError(f"Invalid values, actions, or ops in {name}")
        results[name] = {
            "values": record["values"],
            "actions": record["actions"],
            "ops": record["ops"],
        }
    if set(results) != CASES:
        raise ValueError(
            f"Fixture set mismatch: missing={sorted(CASES - results.keys())}, "
            f"unexpected={sorted(results.keys() - CASES)}"
        )
    return results


def first_difference(expected, actual, path="$"):
    if type(expected) is not type(actual):
        return path, expected, actual
    if isinstance(expected, dict):
        if expected.keys() != actual.keys():
            return path, sorted(expected), sorted(actual)
        for key in expected:
            difference = first_difference(expected[key], actual[key], f"{path}.{key}")
            if difference:
                return difference
    elif isinstance(expected, list):
        for index, (left, right) in enumerate(zip(expected, actual)):
            difference = first_difference(left, right, f"{path}[{index}]")
            if difference:
                return difference
        if len(expected) != len(actual):
            return f"{path}.length", len(expected), len(actual)
    elif expected != actual:
        return path, expected, actual
    return None


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--reference", type=Path, required=True, help="Pinned Java generator checkout")
    parser.add_argument("--c-probe", type=Path, required=True, help="Built lw_java_parity_probe executable")
    parser.add_argument("--gradle", default="gradle", help="Gradle executable")
    parser.add_argument("--offline", action="store_true", help="Run Gradle without network access")
    args = parser.parse_args()
    reference = args.reference.resolve()
    probe = args.c_probe.resolve()
    fixture_dir = Path(__file__).resolve().parent
    verify_checkout(reference, JAVA_COMMIT)
    verify_checkout(reference / "leekscript", LEEKSCRIPT_COMMIT)

    command = [
        args.gradle, ":test", "--tests", "test.CParityFixtures", "--no-daemon",
        "--rerun-tasks", "--init-script", str(fixture_dir / "init.gradle"),
        f"-PcParityFixtures={fixture_dir / 'java'}",
    ]
    if args.offline:
        command.append("--offline")
    java = parse_results(run(command, reference))
    c = parse_results(run([str(probe)], fixture_dir))
    failed = False
    for name in sorted(CASES):
        difference = first_difference(java[name], c[name])
        if difference:
            path, expected, actual = difference
            print(f"FAIL {name} {path}: Java={expected!r}, C={actual!r}")
            failed = True
        else:
            print(f"PASS {name}")
    return 1 if failed else 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (OSError, ValueError, RuntimeError) as error:
        print(f"Parity check failed: {error}", file=sys.stderr)
        sys.exit(2)
