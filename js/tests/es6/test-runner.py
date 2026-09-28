#!/usr/bin/env python3
"""Regression tests for conformance accounting, independent of the JS engine."""
import importlib.util
from pathlib import Path
import tempfile
import unittest
from unittest import mock

spec = importlib.util.spec_from_file_location('runner', Path(__file__).with_name('run-test262.py'))
runner = importlib.util.module_from_spec(spec)
spec.loader.exec_module(runner)


class RunnerTests(unittest.TestCase):
    def case(self, negative=None):
        return dict(test='fixture.js', mode='strict', source='"use strict";',
                    record={} if negative is None else {'negative': negative})

    def result(self, output, negative=None, code=0):
        return runner.classify(self.case(negative), output, code, 'RESULT ')[0]

    def test_negative_cannot_pass_harness_failure(self):
        self.assertEqual(self.result('RESULT HARNESS SyntaxError', 'SyntaxError'), 'harness-error')

    def test_negative_exception_matching(self):
        self.assertEqual(self.result('RESULT THROW SyntaxError: test', 'SyntaxError'), 'pass')
        self.assertEqual(self.result('RESULT THROW TypeError: test', 'SyntaxError'), 'fail')
        self.assertEqual(self.result('RESULT PASS ', 'SyntaxError'), 'fail')

    def test_process_failures_cannot_pass(self):
        self.assertEqual(self.result('RESULT PASS ', code=-11), 'crash')
        self.assertEqual(self.result('RESULT PASS ', code=1), 'harness-error')
        self.assertEqual(self.result(''), 'harness-error')
        self.assertEqual(self.result('RESULT PASS \nRESULT PASS '), 'harness-error')

    def test_unsupported_is_never_pass(self):
        self.assertEqual(self.result('RESULT UNSUPPORTED async', '.*'), 'unsupported')
        self.assertEqual(self.result('RESULT UNSUPPORTED module host APIs are unavailable', '.*'), 'unsupported')

    def test_modules_reach_the_engine(self):
        case = self.case('SyntaxError')
        case['mode'] = 'module'
        with tempfile.TemporaryDirectory() as temporary:
            suite = Path(temporary)
            harness = suite / 'harness'
            harness.mkdir()
            for name in ('sta.js', 'cth.js', 'assert.js'):
                (harness / name).write_text('// harness\n')
            def run(command, **kwargs):
                driver = Path(command[-1]).read_text()
                self.assertIn('evaluateModuleUnit(compileModuleUnit(', driver)
                self.assertIn('if (true)', driver)
                marker = 'ZOOL262-' + Path(command[-1]).parent.name + ' '
                return mock.Mock(returncode=0, stdout=(marker+'THROW SyntaxError: expected\n').encode())
            with mock.patch.object(runner.subprocess, 'run', side_effect=run) as process:
                result = runner.run_case(case, suite, Path('/test/xpcshell'), {}, 1)
            process.assert_called_once()
            self.assertEqual(result['status'], 'pass')

    def test_mode_policy(self):
        with tempfile.TemporaryDirectory() as temporary:
            suite = Path(temporary)
            directory = suite / 'test/language'
            directory.mkdir(parents=True)
            path = directory / 'fixture.js'
            for flags, expected in [('', ['non-strict', 'strict']),
                                    ('noStrict', ['non-strict']),
                                    ('onlyStrict', ['strict']),
                                    ('module', ['module']), ('raw', ['raw'])]:
                path.write_text('/*---\nflags: [' + flags + ']\n---*/\n')
                self.assertEqual([r['mode'] for r in runner.records(suite)], expected)
            for flags in ['noStrict, onlyStrict', 'unknown']:
                path.write_text('/*---\nflags: [' + flags + ']\n---*/\n')
                with self.assertRaises(ValueError):
                    list(runner.records(suite))


if __name__ == '__main__':
    unittest.main()
