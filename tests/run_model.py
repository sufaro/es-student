"""Reproduce course scenarios without a board, using compiled production logic.

All generated evidence is explicitly MODEL, including synthetic audio (not voice).
The USB ID/zero serial are protocol fixtures, not a claim of owning a Pico.
"""
import argparse
import ast
import csv
import hashlib
import importlib.util
import json
from pathlib import Path
import subprocess
import sys
from types import SimpleNamespace

ROOT = Path(__file__).resolve().parents[1]
MARKER = b"\n@MODEL-END\n"
NOTICE = "MODEL: host platform stubs; no physical board, measurements or recorded voice.\n"
BOARD = SimpleNamespace(vid=0x2E8A, pid=0x000A,
                        serial_number="0000000000000000", device="MODEL")


def scenarios(script):
    env = {"BACKSPACE": "\x7f", "RECONNECT": None}
    tree = ast.parse(script.read_text(encoding="utf-8-sig"))
    assignment = next(n for n in tree.body if isinstance(n, ast.Assign)
                      and any(isinstance(t, ast.Name) and t.id == "STEPS" for t in n.targets))
    return eval(compile(ast.Expression(assignment.value), str(script), "eval"), {}, env)


def execute(build, level, steps):
    executable = build / ("model_%d" % level)
    if sys.platform == "win32":
        executable = executable.with_suffix(".exe")
    commands = "".join("%d %s\n" % (round(seconds * 1e6), (frame or "").encode("ascii").hex())
                       for frame, seconds in steps)
    result = subprocess.run([str(executable)], input=commands.encode(), stdout=subprocess.PIPE, check=True)
    chunks = result.stdout.split(MARKER)
    assert len(chunks) == len(steps) + 1 and chunks[-1] == b"", "model transport framing"
    return chunks[:-1]


def header(task, project):
    return (NOTICE + "задание: %s\nпроект: %s\nустройство: 2e8a:000a\n"
            "серийный номер: 0000000000000000\nпорт: MODEL\n"
            "начало: 2026-11-12T00:00:00\n" % (task, project))


def exchange_log(out, build, level, project, task):
    script = ROOT / project / ("check-%s.py" % task.replace(".", "-"))
    steps = scenarios(script)
    # RECONNECT is a deadline, not a mandatory 15-second delay: model reboot at 2 s.
    steps = [(frame, 3 if frame is None else seconds) for frame, seconds in steps]
    chunks = execute(build, level, steps)
    lines, moment, answers = [], 0.0, 0
    for (frame, seconds), data in zip(steps, chunks):
        if frame is not None:
            printable = frame.replace("\x7f", "<BS>").replace("\b", "<BS>").replace("\r", "").replace("\n", "")
            lines.append("%8.3f --> %s\n" % (moment, printable))
        else:
            lines.append("%8.3f --- ждём перезапуска платы\n" % moment)
            lines.append("%8.3f --- плата снова на связи\n" % (moment + seconds))
        for reply in data.decode("ascii").replace("\r", "").splitlines():
            if reply.strip():
                lines.append("%8.3f <-- %s\n" % (moment + 0.025, reply.strip()))
                answers += 1
        moment += seconds
    path = out / project / ("device-%s.log" % task.replace(".", "-"))
    path.write_text(header(task, project) + "".join(lines) +
                    "итог: отправлено кадров %d, принято строк %d\n" % (len(steps), answers), encoding="utf-8")


def telemetry(out, build):
    project = out / "311-adc"
    plans = {
        "telemetry": [("charge_on\r", .01), ("tm_start 100\r", 10), ("tm_stop\r", .01)],
        "rc": [("tm_start 10\r", .5), ("charge_on\r", 5), ("charge_off\r", 5.5), ("tm_stop\r", .01)],
    }
    for name, steps in plans.items():
        records = []
        for data in execute(build, 2, steps):
            for line in data.decode("ascii").splitlines():
                if line.startswith("tm:"):
                    _, us, voltage, temp = line.split()
                    records.append((int(us), voltage, temp))
        assert len(records) > 90, "actual telemetry task produced too few records"
        with (project / (name + ".csv")).open("w", encoding="utf-8", newline="") as f:
            writer = csv.writer(f)
            writer.writerow(["time_s", "voltage_v", "temp_c"])
            for us, voltage, temp in records:
                writer.writerow(["%.6f" % ((us - records[0][0]) / 1e6), voltage, temp])
        script = "get_telemetry.py" if name == "telemetry" else "rc.py"
        args = [] if name == "telemetry" else ["--r", "10", "--c", "100"]
        subprocess.run([sys.executable, "-X", "utf8", str(ROOT / "311-adc" / script), *args,
                        "--plot", name + ".csv"], cwd=project, check=True)
    rc_log = project / "rc.log"
    rc_log.write_text(NOTICE + rc_log.read_text(encoding="utf-8"), encoding="utf-8")
    # Put provenance on the images too, without changing the course plotting script.
    import matplotlib.image as mpimg
    import matplotlib.pyplot as plt
    for name in plans:
        path = project / (name + ".png")
        image = mpimg.imread(path)
        fig = plt.figure(figsize=(8, 6))
        ax = fig.add_axes((0, .06, 1, .94))
        ax.imshow(image); ax.axis("off")
        fig.text(.5, .02, "MODEL — simulated data, no physical board", ha="center")
        fig.savefig(path, dpi=100); plt.close(fig)


