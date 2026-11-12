import copy
import unittest
from mark_model_report import mark, verify


class ModelReportTests(unittest.TestCase):
    def test_provenance_preserves_failures_counts_and_course_items(self):
        original = {"title": "Задание 3.1.1", "passed": 7, "total": 9, "status": "failed",
                    "warned": 0, "items": [{"ok": False, "status": "fail", "text": "failure"}]}
        report = mark(copy.deepcopy(original))
        for key in ("passed", "total", "status"):
            self.assertEqual(report[key], original[key])
        self.assertEqual(report["items"][1:], original["items"])
        self.assertTrue(verify(report))
        self.assertEqual(report["warned"], 1)
        self.assertEqual(mark(copy.deepcopy(report)), report)
        self.assertFalse(verify(original))


if __name__ == "__main__":
    unittest.main()
