#!/usr/bin/env python3
"""Load exercise 3 and print NEC frames using the existing USB ST-LINK."""
import argparse
import pathlib
import queue
import re
import socket
import subprocess
import sys
import threading
import time
from stlink import GDB, latest_tool


class PuttyBridge:
    """Local Raw TCP stream for PuTTY; keep recent messages for reconnects."""
    def __init__(self, port):
        self.lock = threading.Lock()
        self.clients = []
        self.history = []
        self.listener = socket.socket()
        self.listener.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.listener.bind(('127.0.0.1', port))
        self.listener.listen(4)
        self.listener.settimeout(0.5)
        self.closed = False
        self.thread = threading.Thread(target=self.accept, daemon=True)
        self.thread.start()

    def accept(self):
        while not self.closed:
            try:
                client, _ = self.listener.accept()
            except socket.timeout:
                continue
            except OSError:
                return
            client.settimeout(0.5)
            with self.lock:
                try:
                    client.sendall(b'IR remote monitor - USB ST-LINK\r\n' +
                                   b''.join(self.history))
                    self.clients.append(client)
                except OSError:
                    client.close()

    def emit(self, message):
        print(message, flush=True)
        data = (message + '\r\n').encode('utf-8')
        with self.lock:
            self.history.append(data)
            self.history = self.history[-40:]
            for client in self.clients[:]:
                try:
                    client.sendall(data)
                except OSError:
                    self.clients.remove(client)
                    client.close()

    def close(self):
        self.closed = True
        self.listener.close()
        self.thread.join(timeout=1)
        with self.lock:
            for client in self.clients:
                client.close()


def value(gdb, expression):
    response = gdb.command('-data-evaluate-expression "' + expression + '"')
    match = re.search(r'value="(\d+)', response)
    if not match:
        raise RuntimeError("Cannot read value: " + response)
    return int(match.group(1))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--seconds', type=float, default=0,
                        help='Stop after this many seconds; default: until Ctrl+C')
    parser.add_argument('--tcp-port', type=int, default=9000,
                        help='PuTTY Raw connection port on localhost (default 9000)')
    parser.add_argument('--putty', action='store_true', help='Open PuTTY automatically')
    parser.add_argument('--exercise', choices=['IR', 'Remote'], default='IR')
    args = parser.parse_args()
    root = pathlib.Path(__file__).resolve().parents[1]
    elf = root / 'build' / args.exercise / 'Lab1_20261.elf'
    if not elf.is_file():
        raise RuntimeError(f'Build first: cmake --preset {args.exercise} && cmake --build --preset {args.exercise}')
    server_bin = latest_tool('.local/share/stm32cube/bundles/stlink-gdbserver/*/bin/ST-LINK_gdbserver')
    programmer = latest_tool('.local/share/stm32cube/bundles/programmer/*/bin/STM32_Programmer_CLI')
    with socket.socket() as sock:
        sock.bind(('127.0.0.1', 0))
        port = sock.getsockname()[1]
    server = gdb = bridge = None
    with (elf.parent / 'ir-stlink.log').open('w') as log:
        try:
            bridge = PuttyBridge(args.tcp_port)
            bridge.emit(f'PuTTY: Raw / 127.0.0.1 / port {args.tcp_port}')
            if args.putty:
                subprocess.Popen(['putty', '-raw', '127.0.0.1', '-P', str(args.tcp_port)])
            server = subprocess.Popen(
                [server_bin, '-d', '-k', '--frequency', '1000', '-p', str(port),
                 '-cp', str(pathlib.Path(programmer).parent)],
                stdout=log, stderr=subprocess.STDOUT)
            time.sleep(1.5)
            if server.poll() is not None:
                log.flush()
                raise RuntimeError('ST-LINK server failed; stop other debug sessions.\n' +
                                   (elf.parent / 'ir-stlink.log').read_text()[-1800:])
            gdb = GDB(elf)
            gdb.command('-gdb-set mi-async on')
            gdb.command(f'-target-select remote 127.0.0.1:{port}')
            gdb.halt()
            bridge.emit(f'Loading {args.exercise} through ST-LINK...')
            gdb.command('-target-download', timeout=60)
            gdb.command('-interpreter-exec console "monitor reset"')
            gdb.command('-break-insert IR_FrameReady')
            gdb.resume()
            bridge.emit('READY. Press remote towards HS0038 (OUT -> PG5). Ctrl+C to stop monitor.')
            started = time.monotonic()
            while not args.seconds or time.monotonic() - started < args.seconds:
                # Wait for a completed frame, without halting a frame mid-flight.
                try:
                    line = gdb.lines.get(timeout=0.2)
                except queue.Empty:
                    continue
                if line is None:
                    raise RuntimeError('GDB exited unexpectedly')
                if not line.startswith('*stopped'):
                    continue
                gdb.stopped = True
                if 'breakpoint-hit' not in line:
                    raise RuntimeError('MCU stopped unexpectedly: ' + line)
                frame = value(gdb, '(unsigned int)IR_LastFrame')
                count = value(gdb, '(unsigned int)IR_FrameCount')
                data = [(frame >> (8 * i)) & 255 for i in range(4)]
                bridge.emit(f'#{count}  NEC: ' + ' '.join(f'{b:02X}' for b in data) +
                            f'  | key = 0x{data[2]:02X} ({data[2]})')
                if args.exercise == 'Remote':
                    mode = value(gdb, '(unsigned int)LED_Mode')
                    led4 = value(gdb, '(unsigned int)Remote_LED4State')
                    bridge.emit(f'7seg = {data[2] % 100:02d} | effect = {mode} | LED4 = {led4}')
                gdb.resume()
            gdb.halt()
            bridge.emit(f"Diagnostics: edges = {value(gdb, '(unsigned int)IR_EdgeCount')}"
                        f" | frames = {value(gdb, '(unsigned int)IR_FrameCount')}"
                        f" | last interval (100 us) = {value(gdb, '(unsigned int)IR_LastInterval')}")
        except KeyboardInterrupt:
            print('\nĐã dừng.', flush=True)
        finally:
            if gdb:
                try:
                    gdb.halt()
                    gdb.command('-break-delete')
                    gdb.command('-target-detach')
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
            if bridge:
                bridge.close()


if __name__ == '__main__':
    try:
        main()
    except (RuntimeError, OSError) as exc:
        print('Lỗi:', exc, file=sys.stderr)
        sys.exit(1)
