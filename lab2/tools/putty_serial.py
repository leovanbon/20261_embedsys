#!/usr/bin/env python3
"""Launch PuTTY Serial using the kit's ST-LINK virtual COM port."""
import argparse
import datetime
import os
from pathlib import Path
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[1]

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--port', help='Serial device; default: the sole ST-LINK VCP')
    args = parser.parse_args()
    ports = sorted(Path('/dev/serial/by-id').glob('*STM32_STLink*'))
    if args.port:
        port = Path(args.port)
    elif len(ports) == 1:
        port = ports[0]
    else:
        parser.error('Use --port /dev/ttyACM0 (no unique ST-LINK VCP found).')
    if not port.exists():
        parser.error(f'Serial device not found: {port}')
    putty = shutil.which('putty')
    if not putty:
        parser.error('PuTTY is not installed.')
    config = ROOT / 'build' / 'putty'
    sessions = config / 'sessions'
    sessions.mkdir(parents=True, exist_ok=True)
    logs = ROOT / 'logs'
    logs.mkdir(exist_ok=True)
    stamp = datetime.datetime.now().strftime('%Y%m%d_%H%M%S')
    log = logs / f'putty_{stamp}.log'
    # Keep this session within the project, independent of ~/.putty.
    (sessions / 'Lab2-Serial').write_text(
        f'Protocol=serial\nSerialLine={port}\nSerialSpeed=115200\n'
        'SerialDataBits=8\nSerialStopHalfbits=2\nSerialParity=0\n'
        # PuTTY TriState: FORCE_ON=0, FORCE_OFF=1, AUTO=2.
        'SerialFlowControl=0\nLocalEcho=0\nLocalEdit=0\n'
        'TermWidth=108\nTermHeight=34\nScrollbackLines=10000\n'
        'CloseOnExit=0\nWarnOnClose=0\n', encoding='utf-8')
    env = os.environ.copy()
    env['PUTTYDIR'] = str(config)
    # XWayland allows capturing this specific terminal window for the report.
    if env.get('DISPLAY'):
        env['GDK_BACKEND'] = 'x11'
    command = [putty, '-load', 'Lab2-Serial', '-serial', str(port),
               '-sercfg', '115200,8,n,1,N', '-sessionlog', str(log),
               '-logappend', '-title', 'Lab 2 - STM32 UART - 115200 8N1']
    print(f'PuTTY Serial: {port.resolve()} / 115200 8N1 / no flow control', flush=True)
    print(f'Log: {log}', flush=True)
    subprocess.Popen(command, env=env, start_new_session=True)

if __name__ == '__main__':
    main()
