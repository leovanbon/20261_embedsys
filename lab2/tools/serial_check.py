#!/usr/bin/env python3
"""Read-only UART checks, with the received bytes saved as report evidence.
Close PuTTY before running: only one reader may use the serial device.
"""
import argparse
import datetime
import os
from pathlib import Path
import select
import termios
import time

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--port', default='/dev/ttyACM0')
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    logs = root / 'logs'
    logs.mkdir(exist_ok=True)
    filename = logs / ('serial_check_' + datetime.datetime.now().strftime('%Y%m%d_%H%M%S') + '.txt')
    fd = os.open(args.port, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
    old = termios.tcgetattr(fd)
    def receive(seconds):
        result = bytearray()
        end = time.monotonic() + seconds
        while time.monotonic() < end:
            if select.select([fd], [], [], .05)[0]:
                try:
                    data = os.read(fd, 4096)
                    if not data:
                        raise RuntimeError('Serial device disconnected')
                    result.extend(data)
                except BlockingIOError:
                    pass
        return bytes(result)
    try:
        settings = termios.tcgetattr(fd)
        settings[0] = settings[1] = settings[3] = 0
        settings[2] = termios.CS8 | termios.CREAD | termios.CLOCAL
        settings[4] = settings[5] = termios.B115200
        settings[6][termios.VMIN] = settings[6][termios.VTIME] = 0
        termios.tcsetattr(fd, termios.TCSANOW, settings)
        with filename.open('w', encoding='utf-8') as log:
            def emit(text):
                print(text, end='', flush=True)
                log.write(text)
                log.flush()
            emit(f'UART evidence | {datetime.datetime.now().isoformat()} | {args.port} | 115200 8N1\n')
            os.write(fd, b'\r')
            emit(receive(.5).decode('utf-8', errors='backslashreplace'))
            for command in ['HELP', 'TIME', 'STATUS', 'LIST', 'LOG']:
                emit(f'\nPC> {command}\n')
                for byte in (command + '\r').encode('ascii'):
                    os.write(fd, bytes([byte]))
                    time.sleep(.02)
                reply = receive(1)
                emit(reply.decode('utf-8', errors='backslashreplace'))
                if not reply:
                    raise RuntimeError(f'No UART response to {command}; see {filename}')
        print(f'\nSaved: {filename}')
    finally:
        termios.tcsetattr(fd, termios.TCSANOW, old)
        os.close(fd)

if __name__ == '__main__':
    main()
