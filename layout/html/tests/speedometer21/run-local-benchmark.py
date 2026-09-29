#!/usr/bin/env python3
"""Run the unchanged Suite benchmark with an owned HTTP server and private HOME."""
import argparse
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import threading


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runtime', type=Path, required=True)
    parser.add_argument('--serve-root', type=Path, required=True,
                        help='Unmodified upstream root containing Speedometer2.1 and resources')
    parser.add_argument('--report', type=Path, required=True)
    parser.add_argument('--timeout', type=int, default=10800)
    args = parser.parse_args()
    runtime, root = args.runtime.resolve(), args.serve_root.resolve()
    if not (root / 'Speedometer2.1/index.html').is_file():
        parser.error('--serve-root must contain Speedometer2.1/index.html')
    args.report.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='zool-benchmark-home-') as temporary:
        home = Path(temporary)
        env = dict(os.environ, HOME=str(home), LD_LIBRARY_PATH=str(runtime),
                   MOZILLA_FIVE_HOME=str(runtime), MOZ_NO_REMOTE='1')
        source = "var p=Components.classes['@mozilla.org/profile/manager;1'].getService(Components.interfaces.nsIProfile);p.createNewProfile('benchmark-baseline'," + json.dumps(str(home)) + ",null,false);p.currentProfile='benchmark-baseline';"
        script = home / 'profile.js'
        script.write_text(source + '\n')
        with args.report.with_suffix('.profile.log').open('w') as output:
            subprocess.run([str(runtime / 'xpcshell'), '-f', str(script)], env=env,
                           stdout=output, stderr=subprocess.STDOUT, timeout=30, check=True)
        with args.report.with_suffix('.http.log').open('w') as http_log:
            class Handler(SimpleHTTPRequestHandler):
                def log_message(self, format, *values):
                    http_log.write((format % values) + '\n')
                    http_log.flush()

            handler = partial(Handler, directory=str(root))
            with ThreadingHTTPServer(('127.0.0.1', 0), handler) as server:
                worker = threading.Thread(target=server.serve_forever, daemon=True)
                worker.start()
                try:
                    url = 'http://127.0.0.1:%d/Speedometer2.1/' % server.server_port
                    command = [sys.executable, str(Path(__file__).with_name('run-suite.py')),
                               '--runtime', str(runtime), '--url', url,
                               '--timeout', str(args.timeout), '--report', str(args.report.resolve())]
                    result = subprocess.run(command, env=env, timeout=args.timeout + 90)
                finally:
                    server.shutdown()
                    worker.join()
    return result.returncode


if __name__ == '__main__':
    raise SystemExit(main())
