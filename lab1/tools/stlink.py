"""Shared GDB/MI support for the STM32 ST-LINK tools."""
import glob
import json
import pathlib
import queue
import subprocess
import threading
import time

class GDB:
    def __init__(self, elf):
        self.process = subprocess.Popen(
            ["arm-none-eabi-gdb", "--quiet", "--nx", "--interpreter=mi2", str(elf)],
            stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
            text=True, bufsize=1)
        self.lines = queue.Queue()
        self.token = 0
        self.stopped = False
        threading.Thread(target=self._read, daemon=True).start()

    def _read(self):
        for line in self.process.stdout:
            self.lines.put(line.strip())
        self.lines.put(None)

    def next_line(self, deadline):
        try:
            line = self.lines.get(timeout=max(0.01, deadline - time.monotonic()))
        except queue.Empty:
            raise RuntimeError("GDB timed out; check ST-LINK connection")
        if line is None:
            raise RuntimeError("GDB exited unexpectedly")
        if line.startswith("*stopped"):
            self.stopped = True
        return line

    def command(self, command, timeout=10):
        self.token += 1
        prefix = str(self.token) + "^"
        self.process.stdin.write(str(self.token) + command + "\n")
        self.process.stdin.flush()
        deadline = time.monotonic() + timeout
        while True:
            line = self.next_line(deadline)
            if line.startswith(prefix):
                if line.startswith(prefix + "error"):
                    raise RuntimeError(line)
                return line

    def halt(self):
        if not self.stopped:
            self.command("-exec-interrupt --all")
            deadline = time.monotonic() + 5
            while not self.stopped:
                self.next_line(deadline)

    def resume(self):
        self.stopped = False
        self.command("-exec-continue")

    def set_value(self, name, value):
        self.command('-interpreter-exec console ' + json.dumps(f"set var {name} = {value}"))

    def close(self):
        self.process.terminate()
        try:
            self.process.wait(timeout=3)
        except subprocess.TimeoutExpired:
            self.process.kill()
            self.process.wait()


def latest_tool(pattern):
    paths = glob.glob(str(pathlib.Path.home() / pattern))
    if not paths:
        raise RuntimeError("STM32Cube tool not found: " + pattern)
    return max(paths, key=lambda p: pathlib.Path(p).stat().st_mtime)


