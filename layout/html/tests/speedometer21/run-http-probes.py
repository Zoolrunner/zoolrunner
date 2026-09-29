#!/usr/bin/env python3
"""Run Suite's HTTP storage and XMLHttpRequest regression fixtures."""
import argparse
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
import json
from pathlib import Path
import socket
import struct
import subprocess
import sys
import threading


class Handler(SimpleHTTPRequestHandler):
    def do_GET(self):
        if self.path == '/connection-error':
            # A reproducible same-origin network error, without an external host
            # or assumptions about a port being unused.
            self.close_connection = True
            self.connection.setsockopt(socket.SOL_SOCKET, socket.SO_LINGER,
                                       struct.pack('ii', 1, 0))
            self.connection.close()
            return
        super().do_GET()

    def log_message(self, format, *args):
        pass


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runtime', type=Path, required=True)
    parser.add_argument('--reports', type=Path, required=True)
    parser.add_argument('--application', choices=['suite', 'browser', 'calendar', 'xulrunner'], default='suite')
    args = parser.parse_args()
    here = Path(__file__).resolve().parent
    root = here.parents[3]
    args.reports.mkdir(parents=True, exist_ok=True)
    handler = partial(Handler, directory=str(here.parent / 'style'))
    results = []
    with ThreadingHTTPServer(('127.0.0.1', 0), handler) as server:
        worker = threading.Thread(target=server.serve_forever, daemon=True)
        worker.start()
        try:
            for name in ('local-storage', 'legacy-storage-event',
                         'xhr-event-lifetime', 'xhr-event-dispatch', 'xhr-event-error',
                         'xhr-listener-registration'):
                report = args.reports / (name + '.json')
                report.unlink(missing_ok=True)
                url = 'http://127.0.0.1:%d/%s.html' % (server.server_port, name)
                runner = 'run-suite.py' if args.application == 'suite' else 'run-toolkit.py'
                command = ['sh', str(root / 'build/linux/with-display.sh'),
                           sys.executable, str(here / runner),
                           '--runtime', str(args.runtime.resolve()), '--mode', 'probe',
                           '--url', url, '--timeout', '60', '--report', str(report.resolve())]
                if args.application != 'suite':
                    command += ['--application', args.application]
                with report.with_suffix('.stdout').open('w') as output:
                    status = subprocess.run(command, stdout=output, stderr=subprocess.STDOUT)
                data = json.loads(report.read_text()) if report.exists() else {}
                detail = data.get('result', {}).get('detail', {})
                row = {'fixture': name, 'pass': status.returncode == 0 and data.get('pass') is True,
                       'checks': len(detail.get('results', [])), 'failures': detail.get('failures')}
                results.append(row)
                print(json.dumps(row), flush=True)
        finally:
            server.shutdown()
            worker.join()
    (args.reports / 'summary.json').write_text(json.dumps(results, indent=2) + '\n')
    return 0 if all(row['pass'] for row in results) else 1


if __name__ == '__main__':
    raise SystemExit(main())
