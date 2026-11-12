"""Attach provenance to the course report; preserve every test outcome and count."""
import argparse
import json
from pathlib import Path

NOTICE = ("MODEL: программная модель без физической платы. Логи, измерения, "
          "тайминги и аудио синтетические; работа на оборудовании не проверена.")


def mark(report):
    if not report["title"].startswith("MODEL — "):
        report["title"] = "MODEL — " + report["title"]
    if not any(item.get("text") == NOTICE for item in report["items"]):
        report["items"].insert(0, {"ok": False, "status": "warn", "text": NOTICE})
        report["warned"] += 1
    return report


def verify(report):
    return (report["title"].startswith("MODEL — ")
            and report["items"][0] == {"ok": False, "status": "warn", "text": NOTICE})


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--verify", action="store_true")
    parser.add_argument("path", type=Path)
    args = parser.parse_args()
    report = json.loads(args.path.read_text(encoding="utf-8"))
    if args.verify:
        if not verify(report):
            raise SystemExit("MODEL provenance is missing: refusing to submit report")
    else:
        args.path.write_text(json.dumps(mark(report), ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print("MODEL report: physical hardware was not tested.")


if __name__ == "__main__":
    main()
