"""Offline protocol tests; generated samples are test fixtures, not device logs."""
import importlib.util
from pathlib import Path
import struct
import tempfile
import unittest
import wave

SOURCE = Path(__file__).resolve().parents[1] / "321-audio-recorder" / "rec_audio.py"
spec = importlib.util.spec_from_file_location("rec_audio", SOURCE)
audio = importlib.util.module_from_spec(spec)
spec.loader.exec_module(audio)


def frame(number, bits=16):
    count = 512 if bits == 16 else 1024
    data = struct.pack("<512H", *range(512)) if bits == 16 else bytes(range(256)) * 4
    return struct.pack("<HHH", 0x5AA5, number, count) + data


class ReceiverTests(unittest.TestCase):
    def test_fragments_noise_and_sequence_wrap(self):
        receiver = audio.Receiver(16)
        wire = b"uptime: 100 ms\r\n" + frame(65535) + b"echo\r\n" + frame(0) + frame(3)
        for byte in wire:
            receiver.feed(bytes([byte]))
        self.assertEqual(receiver.frames, 3)
        self.assertEqual(receiver.lost, 2)
        self.assertEqual(receiver.sample_count, 1536)
        self.assertEqual(receiver.samples[:512], list(range(512)))
        self.assertEqual(receiver.sample_max, 511)
        self.assertEqual(receiver.sample_min, 0)

    def test_invalid_header_and_8_bit_payload(self):
        receiver = audio.Receiver(8)
        receiver.feed(b"\xa5\x5a\0\0\1\0bad header" + frame(20, 8))
        self.assertEqual(receiver.frames, 1)
        self.assertEqual(receiver.samples, list(range(256)) * 4)

    def test_bounded_memory(self):
        chunks = 0

        def consume(samples):
            nonlocal chunks
            chunks += 1
            self.assertEqual(len(samples), 512)

        receiver = audio.Receiver(16, consume, keep_samples=False)
        for i in range(1000):
            receiver.feed(frame(i))
        self.assertEqual(chunks, 1000)
        self.assertEqual(receiver.sample_count, 512000)
        self.assertEqual(receiver.samples, [])
        self.assertLess(len(receiver.buffer), 1030)

    def test_incremental_wav_and_short_recording(self):
        with tempfile.TemporaryDirectory() as directory:
            for bits in (8, 16):
                path = str(Path(directory) / (str(bits) + ".wav"))
                writer = audio.WaveWriter(path, bits, False)
                mid = 128 if bits == 8 else 2048
                for _ in range(15):
                    writer.feed([mid] * 512)
                writer.close()
                with wave.open(path, "rb") as wav:
                    self.assertEqual(wav.getnframes(), 7680)
                    self.assertEqual(wav.getframerate(), 44100)
                    self.assertEqual(wav.getsampwidth(), bits // 8)
                    expected = b"\x80" if bits == 8 else b"\0\0"
                    self.assertEqual(wav.readframes(7680), expected * 7680)
            path = str(Path(directory) / "short.wav")
            writer = audio.WaveWriter(path, 16, False)
            writer.feed([2000] * 10)
            writer.close()
            with wave.open(path, "rb") as wav:
                self.assertEqual(wav.getnframes(), 10)

    def test_filter_continuity_between_chunks(self):
        samples = [0, 4095] * 300
        with tempfile.TemporaryDirectory() as directory:
            paths = [str(Path(directory) / (str(i) + ".wav")) for i in range(2)]
            audio.save_wav(paths[0], samples, 16, True)
            writer = audio.WaveWriter(paths[1], 16, True)
            for i in range(0, len(samples), 37):
                writer.feed(samples[i:i + 37])
            writer.close()
            self.assertEqual(Path(paths[0]).read_bytes(), Path(paths[1]).read_bytes())


if __name__ == "__main__":
    unittest.main()