def audio(out, build):
    spec = importlib.util.spec_from_file_location("recorder", ROOT / "321-audio-recorder/rec_audio.py")
    rec = importlib.util.module_from_spec(spec); spec.loader.exec_module(rec)
    project = out / "321-audio-recorder"
    for name, task, melody, bits in [("stream", "3.2.4", False, 16), ("melody", "3.2.5", True, 16),
                                     ("recording", "3.2.7", False, 16),
                                     ("recording-8bit", "3.2.7д", False, 8)]:
        events, length = rec.melody_events() if melody else ([], 4)
        command = "stream_start" if bits == 16 else "stream_start 8"
        steps = [(command + "\r", .01)]
        if melody:
            # Explicit GP20 -> GP26 model loopback, including silent gaps.
            steps.append(("generate_tone 0\r", .49))
            for i, (when, hz) in enumerate(events):
                end = events[i + 1][0] if i + 1 < len(events) else length + .5
                steps.append(("generate_tone %d\r" % hz, end - when))
            elapsed = length + 1
        else:
            steps.append(("", length - .01)); elapsed = length
        steps.append(("stream_stop\r", .01))
        chunks = execute(build, 3, steps)
        wav_name = name + ".wav" if name != "stream" else None
        writer = rec.WaveWriter(str(project / wav_name), bits, melody) if wav_name else None
        receiver = rec.Receiver(bits, writer.feed if writer else None, keep_samples=False)
        try:
            # Deliberately fragment at non-frame boundaries, as with USB reads.
            for data in chunks:
                for at in range(0, len(data), 137):
                    receiver.feed(data[at:at + 137])
        finally:
            if writer: writer.close()
        start, _ = rec.read_line_until_bytes(chunks[0], "stream: started")
        stop, _ = rec.read_line_until_bytes(chunks[-1], "stream: stopped")
        assert receiver.frames > (250 if bits == 16 else 150) and receiver.lost == 0 and stop
        assert receiver.sample_count == receiver.frames * (512 if bits == 16 else 1024)
        exchange = [("-->", command), ("<--", start)]
        if melody:
            exchange.append(("-->", "generate_tone: %d команд, MODEL" % (len(events) + 1)))
        exchange.extend([("-->", "stream_stop"), ("<--", stop)])
        path = project / (name + ".log")
        rec.write_log(str(path), BOARD, task, exchange, receiver, elapsed, wav_name,
                      writer.bias if writer else None)
        text = path.read_text(encoding="utf-8")
        # A fixed model epoch makes regeneration deterministic.
        text = "\n".join("начало: 2026-11-12T00:00:00" if line.startswith("начало:") else line
                         for line in text.splitlines()) + "\n"
        path.write_text(NOTICE + text, encoding="utf-8")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build", type=Path, default=ROOT / "tests/build")
    parser.add_argument("--output", type=Path, default=ROOT)
    args = parser.parse_args()
    out, build = args.output.resolve(), args.build.resolve()
    projects = [(1, "231-firmware-template", ["2.3.8"]),
                (2, "311-adc", ["3.1.1", "3.1.2", "3.1.3"]),
                (3, "321-audio-recorder", ["3.2.1", "3.2.2", "3.2.3", "3.2.6", "3.2.7"])]
    for level, project, tasks in projects:
        (out / project).mkdir(parents=True, exist_ok=True)
        for task in tasks:
            exchange_log(out, build, level, project, task)
    telemetry(out, build); audio(out, build)
    manifest = {"mode": "MODEL", "physical_board": False,
                "limitations": "Ideal RC tau=1s, ADC signals, DMA/timer timing and reset are modeled. "
                "Synthetic recording.wav is not recorded voice. USB ID and zero serial are fixtures.",
                "files": {}}
    for _, project, _ in projects:
        for path in sorted((out / project).iterdir()):
            if path.suffix in (".log", ".csv", ".png", ".wav"):
                manifest["files"][path.relative_to(out).as_posix()] = hashlib.sha256(path.read_bytes()).hexdigest()
    (out / "MODEL-MANIFEST.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    print("MODEL evidence generated; no physical board was tested.")


if __name__ == "__main__":
    main()
