#!/usr/bin/env python3
"""Show desktop playback level on PD8..PD15 using only USB ST-LINK."""
import argparse
import array
import math
import pathlib
import queue
import re
import shutil
import socket
import subprocess
import sys
import threading
import time
from stlink import GDB, latest_tool


def pcm_level(data, previous, gain):
    samples = array.array("h")
    samples.frombytes(data)
    if sys.byteorder != "little":
        samples.byteswap()
    rms = math.sqrt(sum(s * s for s in samples) / max(1, len(samples))) / 32768
    db = 20 * math.log10(max(rms * gain, 1e-8))
    value = max(0.0, min(1.0, (db + 48) / 42))
    # Fast attack and slower fall make the bar follow music visibly.
    value = max(value, previous * 0.85)
    count = min(8, math.ceil(value * 8)) if value > 0.025 else 0
    return (1 << count) - 1, value


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--gain", type=float, default=1.0)
    parser.add_argument("--device", default="@DEFAULT_MONITOR@")
    parser.add_argument("--preview", action="store_true", help="Only show audio level in terminal")
    parser.add_argument("--test-leds", action="store_true", help="Test all 8 LED levels and verify GPIO output")
    parser.add_argument("--elf", type=pathlib.Path,
                        default=pathlib.Path(__file__).resolve().parents[1] / "build/Debug/Lab1_20261.elf")
    args = parser.parse_args()
    for tool in (["parec"] if args.preview else ["parec", "arm-none-eabi-gdb"]):
        if not shutil.which(tool):
            raise RuntimeError("Missing command: " + tool)
    server = audio = gdb = None
    log = None
    try:
        if not args.preview:
            if not args.elf.is_file():
                raise RuntimeError("Build the project first; ELF file is missing")
            server_bin = latest_tool(".local/share/stm32cube/bundles/stlink-gdbserver/*/bin/ST-LINK_gdbserver")
            programmer_bin = latest_tool(".local/share/stm32cube/bundles/programmer/*/bin/STM32_Programmer_CLI")
            with socket.socket() as s:
                s.bind(("127.0.0.1", 0))
                port = s.getsockname()[1]
            log_path = args.elf.parent / "audio-stlink.log"
            log = log_path.open("w")
            server = subprocess.Popen([server_bin, "-d", "-k", "--frequency", "1000", "-p", str(port),
                                       "-cp", str(pathlib.Path(programmer_bin).parent)],
                                      stdout=log, stderr=subprocess.STDOUT)
            # Do not probe the socket: the server accepts one debugger connection.
            time.sleep(1.5)
            if server.poll() is not None:
                log.flush()
                details = log_path.read_text()[-1500:]
                raise RuntimeError(f"ST-LINK server failed. Stop VS Code debug first.\n{details}")
            gdb = GDB(args.elf)
            gdb.command("-gdb-set mi-async on")
            gdb.command(f"-target-select remote 127.0.0.1:{port}")
            gdb.halt()
            print("Loading firmware through ST-LINK...", flush=True)
            gdb.command("-target-download", timeout=60)
            gdb.command('-interpreter-exec console "monitor reset"')
            # Startup clears BSS. Wait until main before setting the mailbox.
            gdb.command("-break-insert -t main")
            gdb.resume()
            deadline = time.monotonic() + 10
            while not gdb.stopped:
                gdb.next_line(deadline)
            gdb.set_value("AudioLED_Enabled", 1)
            gdb.set_value("AudioLED_Mask", 0)
            gdb.resume()
            if args.test_leds:
                for count in range(9):
                    mask = (1 << count) - 1
                    gdb.halt()
                    gdb.set_value("AudioLED_Mask", mask)
                    gdb.set_value("AudioLED_Sequence", count + 1)
                    gdb.resume()
                    time.sleep(0.15)
                    gdb.halt()
                    result = gdb.command('-data-evaluate-expression "(*(unsigned int*)0x40020c14 >> 8) & 255"')
                    match = re.search(r'value="(\d+)"', result)
                    if not match or int(match.group(1)) != mask:
                        raise RuntimeError(f"GPIO check failed: wanted {mask}, received {result}")
                    print(f"Verified {count}/8 LEDs (GPIOD output 0x{mask:02x})", flush=True)
                    gdb.resume()
                return
        audio = subprocess.Popen(["parec", "--raw", "--format=s16le", "--rate=16000",
                                  "--channels=1", "--latency-msec=50", "--device=" + args.device],
                                 stdout=subprocess.PIPE)
        print("Play music. Ctrl+C to stop. LED bar: PD8 -> PD15", flush=True)
        frames = queue.Queue(maxsize=1)

        def capture():
            while True:
                data = audio.stdout.read(1600)
                if len(data) != 1600:
                    data = None
                try:
                    frames.get_nowait()
                except queue.Empty:
                    pass
                frames.put(data)
                if data is None:
                    return

        threading.Thread(target=capture, daemon=True).start()
        previous = 0.0
        sequence = 0
        while True:
            # Keep only the newest 50 ms frame when debug writes take longer.
            try:
                data = frames.get(timeout=5)
            except queue.Empty:
                raise RuntimeError("No audio data received for 5 seconds")
            if data is None:
                raise RuntimeError("Audio capture stopped; check PipeWire/PulseAudio")
            mask, previous = pcm_level(data, previous, args.gain)
            if gdb:
                gdb.halt()
                sequence = (sequence + 1) & 0xFFFFFFFF
                gdb.set_value("AudioLED_Enabled", 1)
                gdb.set_value("AudioLED_Mask", mask)
                gdb.set_value("AudioLED_Sequence", sequence)
                gdb.resume()
            print("\r[" + "#" * mask.bit_count() + " " * (8 - mask.bit_count()) + "]", end="", flush=True)
    except KeyboardInterrupt:
        print("\nStopped.")
    finally:
        if audio:
            audio.terminate()
            audio.wait()
        if gdb:
            try:
                gdb.halt()
                gdb.set_value("AudioLED_Mask", 0)
                gdb.set_value("AudioLED_Sequence", 0)
                gdb.resume()
            except RuntimeError:
                pass
            gdb.close()
        if server:
            server.terminate()
            try:
                server.wait(timeout=3)
            except subprocess.TimeoutExpired:
                server.kill()
                server.wait()
        if log:
            log.close()


if __name__ == "__main__":
    try:
        main()
    except (RuntimeError, OSError) as exc:
        print(f"\nError: {exc}", file=sys.stderr)
        sys.exit(1)
