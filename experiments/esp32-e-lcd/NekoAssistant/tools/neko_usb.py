#!/usr/bin/env python3
"""USB diagnostics and configuration. Credentials stay in memory, never in output."""
import argparse
import json
import re
import subprocess
import sys
import time
from pathlib import Path
import serial


def send(port, command, timeout=2):
    port.reset_input_buffer()
    port.write((command + '\n').encode())
    end = time.monotonic() + timeout
    lines = []
    while time.monotonic() < end:
        line = port.readline().decode('utf-8', 'replace').strip()
        if line:
            lines.append(line)
            if line.startswith('{') or line == '[NEKO] OK' or line.startswith('NEKO_WEB_KEY '):
                break
    return lines


def capture(port, path):
    port.reset_input_buffer()
    port.write(b'screen\n')
    data = bytearray()
    width = height = 0
    end = time.monotonic() + 15
    while time.monotonic() < end:
        line = port.readline().decode('ascii', 'replace').strip()
        if line.startswith('NEKO_FRAME '):
            _, width, height, _ = line.split()
            width, height = int(width), int(height)
        elif line == 'NEKO_END':
            break
        elif width and re.fullmatch('[0-9a-f]+', line) and len(line) % 2 == 0:
            data.extend(bytes.fromhex(line))
    if len(data) != width * height // 8 or not width:
        raise RuntimeError('Incomplete display capture')
    from PIL import Image
    # U8g2 R1: physical x = 299-logical_y, physical y = logical_x.
    image = Image.new('1', (400, 300), 1)
    for y in range(300):
        for x in range(400):
            px, py = 299 - y, x
            white = bool(data[(py // 8) * width + px] & (1 << (py % 8)))
            image.putpixel((x, y), 255 if white else 0)
    image.save(path)
    print('Display saved:', path)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--port', default='/dev/ttyACM0')
    parser.add_argument('--command', default='status')
    parser.add_argument('--capture', type=Path)
    parser.add_argument('--provision-from-nm', metavar='CONNECTION')
    args = parser.parse_args()
    with serial.Serial(args.port, 115200, timeout=.15, write_timeout=2, exclusive=True) as port:
        if args.provision_from_nm:
            def nm(field):
                result = subprocess.run(['nmcli', '--show-secrets', '-g', field, 'connection', 'show', args.provision_from_nm], capture_output=True, text=True, check=True)
                return result.stdout.rstrip('\n')
            ssid = nm('802-11-wireless.ssid')
            password = nm('802-11-wireless-security.psk')
            if not password or password == '--':
                raise RuntimeError('Wi-Fi password unavailable in NetworkManager; use local setup')
            if any(c in ssid + password for c in '\t\r\n'):
                raise RuntimeError('Unsupported control character in Wi-Fi configuration')
            # This command is deliberately never printed or written to disk.
            port.write(('join\t' + ssid + '\t' + password + '\n').encode())
            del password
            deadline = time.monotonic() + 25
            connected = False
            while time.monotonic() < deadline:
                line = port.readline().decode('utf-8', 'replace').strip()
                if line.startswith('[WIFI]'):
                    print(line)
                    if 'connected ip=' in line:
                        connected = True
                        break
            if not connected:
                print('Connection pending; check status after a few seconds.')
        elif args.capture:
            capture(port, args.capture)
        else:
            for line in send(port, args.command):
                # web-key is a machine-only command, never printed by this CLI.
                if not line.startswith('NEKO_WEB_KEY '):
                    print(line)


if __name__ == '__main__':
    try:
        main()
    except Exception as error:
        print(f'Neko USB: {error}', file=sys.stderr)
        sys.exit(1)
