import json
import sys
import unittest
from pathlib import Path

from compare import CASES, first_difference, parse_results, run


class CompareTests(unittest.TestCase):
    def test_reports_first_different_action(self):
        java = {"actions": [[10, 0, [1, 2]], [17, 2, 0, 6]]}
        c = {"actions": [[10, 0, [1, 2]], [17, 2, 0, 5]]}
        self.assertEqual(first_difference(java, c), ("$.actions[1][3]", 6, 5))

    def test_rejects_missing_and_duplicate_fixtures(self):
        with self.assertRaisesRegex(ValueError, "Fixture set mismatch"):
            parse_results("")
        line = "C_PARITY " + json.dumps(
            {"case": "plant_walk", "values": [], "actions": [], "ops": {}}
        )
        with self.assertRaisesRegex(ValueError, "duplicate"):
            parse_results(line + "\n" + line)

    def test_keeps_unknown_actions(self):
        lines = [
            "C_PARITY " + json.dumps({
                "case": name, "values": [], "actions": [[9999, 1]], "ops": {}
            })
            for name in CASES
        ]
        self.assertEqual(parse_results("\n".join(lines))["plant_walk"]["actions"], [[9999, 1]])

    def test_action_count_and_types_are_compared(self):
        self.assertEqual(first_difference([[10]], [[10], [1000]]), ("$.length", 1, 2))
        self.assertEqual(first_difference([1], [True]), ("$[0]", 1, True))

    def test_subprocess_failure_surfaces_output(self):
        with self.assertRaisesRegex(RuntimeError, "fixture failed"):
            run([sys.executable, "-c", "import sys; print('fixture failed'); sys.exit(7)"], Path.cwd())


if __name__ == "__main__":
    unittest.main()
