#!/usr/bin/env python3

#
#   Copyright (C) 2015, 2016, 2017, 2018 Marek Marecki
#
#   This file is part of Viua VM.
#
#   Viua VM is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.
#
#   Viua VM is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with Viua VM.  If not, see <http://www.gnu.org/licenses/>.
#

"""This is initial unit tests suite for Viua virtual machine.
It uses sample asm code (samples can also be compiled and run directly).

Each unit passes if:

    * the sample compiles,
    * compiled code runs,
    * compiled code returns correct output,
    * compiled code does not leak memory,

Returning correct may mean raising an exception in some cases.
Acceptable memory leak is at most 0 bytes.
Memory leak tests may be disabled for some runs as they are slow.
"""

import datetime
import functools
import hashlib
import json
import os
import subprocess
import sys
import re
import unittest


COMPILED_SAMPLES_PATH = './tests/compiled'

VIUA_KERNEL_PATH = './build/bin/vm/kernel'


class ViuaError(Exception):
    """Generic Viua exception.
    """
    pass

class ViuaAssemblerError(ViuaError):
    """Base class for exceptions related to Viua assembler.
    """
    pass

class ViuaDisassemblerError(ViuaError):
    """Base class for exceptions related to Viua assembler.
    """
    pass

class ViuaCPUError(ViuaError):
    """Base class for exceptions related to Viua CPU.
    """
    pass

class ValgrindException(Exception):
    pass


def getCPUArchitecture():
    p = subprocess.Popen(('uname', '-m'), stdout=subprocess.PIPE)
    output, error = p.communicate()
    output = output.decode('utf-8').strip()
    return output

def assemble(asm, out=None, links=(), opts=(), okcodes=(0,)):
    """Assemble path given as `asm` and put binary in `out`.
    Raises exception if compilation is not successful.
    """
    asmargs = ('./build/bin/vm/asm',) + opts
    if out is not None: asmargs += ('--out', out,)
    asmargs += (asm,)
    asmargs += links
    p = subprocess.Popen(asmargs, stdout=subprocess.PIPE)
    output, error = p.communicate()
    output = output.decode('utf-8')
    exit_code = p.wait()
    if exit_code not in okcodes:
        with open('/tmp/viua_test_suite_last_assembler_failure', 'w') as ofstream:
            output_option = ('--out', '-o',)
            def undesirable(x):
                i, each = x
                return not ((each in output_option) or (i and asmargs[i-1] in output_option))
            parts = tuple(map(lambda each: each[1], filter(undesirable, enumerate(asmargs))))
            s = (asmargs[0] + ' ' + ' '.join(('-o', '/dev/null',) + parts[1:]))
            ofstream.write('#!/usr/bin/env bash\n\n')
            ofstream.write('echo "{}"\n'.format(s))
            ofstream.write('{}\n'.format(s))
        raise ViuaAssemblerError('{0}: {1}'.format(asm, output.strip()))
    return (output, error, exit_code)

def disassemble(path, out=None):
    """Disassemle path given as `path` and put resulting assembly code in `out`.
    Raises exception if disassembly is not successful.
    """
    asmargs = ('./build/bin/vm/dis',)
    if out is not None: asmargs += ('--out', out,)
    asmargs += (path,)
    p = subprocess.Popen(asmargs, stdout=subprocess.PIPE)
    output, error = p.communicate()
    output = output.decode('utf-8')
    exit_code = p.wait()
    if exit_code != 0:
        raise ViuaDisassemblerError('{0}: {1}'.format(' '.join(asmargs), output.strip()))
    return (output, error, exit_code)

def run(path, expected_exit_code=0, pipe_error=False):
    """Run given file with Viua CPU and return its output.
    """
    p = subprocess.Popen((VIUA_KERNEL_PATH, path), stdout=subprocess.PIPE, stderr=(subprocess.PIPE if pipe_error else None))
    output, error = p.communicate()
    exit_code = p.wait()
    if exit_code not in (expected_exit_code if type(expected_exit_code) in [list, tuple] else (expected_exit_code,)):
        raise ViuaCPUError('{0} [{1}]: {2}'.format(path, exit_code, output.decode('utf-8').strip()))
    return (exit_code, output.decode('utf-8'), (error if error is not None else b'').decode('utf-8'))

FLAG_TEST_ONLY_ASSEMBLING = bool(int(os.environ.get('VIUA_TEST_ONLY_ASMING', 0)))
MEMORY_LEAK_CHECKS_SKIPPED = 0
MEMORY_LEAK_CHECKS_RUN = 0
MEMORY_LEAK_CHECKS_ENABLE = bool(int(os.environ.get('VIUA_TEST_SUITE_VALGRIND_CHECKS', 1)))
MEMORY_LEAK_CHECKS_SKIP_LIST = []
MEMORY_LEAK_CHECKS_ALLOWED_LEAK_VALUES = (0, 72704)
MEMORY_LEAK_CHECKS_EXTRA_ALLOWED_LEAK_VALUES = ()
MEMORY_LEAK_CHECKS_REPORT_SUPPRESSED = False
MEMORY_LEAK_REPORT_MAX_HEAP_USAGE = {'bytes': 0, 'allocs': 0, 'frees': 0, 'test': ''}
valgrind_regex_heap_summary_in_use_at_exit = re.compile('in use at exit: (\d+(?:,\d+)?) bytes in (\d+) blocks')
valgrind_regex_heap_summary_total_heap_usage = re.compile('total heap usage: (\d+(?:,\d+)*?) allocs, (\d+(?:,\d+)*?) frees, (\d+(?:,\d+)*?) bytes allocated')
valgrind_regex_leak_summary_definitely_lost = re.compile('definitely lost: (\d+(?:,\d+)?) bytes in (\d+) blocks')
valgrind_regex_leak_summary_indirectly_lost = re.compile('indirectly lost: (\d+(?:,\d+)?) bytes in (\d+) blocks')
valgrind_regex_leak_summary_possibly_lost = re.compile('possibly lost: (\d+(?:,\d+)?) bytes in (\d+) blocks')
valgrind_regex_leak_summary_still_reachable = re.compile('still reachable: (\d+(?:,\d+)?) bytes in (\d+) blocks')
valgrind_regex_leak_summary_suppressed = re.compile('suppressed: (\d+(?:,\d+)?) bytes in (\d+) blocks')
def valgrindBytesInBlocks(line, regex):
    matched = regex.search(line)
    if matched is None:
        print('failed to extract Valgrind statistics about leaked bytes and blocks: {}'.format(repr(line)))
    return {'bytes': int((matched.group(1) if matched else '-1').replace(',', '')), 'blocks': int((matched.group(2) if matched else '-1').replace(',', ''))}

def valgrindSummary(text):
    output_lines = text.splitlines()
    valprefix = output_lines[0].split(' ')[0]

    invalid_read_preifx = '{} Invalid read'.format(valprefix)
    if len(list(filter(lambda each: each.startswith(invalid_read_preifx), output_lines))):
        print('\n'.join(output_lines))
        raise ValgrindException()

    interesting_lines = [line[len(valprefix):].strip() for line in output_lines[output_lines.index('{0} HEAP SUMMARY:'.format(valprefix)):]]

    total_heap_usage_matched = valgrind_regex_heap_summary_total_heap_usage.search(interesting_lines[2])
    total_heap_usage = {
        'allocs': int(total_heap_usage_matched.group(1).replace(',', '')),
        'frees': int(total_heap_usage_matched.group(2).replace(',', '')),
        'bytes': int(total_heap_usage_matched.group(3).replace(',', '')),
    }

    summary = {
        'heap': {
            'in_use_at_exit': valgrindBytesInBlocks(interesting_lines[1], valgrind_regex_heap_summary_in_use_at_exit),
            'total_heap_usage': total_heap_usage,
        },
        'leak': {
            'definitely_lost': {'bytes': 0, 'blocks': 0},
            'indirectly_lost': {'bytes': 0, 'blocks': 0},
            'possibly_lost': {'bytes': 0, 'blocks': 0},
            'still_reachable': {'bytes': 0, 'blocks': 0},
            'suppressed': {'bytes': 0, 'blocks': 0},
        }
    }
    if summary['heap']['in_use_at_exit']['bytes'] == 0:
        # early return because if no bytes were leaked then there's no use in analysing Valgrind's output
        # also, in such a case Valgrind does not generate the report so we wouldn't get any results anyway (only a bunch of regex-match errors)
        return summary
    try:
        summary['leak']['definitely_lost'] = valgrindBytesInBlocks(interesting_lines[5], valgrind_regex_leak_summary_definitely_lost)
        summary['leak']['indirectly_lost'] = valgrindBytesInBlocks(interesting_lines[6], valgrind_regex_leak_summary_indirectly_lost)
        summary['leak']['possibly_lost'] = valgrindBytesInBlocks(interesting_lines[7], valgrind_regex_leak_summary_possibly_lost)
        summary['leak']['still_reachable'] = valgrindBytesInBlocks(interesting_lines[8], valgrind_regex_leak_summary_still_reachable)
        summary['leak']['suppressed'] = valgrindBytesInBlocks(interesting_lines[9], valgrind_regex_leak_summary_suppressed)
    except IndexError:
        pass
    return summary

def valgrindCheck(self, path):
    """Run compiled code under Valgrind to check for memory leaks.
    """
    p = subprocess.Popen(('valgrind', '--suppressions=./scripts/valgrind.supp', VIUA_KERNEL_PATH, path), stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    output, error = p.communicate()
    exit_code = p.wait()

    error = error.decode('utf-8')

    try:
        summary = valgrindSummary(error)

        memory_was_leaked = False
        allocation_balance = (summary['heap']['total_heap_usage']['allocs'] - summary['heap']['total_heap_usage']['frees'])
        # 1 must be allowed to deal with GCC 5.1 bug that causes 72,704 bytes to remain reachable
        # sources:
        #   https://stackoverflow.com/questions/30393229/new-libstdc-of-gcc5-1-may-allocate-large-heap-memory
        #   https://gcc.gnu.org/bugzilla/show_bug.cgi?id=64535
        if not (allocation_balance == 0 or allocation_balance == 1): memory_was_leaked = True
        if summary['leak']['definitely_lost']['bytes']: memory_was_leaked = True
        if summary['leak']['indirectly_lost']['bytes']: memory_was_leaked = True
        if summary['leak']['possibly_lost']['bytes']: memory_was_leaked = True
        # same as above, we have to allow 72704 bytes to leak
        # also, we shall sometimes allow (sigh...) additional memory to "leak" if Valgrind freaks out
        if summary['leak']['still_reachable']['bytes'] not in (MEMORY_LEAK_CHECKS_ALLOWED_LEAK_VALUES + MEMORY_LEAK_CHECKS_EXTRA_ALLOWED_LEAK_VALUES): memory_was_leaked = True
        if summary['leak']['suppressed']['bytes'] and MEMORY_LEAK_CHECKS_REPORT_SUPPRESSED: memory_was_leaked = True

        global MEMORY_LEAK_REPORT_MAX_HEAP_USAGE
        if summary['heap']['total_heap_usage']['bytes'] > MEMORY_LEAK_REPORT_MAX_HEAP_USAGE['bytes']:
            MEMORY_LEAK_REPORT_MAX_HEAP_USAGE = summary['heap']['total_heap_usage']
            MEMORY_LEAK_REPORT_MAX_HEAP_USAGE['test'] = self
        elif summary['heap']['total_heap_usage']['bytes'] == MEMORY_LEAK_REPORT_MAX_HEAP_USAGE['bytes']:
            if type(MEMORY_LEAK_REPORT_MAX_HEAP_USAGE['test']) != list:
                tst = MEMORY_LEAK_REPORT_MAX_HEAP_USAGE['test']
                MEMORY_LEAK_REPORT_MAX_HEAP_USAGE['test'] = ([tst] if tst else [])
            MEMORY_LEAK_REPORT_MAX_HEAP_USAGE['test'].append(self)


        in_use_at_exit = summary['heap']['in_use_at_exit']
        if in_use_at_exit == summary['leak']['suppressed']:
            if in_use_at_exit['bytes'] not in MEMORY_LEAK_CHECKS_ALLOWED_LEAK_VALUES:
                print('in use at exit: {} bytes in {} block(s), allowed {}'.format(in_use_at_exit['bytes'], in_use_at_exit['blocks'], MEMORY_LEAK_CHECKS_ALLOWED_LEAK_VALUES))
                print(error)
            return 0

        if memory_was_leaked:
            print(error)

        total_leak_bytes = 0
        total_leak_bytes += summary['leak']['definitely_lost']['bytes']
        total_leak_bytes += summary['leak']['indirectly_lost']['bytes']
        total_leak_bytes += summary['leak']['possibly_lost']['bytes']
        total_leak_bytes += summary['leak']['still_reachable']['bytes']

        # suppressed bytes are not a leak
        #total_leak_bytes += summary['leak']['suppressed']['bytes']

        self.assertIn(total_leak_bytes, (MEMORY_LEAK_CHECKS_ALLOWED_LEAK_VALUES + MEMORY_LEAK_CHECKS_EXTRA_ALLOWED_LEAK_VALUES))
    except AssertionError:
        options = ('--suppressions=./scripts/valgrind.supp', '--leak-check=full',)
        if os.environ.get('PROJECT_NAME', ''):  # running on TravisCI
            options += ('--show-reachable=yes',)
        arguments = ('valgrind',) + options + (VIUA_KERNEL_PATH, path,)
        p = subprocess.Popen(arguments, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        output, error = p.communicate()
        exit_code = p.wait()
        error = error.decode('utf-8')
        print('error: memory leak detected: Valgrind rerun with more details, output displayed below')
        print(error)
        # send assertion errors up
        raise
    except Exception as e:
        print('error: failed to analyze Valgrind summary due to an exception: {}: {}'.format(type(e), e))
        print('error: here is what Valgring returned:')
        print(error)

    return 0


def skipValgrind(self):
    MEMORY_LEAK_CHECKS_SKIP_LIST.append(self)

def runMemoryLeakCheck(self, compiled_path, check_memory_leaks):
    if not MEMORY_LEAK_CHECKS_ENABLE: return
    global MEMORY_LEAK_CHECKS_RUN, MEMORY_LEAK_CHECKS_SKIPPED

    if self in MEMORY_LEAK_CHECKS_SKIP_LIST or not check_memory_leaks:
        print('skipped memory leak check for: {0} ({1}.{2})'.format(compiled_path, self.__class__.__name__, str(self).split()[0]))
        MEMORY_LEAK_CHECKS_SKIPPED += 1
    else:
        MEMORY_LEAK_CHECKS_RUN += 1
        valgrindCheck(self, compiled_path)

def runTestBackend(self, name, expected_output=None, expected_exit_code = 0, output_processing_function = None, expected_error=None, error_processing_function=None, check_memory_leaks = True, custom_assert=None, assembly_opts=None, valgrind_enable=True, test_disasm=True):
    if assembly_opts is None:
        assembly_opts = ()
    if expected_output is None and expected_error is None and custom_assert is None:
        raise TypeError('`expected_output`, `expected_error`, and `custom_assert` cannot all be None')

    asm_flags = (assembly_opts + getattr(self, 'ASM_FLAGS', ()))
    assembly_path = os.path.join(self.PATH, name)
    compiled_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.bin'.format(self.PATH[2:].replace('/', '_'), name))
    if assembly_opts is None:
        assemble(assembly_path, compiled_path)
    else:
        assemble(assembly_path, compiled_path, opts=asm_flags)
    if not FLAG_TEST_ONLY_ASSEMBLING:
        excode, output, error = run(compiled_path, expected_exit_code, pipe_error = (expected_error is not None))
        got_output = (output.strip() if output_processing_function is None else output_processing_function(output))
        got_error = (error.strip() if error_processing_function is None else error_processing_function(error))
        try:
            if custom_assert is not None:
                custom_assert(self, excode, got_output)
            else:
                if expected_output is not None: self.assertEqual(expected_output, got_output)
                if expected_error is not None: self.assertEqual(expected_error, got_error)
                self.assertEqual(expected_exit_code, excode)

            if valgrind_enable:
                runMemoryLeakCheck(self, compiled_path, check_memory_leaks)
        except Exception:
            print('test failed: check file {}'.format(assembly_path))
            raise

    if not test_disasm:
        return

    disasm_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.dis.asm'.format(self.PATH[2:].replace('/', '_'), name))
    compiled_disasm_path = '{0}.bin'.format(disasm_path)
    disassemble(compiled_path, disasm_path)
    if assembly_opts is None:
        assemble(disasm_path, compiled_disasm_path)
    else:
        assemble(disasm_path, compiled_disasm_path, opts=asm_flags)

    source_assembly_output = b''
    disasm_assembly_output = b''
    with open(compiled_path, 'rb') as ifstream:
        source_assembly_output = ifstream.read()
    with open(compiled_disasm_path, 'rb') as ifstream:
        disasm_assembly_output = ifstream.read()
    self.assertEqual(hashlib.sha512(source_assembly_output).hexdigest(), hashlib.sha512(disasm_assembly_output).hexdigest())

    if not FLAG_TEST_ONLY_ASSEMBLING:
        dis_excode, dis_output, dis_error = run(compiled_disasm_path, expected_exit_code, pipe_error = (expected_error is not None))
        if custom_assert is not None:
            custom_assert(self, dis_excode, (dis_output.strip() if output_processing_function is None else output_processing_function(dis_output)))
        else:
            if expected_output is not None: self.assertEqual(got_output, (dis_output.strip() if output_processing_function is None else output_processing_function(dis_output)))
            if expected_error is not None: self.assertEqual(got_error, (dis_error.strip() if error_processing_function is None else error_processing_function(dis_error)))
            self.assertEqual(excode, dis_excode)

measured_run_times = []
def runTest(self, name, expected_output=None, expected_exit_code = 0, output_processing_function = None, expected_error=None, error_processing_function=None, check_memory_leaks = True, custom_assert=None, assembly_opts=None, valgrind_enable=True, test_disasm=True):
    begin = datetime.datetime.now()
    try:
        runTestBackend(
            self = self,
            name = name,
            expected_output = expected_output,
            expected_exit_code = expected_exit_code,
            expected_error = expected_error,
            output_processing_function = output_processing_function,
            error_processing_function = error_processing_function,
            check_memory_leaks = check_memory_leaks,
            custom_assert = custom_assert,
            assembly_opts = assembly_opts,
            valgrind_enable = valgrind_enable,
            test_disasm = test_disasm,
        )
    finally:
        end = datetime.datetime.now()
        delta = (end - begin)
        print('timed = {}'.format(delta))
        measured_run_times.append(delta)

def runTestCustomAsserts(self, name, assertions_callback, check_memory_leaks = True):
    assembly_path = os.path.join(self.PATH, name)
    compiled_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.bin'.format(self.PATH[2:].replace('/', '_'), name))
    assemble(assembly_path, compiled_path)
    if not FLAG_TEST_ONLY_ASSEMBLING:
        excode, output, error = run(compiled_path)
        assertions_callback(self, excode, output)
        runMemoryLeakCheck(self, compiled_path, check_memory_leaks)

def runTestSplitlines(self, name, expected_output, expected_exit_code = 0, expected_error = None, error_processing_function = None, assembly_opts=None, test_disasm=True):
    runTest(self,
        name = name,
        expected_output = expected_output,
        expected_exit_code = expected_exit_code,
        output_processing_function = lambda o: o.strip().splitlines(),
        assembly_opts = assembly_opts,
        test_disasm = test_disasm,
        expected_error = expected_error,
        error_processing_function = error_processing_function
    )

def runTestReturnsUnorderedLines(self, name, expected_output, expected_exit_code = 0):
    runTest(self, name, sorted(expected_output), expected_exit_code, output_processing_function = lambda o: sorted(o.strip().splitlines()))

def runTestReturnsIntegers(self, name, expected_output, expected_exit_code = 0, check_memory_leaks=True, assembly_opts=None):
    runTest(self, name, expected_output, expected_exit_code, output_processing_function = lambda o: [int(i) for i in o.strip().splitlines()], check_memory_leaks=check_memory_leaks, assembly_opts=assembly_opts)

def extractExceptionsThrown(output):
    uncaught_object_prefix = 'uncaught object: '
    return list(map(lambda _: (_[0].strip(), _[1].strip(),),
                    filter(lambda _: len(_) == 2,
                           map(lambda _: _.split(' = ', 1),
                               map(lambda _: _[len(uncaught_object_prefix):],
                                   filter(lambda _: _.startswith(uncaught_object_prefix),
                                       output.strip().splitlines()))))))

def extractFirstException(output):
    return extractExceptionsThrown(output)[0]

def runTestThrowsException(self, name, expected_output, assembly_opts=None):
    runTest(self, name, expected_error=expected_output, expected_exit_code=1, error_processing_function=extractFirstException, valgrind_enable=False, assembly_opts=assembly_opts)

def runTestThrowsExceptionJSON(self, name, expected_output, output_processing_function, assembly_opts=None):
    was = os.environ.get('VIUA_STACKTRACE_SERIALISATION', 'default')
    was_to = os.environ.get('VIUA_STACKTRACE_PRINT_TO', 'stderr')
    os.environ['VIUA_STACKTRACE_SERIALISATION'] = 'json'
    os.environ['VIUA_STACKTRACE_PRINT_TO'] = 'stdout'
    runTest(self, name, expected_output, expected_exit_code=1, output_processing_function=output_processing_function, valgrind_enable=False, assembly_opts=assembly_opts)
    os.environ['VIUA_STACKTRACE_SERIALISATION'] = was
    os.environ['VIUA_STACKTRACE_PRINT_TO'] = was_to

def runTestReportsException(self, name, expected_output, assembly_opts=None):
    runTest(self, name, expected_error=expected_output, expected_exit_code=0, error_processing_function=extractFirstException, assembly_opts=assembly_opts)

def runTestFailsToAssemble(self, name, expected_output, asm_opts=()):
    assembly_path = os.path.join(self.PATH, name)
    compiled_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.bin'.format(self.PATH[2:].replace('/', '_'), name))
    asm_flags = (asm_opts + getattr(self, 'ASM_FLAGS', ()))
    output, error, exit_code = assemble(assembly_path, compiled_path, okcodes=(0, 1), opts=asm_flags)
    self.assertEqual(1, exit_code)
    self.assertEqual(output.strip().splitlines()[0], expected_output)

def runTestFailsToAssembleDetailed(self, name, expected_output, asm_opts=()):
    assembly_path = os.path.join(self.PATH, name)
    compiled_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.bin'.format(self.PATH[2:].replace('/', '_'), name))
    asm_flags = (asm_opts + getattr(self, 'ASM_FLAGS', ()))
    output, error, exit_code = assemble(assembly_path, compiled_path, okcodes=(0, 1), opts=asm_flags)
    self.assertEqual(1, exit_code)
    lines = map(lambda l: (l[len(assembly_path)+1:] if l.startswith(assembly_path) else l),
            filter(lambda l: (l.startswith(assembly_path) or l.lstrip().startswith('^ ')),
                output.strip().splitlines()))
    self.assertEqual(list(lines), expected_output)


def sameLines(self, excode, output, no_of_lines):
    lines = output.splitlines()
    self.assertTrue(len(lines) == no_of_lines)
    for i in range(1, no_of_lines):
        self.assertEqual(lines[0], lines[i])

def partiallyAppliedSameLines(n):
    return functools.partial(sameLines, no_of_lines=n)


class IntegerInstructionsTests(unittest.TestCase):
    """Tests for integer instructions.
    """
    PATH = './sample/asm/int'

    def testIstoreDefault(self):
        runTest(self, 'istore_default.asm', '0', 0)

    def testIADD(self):
        runTest(self, 'add.asm', '1', 0)

    def testIADDWithRReferences(self):
        # FIXME static analyser does not see through register references
        # maybe change rrefs to first-class objects, kind of like pointers?
        runTest(self, 'add_with_rreferences.asm', '0', 0, assembly_opts=('--no-sa',))

    def testISUB(self):
        runTest(self, 'sub.asm', '1', 0)

    def testIMUL(self):
        runTest(self, 'mul.asm', '1', 0)

    def testIDIV(self):
        runTest(self, 'div.asm', '1', 0)

    def testIDEC(self):
        runTest(self, 'dec.asm', '1', 0)

    def testIINC(self):
        runTest(self, 'inc.asm', '1', 0)

    def testILT(self):
        runTest(self, 'lt.asm', 'true', 0)

    def testILTE(self):
        runTest(self, 'lte.asm', 'true', 0)

    def testIGT(self):
        runTest(self, 'gt.asm', 'true', 0)

    def testIGTE(self):
        runTest(self, 'gte.asm', 'true', 0)

    def testIEQ(self):
        runTest(self, 'eq.asm', 'true', 0)

    def testCalculatingModulo(self):
        runTest(self, 'modulo.asm', '65', 0)

    def testIntegersInCondition(self):
        runTest(self, 'in_condition.asm', 'true', 0)

    @unittest.skip('')
    def testBooleanAsInteger(self):
        runTest(self, 'boolean_as_int.asm', '70', 0)


class BooleanInstructionsTests(unittest.TestCase):
    """Tests for boolean instructions.
    """
    PATH = './sample/asm/boolean'

    def testNOT(self):
        runTestSplitlines(self, 'not.asm', ['false', 'true'])

    def testAND(self):
        runTestSplitlines(self, 'and.asm', ['false', 'true', 'false'])

    def testOR(self):
        runTestSplitlines(self, 'or.asm', ['false', 'true', 'true'])


class FloatInstructionsTests(unittest.TestCase):
    """Tests for float instructions.
    """
    PATH = './sample/asm/float'

    def testFstoreDefault(self):
        runTest(self, 'fstore_default.asm', '0.000000', 0)

    def testFADD(self):
        runTest(self, 'add.asm', '0.500000', 0)

    def testFSUB(self):
        runTest(self, 'sub.asm', '1.015000', 0)

    def testFMUL(self):
        runTest(self, 'mul.asm', '8.004000', 0)

    def testFDIV(self):
        runTest(self, 'div.asm', '1.570000', 0)

    def testFLT(self):
        runTest(self, 'lt.asm', 'true', 0)

    def testFLTE(self):
        runTest(self, 'lte.asm', 'true', 0)

    def testFGT(self):
        runTest(self, 'gt.asm', 'true', 0)

    def testFGTE(self):
        runTest(self, 'gte.asm', 'true', 0)

    def testFEQ(self):
        runTest(self, 'eq.asm', 'true', 0)

    def testFloatsInCondition(self):
        runTest(self, 'in_condition.asm', 'true', 0)


class StringInstructionsTests(unittest.TestCase):
    """Tests for string instructions.
    """
    PATH = './sample/asm/string'

    def testStrstoreDefault(self):
        runTest(self, 'strstore_default.asm', '', 0)

    def testHelloWorld(self):
        runTest(self, 'hello_world.asm', 'Hello World!', 0)


class StringInstructionsEscapeSequencesTests(unittest.TestCase):
    """Tests for escape sequence decoding.
    """
    PATH = './sample/asm/string/escape_sequences'

    def testNewline(self):
        runTest(self, 'newline.asm', 'Hello\nWorld!', 0)

    def testTab(self):
        runTest(self, 'tab.asm', 'Hello\tWorld!', 0)

    def testVerticalTab(self):
        runTest(self, 'vertical_tab.asm', 'Hello\vWorld!', 0)

    def testBell(self):
        runTest(self, 'bell.asm', 'Hello \aWorld!', 0)

    def testBackspace(self):
        runTest(self, 'backspace.asm', 'Hello  \bWorld!', 0)

    def testFormFeed(self):
        runTest(self, 'form_feed.asm', 'Hello \fWorld!', 0)

    def testCarriageReturn(self):
        runTest(self, 'carriage_return.asm', 'Hello \rWorld!', 0)


class TextInstructionsTests(unittest.TestCase):
    """Tests for string instructions.
    """
    PATH = './sample/asm/text'

    def testHelloWorld(self):
        runTest(self, 'hello_world.asm', 'Hello World!', 0)

    def testTextEquals(self):
        runTest(self, 'texteq.asm', 'true', 0)

    def testTextEqualsNot(self):
        runTest(self, 'texteq_not.asm', 'false', 0)

    def testTextat(self):
        runTest(self, 'textat.asm', 'W', 0)

    def testTextsub(self):
        runTestSplitlines(self, 'textsub.asm', ['Hello World!', 'Hello'], 0)

    def testTextlength(self):
        runTestReturnsIntegers(self, 'textlength.asm', [12, 14], 0)

    def testTextCommonPrefix(self):
        runTestReturnsIntegers(self, 'textcommonprefix.asm', [6], 0)

    def testTextCommonSuffix(self):
        runTestReturnsIntegers(self, 'textcommonsuffix.asm', [7], 0)

    def testTextconcat(self):
        runTest(self, 'textconcat.asm', 'Hello World!', 0)


class TextInstructionsEscapeSequencesTests(unittest.TestCase):
    """Tests for escape sequence decoding.
    """
    PATH = './sample/asm/text/escape_sequences'

    def testNewline(self):
        runTest(self, 'newline.asm', 'Hello\nWorld!', 0)

    def testTab(self):
        runTest(self, 'tab.asm', 'Hello\tWorld!', 0)

    def testVerticalTab(self):
        runTest(self, 'vertical_tab.asm', 'Hello\vWorld!', 0)

    def testBell(self):
        runTest(self, 'bell.asm', 'Hello \aWorld!', 0)

    def testBackspace(self):
        runTest(self, 'backspace.asm', 'Hello  \bWorld!', 0)

    def testFormFeed(self):
        runTest(self, 'form_feed.asm', 'Hello \fWorld!', 0)

    def testCarriageReturn(self):
        runTest(self, 'carriage_return.asm', 'Hello \rWorld!', 0)


class BitsManipulationTests(unittest.TestCase):
    PATH = './sample/asm/bits/manipulation'

    def testHelloWorld(self):
        runTest(self, 'hello_world.asm', '00000000')

    def testBitsInBooleanContext(self):
        runTest(self, 'bits_in_boolean_context.asm', 'OH YEAH')

    def testBitnot(self):
        runTestSplitlines(self, 'bitnot.asm', ['00000000', '11111111'])

    def testBitAnd(self):
        runTestSplitlines(self, 'bitand.asm', ['11110101', '10111001', '10110001',])

    def testBitOr(self):
        runTestSplitlines(self, 'bitor.asm', ['00001100', '00001010', '00001110',])

    def testBitXor(self):
        runTestSplitlines(self, 'bitxor.asm', ['00001100', '00001010', '00000110',])

    def testBitAndWithDifferentWidths(self):
        runTestSplitlines(self, 'bitand_with_different_widths.asm', ['1101000100100111', '00001101', '0000000000000101', '00000101',])

    def testBitOrWithDifferentWidths(self):
        runTestSplitlines(self, 'bitor_with_different_widths.asm', ['1101000100100111', '00001101', '0000000000101111', '00101111',])

    def testBitXorWithDifferentWidths(self):
        runTestSplitlines(self, 'bitxor_with_different_widths.asm', ['1101000100100111', '00001101', '0000000000101010', '00101010',])

    def testArithmeticShiftLeft(self):
        runTestSplitlines(self, 'arithmetic_shift_left.asm', ['10000000000000000000000001100000', '10000000000000000000011000000000', '1000',])

    def testArithmeticShiftLeftToVoid(self):
        runTestSplitlines(self, 'ashl_to_void.asm', ['10000000000000000000000001100000', '10000000000000000000011000000000',])

    def testArithmeticShiftRight(self):
        runTestSplitlines(self, 'arithmetic_shift_right.asm', ['10000000000000000000000001100001', '11111000000000000000000000000110', '0001'])

    def testArithmeticShiftRightToVoid(self):
        runTestSplitlines(self, 'ashr_to_void.asm', ['10000000000000000000000001100000', '11111000000000000000000000000110',])

    def testRol(self):
        runTestSplitlines(self, 'rol.asm', ['10011101', '01110110',])

    def testRor(self):
        runTestSplitlines(self, 'ror.asm', ['10011101', '01100111',])

    def testBitAt(self):
        runTestSplitlines(self, 'bitat.asm', ['10011101', 'true', 'false',])

    def testBitSet(self):
        runTestSplitlines(self, 'bitset.asm', ['10000000', '00000001',])

    def testLogicalShiftLeft(self):
        runTestSplitlines(self, 'shl.asm', ['10100000', '10000000', '10'])

    def testLogicalShiftLeftToVoid(self):
        runTestSplitlines(self, 'shl_to_void.asm', ['00100000', '10000000',])

    def testShlOvershift(self):
        runTestSplitlines(self, 'shl_overshift.asm', ['10010111', '00000000', '1001011100000000'])

    def testLogicalShiftRight(self):
        runTestSplitlines(self, 'shr.asm', ['10000001', '00100000', '01',])

    def testLogicalShiftRightToVoid(self):
        runTestSplitlines(self, 'shr_to_void.asm', ['10000001', '00100000',])

    def testShrOvershift(self):
        runTestSplitlines(self, 'shr_overshift.asm', ['10010111', '00000000', '0000000010010111'])

    def testLiterals(self):
        runTestSplitlines(self, 'literals.asm', [
            '11011110101011011011111011101111',
            '11011110101011011011111011101111',
            '11011110101011011011111011101111',
        ])


class BitsSignedWrappingArithmeticTests(unittest.TestCase):
    PATH = './sample/asm/bits/arithmetic/signed_wrapping'

    def test_basic_addition(self):
        runTestSplitlines(self, 'basic_addition.asm', [
            '00000011',
            '00000110',
            '00001001',
        ])

    def test_overflowing_addition(self):
        runTestSplitlines(self, 'overflowing_addition.asm', [
            '01111111',
            '00000111',
            '10000110',
        ])

    def test_maximum_maximum_addition(self):
        runTestSplitlines(self, 'maximum_maximum_addition.asm', [
            '01111111',
            '01111111',
            '11111110',
        ])

    def test_minimum_minimum_addition(self):
        runTestSplitlines(self, 'minimum_minimum_addition.asm', [
            '10000000',
            '10000000',
            '00000000',
        ])

    def test_maximum_minimum_addition(self):
        runTestSplitlines(self, 'maximum_minimum_addition.asm', [
            '01111111',
            '10000000',
            '11111111',
        ])

    def test_minimum_maximum_addition(self):
        runTestSplitlines(self, 'minimum_maximum_addition.asm', [
            '10000000',
            '01111111',
            '11111111',
        ])

    def test_maximum_maximum_subtraction(self):
        runTestSplitlines(self, 'maximum_maximum_subtraction.asm', [
            '01111111',
            '01111111',
            '00000000',
        ])

    def test_minimum_minimum_subtraction(self):
        runTestSplitlines(self, 'minimum_minimum_subtraction.asm', [
            '10000000',
            '10000000',
            '00000000',
        ])

    def test_maximum_minimum_subtraction(self):
        runTestSplitlines(self, 'maximum_minimum_subtraction.asm', [
            '01111111',
            '10000000',
            '11111111',
        ])

    def test_minimum_maximum_subtraction(self):
        runTestSplitlines(self, 'minimum_maximum_subtraction.asm', [
            '10000000',
            '01111111',
            '00000001',
        ])

    def test_zero_maximum_subtraction(self):
        runTestSplitlines(self, 'zero_maximum_subtraction.asm', [
            '00000000',
            '01111111',
            '10000001',
        ])

    def test_zero_minimum_subtraction(self):
        runTestSplitlines(self, 'zero_minimum_subtraction.asm', [
            '00000000',
            '10000000',
            '10000000',
        ])

    def test_maximum_increment(self):
        runTestSplitlines(self, 'maximum_increment.asm', [
            '10000000',
        ])

    def test_minimum_decrement(self):
        runTestSplitlines(self, 'minimum_decrement.asm', [
            '01111111',
        ])

    def test_basic_multiplication(self):
        runTestSplitlines(self, 'basic_multiplication.asm', [
            '00000011',
            '00000110',
            '00010010',
        ])

    def test_overflowing_64x2_multiplication(self):
        runTestSplitlines(self, 'overflowing_64x2_multiplication.asm', [
            '01000000',
            '00000010',
            '10000000',
        ])

    def test_maximum_maximum_multiplication(self):
        runTestSplitlines(self, 'maximum_maximum_multiplication.asm', [
            '01111111',
            '01111111',
            '00000001',
        ])

    def test_minimum_minimum_multiplication(self):
        runTestSplitlines(self, 'minimum_minimum_multiplication.asm', [
            '10000000',
            '10000000',
            '00000000',
        ])

    def test_maximum_minimum_multiplication(self):
        runTestSplitlines(self, 'maximum_minimum_multiplication.asm', [
            '01111111',
            '10000000',
            '10000000',
        ])

    def test_minimum_maximum_multiplication(self):
        runTestSplitlines(self, 'minimum_maximum_multiplication.asm', [
            '10000000',
            '01111111',
            '10000000',
        ])

    def test_ones_by_zeroes_multiplication(self):
        runTestSplitlines(self, 'ones_by_zeroes_multiplication.asm', [
            '11111111',
            '00000000',
            '00000000',
        ])

    def test_basic_division(self):
        runTestSplitlines(self, 'basic_division.asm', [
            '00010010',
            '00000010',
            '00001001',
        ])

    def test_x_x_division(self):
        runTestSplitlines(self, 'x_x_division.asm', [
            '00001010',
            '00001010',
            '00000001',
        ])

    def test_maximum_maximum_division(self):
        runTestSplitlines(self, 'maximum_maximum_division.asm', [
            '01111111',
            '01111111',
            '00000001',
        ])

    def test_minimum_minimum_division(self):
        runTestSplitlines(self, 'minimum_minimum_division.asm', [
            '10000000',
            '10000000',
            '00000001',
        ])

    def test_maximum_minimum_division(self):
        runTestSplitlines(self, 'maximum_minimum_division.asm', [
            '01111111',
            '10000000',
            '00000000',
        ])

    def test_minimum_maximum_division(self):
        runTestSplitlines(self, 'minimum_maximum_division.asm', [
            '10000000',
            '01111111',
            '11111111',
        ])

    def test_maximum_minus_1_division(self):
        runTestSplitlines(self, 'maximum_minus_1_division.asm', [
            '01111111',
            '11111111',
            '10000001',
        ])

    def test_minimum_minus_1_division(self):
        runTestSplitlines(self, 'minimum_minus_1_division.asm', [
            '10000000',
            '11111111',
            '10000000',
        ])

    def test_x_zero_division(self):
        runTestThrowsException(self, 'x_zero_division.asm', ('Exception', 'division by zero',))

    def test_zero_x_division(self):
        runTestSplitlines(self, 'zero_x_division.asm', [
            '00000000',
            '00000001',
            '00000000',
        ])

class BitsUnsignedWrappingArithmeticTests(unittest.TestCase):
    PATH = './sample/asm/bits/arithmetic/unsigned_wrapping'

    def test_maximum_increment(self):
        runTestSplitlines(self, 'maximum_increment.asm', [
            '00000000',
        ])

    def test_minimum_decrement(self):
        runTestSplitlines(self, 'minimum_decrement.asm', [
            '11111111',
        ])

class BitsSignedCheckedArithmeticTests(unittest.TestCase):
    PATH = './sample/asm/bits/arithmetic/signed_checked'

    def test_basic_addition(self):
        runTestSplitlines(self, 'basic_addition.asm', [
            '00000011',
            '00000110',
            '00001001',
        ])

    def test_overflowing_addition_two_positives_give_negative(self):
        runTestThrowsException(self, 'overflowing_addition.asm', ('Exception', 'CheckedArithmeticAdditionSignedOverflow'))

    def test_overflowing_addition_two_negatives_give_positive(self):
        runTestThrowsException(self, 'overflowing_addition_two_negatives_give_positive.asm', ('Exception', 'CheckedArithmeticAdditionSignedOverflow'))

    def test_addition_gives_negative_result(self):
        runTestSplitlines(self, 'addition_gives_negative_result.asm', [
            '00001001',
            '11101111',
            '11111000',
        ])

    def test_maximum_increment(self):
        runTestThrowsException(self, 'maximum_increment.asm', ('Exception', 'CheckedArithmeticIncrementSignedOverflow'))

    def test_minimum_decrement(self):
        runTestThrowsException(self, 'minimum_decrement.asm', ('Exception', 'CheckedArithmeticDecrementSignedOverflow'))

    def test_decrement_from_positive_to_negative(self):
        runTestSplitlines(self, 'decrement_from_positive_to_negative.asm', [
            '11111111',
        ])

    def test_increment_from_negative_to_positive(self):
        runTestSplitlines(self, 'increment_from_negative_to_positive.asm', [
            '00000000',
        ])

    def test_multiplication_negative_negative_gives_positive(self):
        runTestSplitlines(self, 'multiplication_negative_negative_gives_positive.asm', [
            '11111110',
            '11111110',
            '00000100',
        ])

    def test_multiplication_negative_positive_gives_negative(self):
        runTestSplitlines(self, 'multiplication_negative_positive_gives_negative.asm', [
            '11111110',
            '00000010',
            '11111100',
        ])

    def test_multiplication_positive_negative_gives_negative(self):
        runTestSplitlines(self, 'multiplication_positive_negative_gives_negative.asm', [
            '00000010',
            '11111110',
            '11111100',
        ])

    def test_maximum_maximum_subtraction(self):
        runTestSplitlines(self, 'maximum_maximum_subtraction.asm', [
            '01111111',
            '01111111',
            '00000000',
        ])

    def test_maximum_minus_one_subtraction(self):
        runTestThrowsException(self, 'maximum_minus_one_subtraction.asm', ('Exception',
            'CheckedArithmeticSubtractionSignedOverflow'))

    def test_minimum_minimum_subtraction(self):
        runTestSplitlines(self, 'minimum_minimum_subtraction.asm', [
            '10000000',
            '10000000',
            '00000000',
        ])

    def test_minimum_one_subtraction(self):
        runTestThrowsException(self, 'minimum_one_subtraction.asm', ('Exception',
            'CheckedArithmeticSubtractionSignedOverflow'))

    def test_overflowing_minimum_minus_1_multiplication(self):
        runTestThrowsException(self, 'overflowing_minimum_minus_1_multiplication.asm', ('Exception', 'CheckedArithmeticMultiplicationSignedOverflow'))

    def test_overflowing_64x2_multiplication(self):
        runTestThrowsException(self, 'overflowing_64x2_multiplication.asm', ('Exception', 'CheckedArithmeticMultiplicationSignedOverflow'))

    def test_overflowing_64x64_multiplication(self):
        runTestThrowsException(self, 'overflowing_64x64_multiplication.asm', ('Exception', 'CheckedArithmeticMultiplicationSignedOverflow'))

    def test_basic_division(self):
        runTestSplitlines(self, 'basic_division.asm', [
            '00010010',
            '00000010',
            '00001001',
        ])

    def test_42_7_division(self):
        runTestSplitlines(self, '42_7_division.asm', [
            '00101010',
            '00000111',
            '00000110',
        ])

    def test_minus_42_7_division(self):
        runTestSplitlines(self, 'minus_42_7_division.asm', [
            '11010110',
            '00000111',
            '11111010',
        ])

    def test_42_minus_7_division(self):
        runTestSplitlines(self, '42_minus_7_division.asm', [
            '00101010',
            '11111001',
            '11111010',
        ])

    def test_minus_42_minus_7_division(self):
        runTestSplitlines(self, 'minus_42_minus_7_division.asm', [
            '11010110',
            '11111001',
            '00000110',
        ])

    def test_x_x_division(self):
        runTestSplitlines(self, 'x_x_division.asm', [
            '00001010',
            '00001010',
            '00000001',
        ])

    def test_minimum_minus_1_division(self):
        runTestThrowsException(self, 'minimum_minus_1_division.asm', ('Exception', 'CheckedArithmeticDivisionSignedOverflow'))

    def test_x_zero_division(self):
        runTestThrowsException(self, 'x_zero_division.asm', ('Exception', 'division by zero',))

class BitsUnsignedCheckedArithmeticTests(unittest.TestCase):
    PATH = './sample/asm/bits/arithmetic/unsigned_checked'

class BitsSignedSaturatingArithmeticTests(unittest.TestCase):
    PATH = './sample/asm/bits/arithmetic/signed_saturating'

    def test_maximum_increment(self):
        runTestSplitlines(self, 'maximum_increment.asm', [
            '01111111',
            '01111111',
        ])

    def test_minimum_decrement(self):
        runTestSplitlines(self, 'minimum_decrement.asm', [
            '10000000',
            '10000000',
        ])

    def test_basic_addition(self):
        runTestSplitlines(self, 'basic_addition.asm', [
            '00000011',
            '00000110',
            '00001001',
        ])

    def test_max_max_addition(self):
        runTestSplitlines(self, 'max_max_addition.asm', [
            '01111111',
            '01111111',
            '01111111',
        ])

    def test_max_one_addition(self):
        runTestSplitlines(self, 'max_one_addition.asm', [
            '01111111',
            '00000001',
            '01111111',
        ])

    def test_one_max_addition(self):
        runTestSplitlines(self, 'one_max_addition.asm', [
            '00000001',
            '01111111',
            '01111111',
        ])

    def test_mish_mash_both_positive_addition(self):
        runTestSplitlines(self, 'mish_mash_both_positive_addition.asm', [
            '01010101',
            '00101011',
            '01111111',
        ])

    def test_mish_mash_saturating_multiplication(self):
        runTestSplitlines(self, 'mish_mash_saturating_multiplication.asm', [
            '01010101',
            '01101011',
            '01111111',
        ])

    def test_64_and_minus_2_multiplication(self):
        runTestSplitlines(self, '64_and_minus_2_multiplication.asm', [
            '01000000',
            '11111110',
            '10000000',
        ])

    def test_65_and_minus_2_multiplication(self):
        runTestSplitlines(self, '65_and_minus_2_multiplication.asm', [
            '01000001',
            '11111110',
            '10000000',
        ])

    def test_minimum_by_maximum_division(self):
        runTestSplitlines(self, 'minimum_by_maximum_division.asm', [
            '10000000',
            '01111111',
            '11111111',
        ])

    def test_maximum_maximum_subtraction(self):
        runTestSplitlines(self, 'maximum_maximum_subtraction.asm', [
            '01111111',
            '01111111',
            '00000000',
        ])

    def test_maximum_minus_one_subtraction(self):
        runTestSplitlines(self, 'maximum_minus_one_subtraction.asm', [
            '01111111',
            '11111111',
            '01111111',
        ])

    def test_maximum_minimum_subtraction(self):
        runTestSplitlines(self, 'maximum_minimum_subtraction.asm', [
            '01111111',
            '10000000',
            '01111111',
        ])

    def test_minimum_minimum_subtraction(self):
        runTestSplitlines(self, 'minimum_minimum_subtraction.asm', [
            '10000000',
            '10000000',
            '00000000',
        ])

    def test_minimum_one_subtraction(self):
        runTestSplitlines(self, 'minimum_one_subtraction.asm', [
            '10000000',
            '00000001',
            '10000000',
        ])

    def test_minus_one_minimum_subtraction(self):
        runTestSplitlines(self, 'minus_one_minimum_subtraction.asm', [
            '11111111',
            '10000000',
            '01111111',
        ])

    def test_maximum_by_minimum_division(self):
        runTestSplitlines(self, 'maximum_by_minimum_division.asm', [
            '01111111',
            '10000000',
            '00000000',
        ])

    def test_basic_division(self):
        runTestSplitlines(self, 'basic_division.asm', [
            '00010010',
            '00000010',
            '00001001',
        ])

    def test_42_7_division(self):
        runTestSplitlines(self, '42_7_division.asm', [
            '00101010',
            '00000111',
            '00000110',
        ])

    def test_minus_42_7_division(self):
        runTestSplitlines(self, 'minus_42_7_division.asm', [
            '11010110',
            '00000111',
            '11111010',
        ])

    def test_42_minus_7_division(self):
        runTestSplitlines(self, '42_minus_7_division.asm', [
            '00101010',
            '11111001',
            '11111010',
        ])

    def test_minus_42_minus_7_division(self):
        runTestSplitlines(self, 'minus_42_minus_7_division.asm', [
            '11010110',
            '11111001',
            '00000110',
        ])

    def test_x_x_division(self):
        runTestSplitlines(self, 'x_x_division.asm', [
            '00001010',
            '00001010',
            '00000001',
        ])

    def test_minimum_by_minus_1_division(self):
        runTestSplitlines(self, 'minimum_by_minus_1_division.asm', [
            '10000000',
            '11111111',
            '01111111',
        ])

    def test_x_zero_division(self):
        runTestThrowsException(self, 'x_zero_division.asm', ('Exception', 'division by zero',))

class BitsUnsignedSaturatingArithmeticTests(unittest.TestCase):
    PATH = './sample/asm/bits/arithmetic/unsigned_saturating'


class VectorInstructionsTests(unittest.TestCase):
    """Tests for vector-related instructions.

    VECTOR instruction does not get its own test, but is used in every other vector test
    so it gets pretty good coverage.
    """
    PATH = './sample/asm/vector'

    def testPackingVec(self):
        runTest(self, 'vec_packing.asm', '["answer to life", 42]', 0, lambda o: o.strip())

    def testPackingVecRefusesToPackItself(self):
        # pass --no-sa because we want to test runtime exception
        runTestThrowsException(self, 'vec_packing_self_pack.asm', ('Exception', 'vector would pack itself',), assembly_opts=('--no-sa',))

    def testPackingVecRefusesToOutOfRegisterSetRange(self):
        # pass --no-sa because we want to test runtime exception
        runTestThrowsException(self, 'vec_packing_out_of_range.asm', ('Exception', 'vector: packing outside of register set range',), assembly_opts=('--no-sa',))

    def testPackingVecRefusesToPackNullRegister(self):
        # pass --no-sa because we want to test runtime exception
        runTestThrowsException(self, 'vec_packing_null.asm', ('Exception', 'vector: cannot pack null register',), assembly_opts=('--no-sa',))

    def testVLEN(self):
        runTest(self, 'vlen.asm', '8', 0)

    def testVINSERT(self):
        runTest(self, 'vinsert.asm', ['Hurr', 'durr', 'Im\'a', 'sheep!'], 0, lambda o: o.strip().splitlines())

    def testInsertingOutOfRangeWithPositiveIndex(self):
        runTestThrowsException(self, 'out_of_range_index_positive.asm', ('OutOfRangeException', 'positive vector index out of range: index = 5, size = 4',))

    def testVPUSH(self):
        runTest(self, 'vpush.asm', ['0', '1', 'Hello World!'], 0, lambda o: o.strip().splitlines())

    def testVPOP(self):
        runTest(self, 'vpop.asm', ['0', '1', '0', 'Hello World!'], 0, lambda o: o.strip().splitlines())

    def testVPOPWithVoidIndexPopsLast(self):
        runTest(self, 'vpop_with_void_index_pops_last.asm', '[0]')

    def testVPOPWithIndexPopsSpecified(self):
        runTest(self, 'vpop_with_index_pops_specified.asm', '[1]')

    def testVAT(self):
        runTest(self, 'vat.asm', ['0', '1', '1', 'Hello World!'], 0, lambda o: o.strip().splitlines())


class CastingInstructionsTests(unittest.TestCase):
    """Tests for byte instructions.
    """
    PATH = './sample/asm/casts'

    def testITOF(self):
        runTest(self, 'itof.asm', '4.000000')

    def testFTOI(self):
        runTest(self, 'ftoi.asm', '3')

    def testSTOI(self):
        runTest(self, 'stoi.asm', '69')


class RegisterManipulationInstructionsTests(unittest.TestCase):
    """Tests for register-manipulation instructions.
    """
    PATH = './sample/asm/regmod'

    def testCOPY(self):
        runTest(self, 'copy.asm', '1')

    def testMOVE(self):
        runTest(self, 'move.asm', '1')

    def testSWAP(self):
        runTest(self, 'swap.asm', [1, 0], 0, lambda o: [int(i) for i in o.strip().splitlines()])

    def testISNULL(self):
        runTest(self, 'isnull.asm', 'true')

    def testDELETE(self):
        runTest(self, 'delete.asm', 'true')


class PointersTests(unittest.TestCase):
    """Tests for register-manipulation instructions.
    """
    PATH = './sample/asm/pointers'

    def testHelloWorld(self):
        runTest(self, 'hello_world.asm', 'Hello World!')

    def testIncrementDecrement(self):
        runTestReturnsIntegers(self, 'increment_decrement.asm', [43, 42])

    def testIntegerArithmetic(self):
        runTest(self, 'integer_arithmetic.asm', '42')

    def testFloatArithmetic(self):
        runTest(self, 'float_arithmetic.asm', '3.550000')

    def testLogicalAnd(self):
        runTest(self, 'and.asm', 'false')

    def testLogicalNot(self):
        runTest(self, 'not.asm', 'true')

    def testLogicalOr(self):
        runTest(self, 'or.asm', 'true')

    def testCastItof(self):
        runTest(self, 'itof.asm', '0.000000')

    def testCastFtoi(self):
        runTest(self, 'ftoi.asm', '0')

    def testCastStoi(self):
        runTest(self, 'stoi.asm', '42')

    def testCastStof(self):
        runTest(self, 'stof.asm', '3.140000')

    def testCaptureCopy(self):
        # FIXME: passing custom assembler options will not be needed once .closure: support is completely implemented
        runTest(self, 'capturecopy.asm', 'Hello World!', assembly_opts=('--no-sa',))

    def testFcall(self):
        runTestReturnsIntegers(self, 'fcall.asm', [2, 1])

    def testIf(self):
        runTest(self, 'if.asm', 'Nope')

    def testVinsertPointerDereferenceAsSource(self):
        runTestSplitlines(self, 'vinsert_pointer_dereference_as_source.asm', ['[0]', '0'])

    def testVinsertPointerDereferenceAsTarget(self):
        runTest(self, 'vinsert_pointer_dereference_as_target.asm', '[0]')

    def testVpushPointerDereferenceAsSource(self):
        runTestSplitlines(self, 'vpush_pointer_dereference_as_source.asm', ['[0]', '0'])


class SampleProgramsTests(unittest.TestCase):
    """Tests for various sample programs.

    These tests (as well as samples) use several types of instructions and/or
    assembler directives and as such are more stressing for both assembler and CPU.
    """
    PATH = './sample/asm'

    def testCalculatingIntegerPowerOf(self):
        runTest(self, 'power_of.asm', '64')

    def testCalculatingAbsoluteValueOfAnInteger(self):
        runTest(self, 'abs.asm', '17')

    def testLooping(self):
        runTestReturnsIntegers(self, 'looping.asm', [i for i in range(0, 11)])

    def testRegisterReferencesInIntegerOperands(self):
        runTestReturnsIntegers(self, 'registerref.asm', [16, 1, 1, 16])

    def testCalculatingFactorial(self):
        """The code that is tested by this unit is not the best implementation of factorial calculation.
        However, it tests passing parameters by value and by reference;
        so we got that going for us what is nice.
        """
        runTest(self, 'factorial.asm', '40320')

    def testCalculatingFactorialPassingAccumulatorByMove(self):
        """The code that is tested by this unit is not the best implementation of factorial calculation.
        However, it tests passing parameters by value and by reference;
        so we got that going for us what is nice.
        """
        runTest(self, 'factorial_accumulator_by_move.asm', '40320')

    def testCalculatingFactorialUsingTailcalls(self):
        runTest(self, 'factorial_tailcall.asm', '40320')

    def testIterativeFibonacciNumbers(self):
        """45. Fibonacci number calculated iteratively.
        """
        # FIXME: SA needs basic support for static register set
        runTest(self, 'iterfib.asm', 1134903170, 0, lambda o: int(o.strip()), assembly_opts=('--no-sa',))


class FunctionTests(unittest.TestCase):
    """Tests for function related parts of the VM.
    """
    PATH = './sample/asm/functions'

    def testBasicFunctionSupport(self):
        runTest(self, 'definition.asm', 42, 0, lambda o: int(o.strip()))

    def testNestedFunctionCallSupport(self):
        runTestReturnsIntegers(self, 'nested_calls.asm', [2015, 1995, 69, 42])

    def testRecursiveCallFunctionSupport(self):
        runTestReturnsIntegers(self, 'recursive.asm', [i for i in range(9, -1, -1)])

    def testLocalRegistersInFunctions(self):
        runTestReturnsIntegers(self, 'local_registers.asm', [42, 666], assembly_opts=('--no-sa',))

    def testObtainingNumberOfParameters(self):
        runTestReturnsIntegers(self, 'argc.asm', [1, 2, 0])

    def testObtainingVectorWithPassedParameters(self):
        assemble('./src/stdlib/viua/misc.asm', './misc.vlib', opts=('-c',))
        runTest(self, 'parameters_vector.asm', '[0, 1, 2, 3]', assembly_opts=('--no-sa',))

    def testReturningReferences(self):
        # FIXME: disassembler must understand the .closure: directive
        # for now, the --no-sa flag and everything's gonna be find, believe me
        runTest(self, 'return_by_reference.asm', 42, 0, lambda o: int(o.strip()), assembly_opts=('--no-sa',))

    def testStaticRegisters(self):
        # FIXME: SA needs basic support for static register set
        runTestReturnsIntegers(self, 'static_registers.asm', [i for i in range(0, 10)], assembly_opts=('--no-sa',))

    def testCallWithPassByMove(self):
        runTestSplitlines(self, 'pass_by_move.asm', ['42', '42', '42',])

    @unittest.skip('functions not ending with "return" or "tailcall" are forbidden')
    def testNeverendingFunction(self):
        runTestSplitlines(self, 'neverending.asm', ['42', '48'], assembly_opts=('--no-sa',))

    @unittest.skip('functions not ending with "return" or "tailcall" are forbidden')
    def testNeverendingFunction0(self):
        runTestThrowsException(self, 'neverending0.asm', ('Exception', 'stack size (8192) exceeded with call to \'one/0\'',), assembly_opts=('--no-sa',))


class HigherOrderFunctionTests(unittest.TestCase):
    """Tests for higher-order function support.
    """
    PATH = './sample/asm/functions/higher_order'

    def testApply(self):
        runTest(self, 'apply.asm', '25')

    def testApplyByMove(self):
        runTest(self, 'apply_by_move.asm', '25')

    def testInvoke(self):
        runTestSplitlines(self, 'invoke.asm', ['42', '42'])

    def testMap(self):
        runTest(self, 'map.asm', [[1, 2, 3, 4, 5], [1, 4, 9, 16, 25]], 0, lambda o: [json.loads(i) for i in o.splitlines()])

    def testMapVectorByMove(self):
        runTest(self, 'map_vector_by_move.asm', [[1, 2, 3, 4, 5], [1, 4, 9, 16, 25]], 0, lambda o: [json.loads(i) for i in o.splitlines()])

    def testFilter(self):
        runTest(self, 'filter.asm', [[1, 2, 3, 4, 5], [2, 4]], 0, lambda o: [json.loads(i) for i in o.splitlines()])

    def testFilterVectorByMove(self):
        runTest(self, 'filter_vector_by_move.asm', [[1, 2, 3, 4, 5], [2, 4]], 0, lambda o: [json.loads(i) for i in o.splitlines()])

    def testFilterByClosure(self):
        # FIXME --no-sa may be removed once closures are differentatied from functions
        runTest(self, 'filter_closure.asm', [[1, 2, 3, 4, 5], [2, 4]], 0, lambda o: [json.loads(i) for i in o.splitlines()], assembly_opts=('--no-sa',))

    def testFilterByClosureVectorByMove(self):
        # FIXME --no-sa may be removed once closures are differentatied from functions
        runTest(self, 'filter_closure_vector_by_move.asm', [[1, 2, 3, 4, 5], [2, 4]], 0, lambda o: [json.loads(i) for i in o.splitlines()], assembly_opts=('--no-sa',))

    def testTailcallOfObject(self):
        runTestThrowsExceptionJSON(self, 'tailcall_of_object.asm', {'frame': {}, 'trace': ['main/0/0()', 'foo/0/0()',], 'uncaught': {'type': 'Integer', 'value': '42',}}, output_processing_function=lambda s: json.loads(s.strip()))

    def testTailcallOfClosure(self):
        runTestThrowsExceptionJSON(self, 'tailcall_of_closure.asm', {'frame': {}, 'trace': ['main/0/0()', 'test/0/0()',], 'uncaught': {'type': 'Integer', 'value': '42',}}, assembly_opts=('--no-sa',), output_processing_function=lambda s: json.loads(s.strip()))


class ClosureTests(unittest.TestCase):
    """Tests for closures.
    """
    PATH = './sample/asm/functions/closures'

    def testSimpleClosure(self):
        # FIXME: passing custom assembler options will not be needed once .closure: support is completely implemented
        runTest(self, 'simple.asm', '42', output_processing_function=None, assembly_opts=('--no-sa',))

    def testVariableSharingBetweenTwoClosures(self):
        # FIXME: passing custom assembler options will not be needed once .closure: support is completely implemented
        runTestReturnsIntegers(self, 'shared_variables.asm', [42, 69], assembly_opts=('--no-sa',))

    def testAdder(self):
        # FIXME: passing custom assembler options will not be needed once .closure: support is completely implemented
        runTestReturnsIntegers(self, 'adder.asm', [5, 8, 16], assembly_opts=('--no-sa',))

    def testCapturedVariableLeftInScope(self):
        # FIXME: passing custom assembler options will not be needed once .closure: support is completely implemented
        runTestSplitlines(self, 'captured_variable_left_in_scope.asm', ['Hello World!', '42'], assembly_opts=('--no-sa',))

    def testChangeCapturedVariableFromClosure(self):
        # FIXME: passing custom assembler options will not be needed once .closure: support is completely implemented
        runTestSplitlines(self, 'change_enclosed_variable_from_closure.asm', ['Hello World!', '42'], assembly_opts=('--no-sa',))

    def testNestedClosures(self):
        # FIXME: passing custom assembler options will not be needed once .closure: support is completely implemented
        runTest(self, 'nested_closures.asm', '10', assembly_opts=('--no-sa',))

    def testSimpleCaptureByCopy(self):
        # FIXME: passing custom assembler options will not be needed once .closure: support is completely implemented
        runTest(self, 'simple_enclose_by_copy.asm', '42', assembly_opts=('--no-sa',))

    def testCaptureCopyCreatesIndependentObjects(self):
        # FIXME: passing custom assembler options will not be needed once .closure: support is completely implemented
        runTestSplitlines(self, 'capturecopy_creates_independent_objects.asm', ['Hello World!', 'Hello World!', '42', 'Hello World!'], assembly_opts=('--no-sa',))

    def testSimpleCaptureByMove(self):
        # FIXME: passing custom assembler options will not be needed once .closure: support is completely implemented
        runTestSplitlines(self, 'simple_enclose_by_move.asm', ['true', 'Hello World!'], assembly_opts=('--no-sa',))


class InvalidInstructionOperandTypeTests(unittest.TestCase):
    """Tests checking detection of invalid operand type during instruction execution.
    """
    PATH = './sample/asm/invalid_operand_types'

    # Why --no-sa flag?
    # Because these tests are for the runtime checks, so if the static analyser will complain
    # about errors at compile time they will not even get the chance to run...
    ASM_FLAGS = ('--no-sa',)

    def testIADD(self):
        runTestThrowsException(self, 'iadd.asm', ('Exception', "fetched invalid type: expected 'Number' but got 'String'",))

    def testISUB(self):
        runTestThrowsException(self, 'isub.asm', ('Exception', "fetched invalid type: expected 'Number' but got 'String'",))

    def testIMUL(self):
        runTestThrowsException(self, 'imul.asm', ('Exception', "fetched invalid type: expected 'Number' but got 'String'",))

    def testIDIV(self):
        runTestThrowsException(self, 'idiv.asm', ('Exception', "fetched invalid type: expected 'Number' but got 'String'",))

    def testILT(self):
        runTestThrowsException(self, 'ilt.asm',
                ('Exception', "fetched invalid type: expected 'Number' but got 'String'",))

    def testILTE(self):
        runTestThrowsException(self, 'ilte.asm',
                ('Exception', "fetched invalid type: expected 'Number' but got 'Text'",))

    def testIGT(self):
        runTestThrowsException(self, 'igt.asm',
                ('Exception', "fetched invalid type: expected 'Number' but got 'Text'",))

    def testIGTE(self):
        runTestThrowsException(self, 'igte.asm',
                ('Exception', "fetched invalid type: expected 'Number' but got 'Text'",))

    def testIEQ(self):
        runTestThrowsException(self, 'ieq.asm',
                ('Exception', "fetched invalid type: expected 'Number' but got 'Text'",))

    def testIINC(self):
        runTestThrowsException(self, 'iinc.asm',
                ('Exception', "fetched invalid type: expected 'Integer' but got 'Text'",))

    def testIDEC(self):
        runTestThrowsException(self, 'idec.asm', ('Exception', "fetched invalid type: expected 'Integer' but got 'Function'",))

    def testFADD(self):
        runTestThrowsException(self, 'fadd.asm',
        ('Exception', "fetched invalid type: expected 'Number' but got 'Text'",))

    def testFSUB(self):
        runTestThrowsException(self, 'fsub.asm',
                ('Exception', "fetched invalid type: expected 'Number' but got 'Text'",))

    def testFMUL(self):
        runTestThrowsException(self, 'fmul.asm',
                ('Exception', "fetched invalid type: expected 'Number' but got 'Text'",))

    def testFDIV(self):
        runTestThrowsException(self, 'fdiv.asm',
                ('Exception', "fetched invalid type: expected 'Number' but got 'Text'",))

    def testFLT(self):
        runTestThrowsException(self, 'flt.asm',
                ('Exception', "fetched invalid type: expected 'Number' but got 'Text'",))

    def testFLTE(self):
        runTestThrowsException(self, 'flte.asm',
                ('Exception', "fetched invalid type: expected 'Number' but got 'Text'",))

    def testFGT(self):
        runTestThrowsException(self, 'fgt.asm',
                ('Exception', "fetched invalid type: expected 'Number' but got 'Text'",))

    def testFGTE(self):
        runTestThrowsException(self, 'fgte.asm',
                ('Exception', "fetched invalid type: expected 'Number' but got 'Text'",))

    def testFEQ(self):
        runTestThrowsException(self, 'feq.asm',
                ('Exception', "fetched invalid type: expected 'Number' but got 'Text'",))


class StaticLinkingTests(unittest.TestCase):
    """Tests for static linking functionality.
    """
    PATH = './sample/asm/linking/static'

    def testLinkingBasic(self):
        lib_name = 'print_N.asm'
        assembly_lib_path = os.path.join(self.PATH, lib_name)
        compiled_lib_path = os.path.join(COMPILED_SAMPLES_PATH, (lib_name + '.wlib'))
        assemble(assembly_lib_path, compiled_lib_path, opts=('--lib',))
        bin_name = 'links.asm'
        assembly_bin_path = os.path.join(self.PATH, bin_name)
        compiled_bin_path = os.path.join(COMPILED_SAMPLES_PATH, (bin_name + '.bin'))
        assemble(assembly_bin_path, compiled_bin_path, links=(compiled_lib_path,))
        excode, output, error = run(compiled_bin_path)
        self.assertEqual('42', output.strip())
        self.assertEqual(0, excode)

    def testLinkingMainFunction(self):
        lib_name = 'main_main.asm'
        assembly_lib_path = os.path.join(self.PATH, lib_name)
        compiled_lib_path = os.path.join(COMPILED_SAMPLES_PATH, (lib_name + '.vlib'))
        assemble(assembly_lib_path, compiled_lib_path, opts=('--lib',))
        bin_name = 'main_link.asm'
        assembly_bin_path = os.path.join(self.PATH, bin_name)
        compiled_bin_path = os.path.join(COMPILED_SAMPLES_PATH, (bin_name + '.bin'))
        assemble(assembly_bin_path, compiled_bin_path, links=(compiled_lib_path,))
        excode, output, error = run(compiled_bin_path)
        self.assertEqual('Hello World!', output.strip())
        self.assertEqual(0, excode)

    def testLinkingCodeWithBranchesAndJumps(self):
        lib_name = 'jumplib.asm'
        assembly_lib_path = os.path.join(self.PATH, lib_name)
        compiled_lib_path = os.path.join(COMPILED_SAMPLES_PATH, (lib_name + '.wlib'))
        assemble(assembly_lib_path, compiled_lib_path, opts=('--lib',))
        bin_name = 'jumplink.asm'
        assembly_bin_path = os.path.join(self.PATH, bin_name)
        compiled_bin_path = os.path.join(COMPILED_SAMPLES_PATH, (bin_name + '.bin'))
        assemble(assembly_bin_path, compiled_bin_path, links=(compiled_lib_path,))
        excode, output, error = run(compiled_bin_path)
        self.assertEqual(['42', ':-)'], output.strip().splitlines())
        self.assertEqual(0, excode)


class JumpingTests(unittest.TestCase):
    """
    """
    PATH = './sample/asm/absolute_jumping'

    def testRelativeJump(self):
        runTest(self, 'relative_jumps.asm', "Hello World")

    def testRelativeBranch(self):
        runTest(self, 'relative_branch.asm', "Hello World")


class TryCatchBlockTests(unittest.TestCase):
    """Tests for user code thrown exceptions.
    """
    PATH = './sample/asm/blocks'

    def testBasicNoThrowNoCatchBlock(self):
        # FIXME implement running block checks as they are entered; then, default assembly options may
        # be used
        runTest(self, 'basic.asm', '42')

    def testCatchingBuiltinType(self):
        runTest(self, 'catching_builtin_type.asm', '42')


@unittest.skip('new SA is almost ready')
class AssemblerStaticAnalysisErrorTests(unittest.TestCase):
    PATH = './sample/asm/static_analysis_errors'

    def testMoveFromEmptyRegister(self):
        runTestFailsToAssemble(self, 'move_from_empty_register.asm', './sample/asm/static_analysis_errors/move_from_empty_register.asm:21:13: error: move from empty register: 0')

    def testCopyFromEmptyRegister(self):
        runTestFailsToAssemble(self, 'copy_from_empty_register.asm', './sample/asm/static_analysis_errors/copy_from_empty_register.asm:21:13: error: copy from empty register: 0')

    def testDeleteOfEmptyRegister(self):
        runTestFailsToAssemble(self, 'delete_of_empty_register.asm', './sample/asm/static_analysis_errors/delete_of_empty_register.asm:21:12: error: delete of empty register: 1')

    def testParameterPassFromEmptyRegister(self):
        runTestFailsToAssemble(self, 'parameter_pass_from_empty_register.asm', './sample/asm/static_analysis_errors/parameter_pass_from_empty_register.asm:26:14: error: parameter pass from empty register: 1')

    def testParameterMoveFromEmptyRegister(self):
        runTestFailsToAssemble(self, 'parameter_move_from_empty_register.asm', './sample/asm/static_analysis_errors/parameter_move_from_empty_register.asm:26:13: error: parameter move from empty register: 1')

    def testParameterMoveEmptiesRegisters(self):
        runTestFailsToAssemble(self, 'parameter_move_empties_registers.asm', './sample/asm/static_analysis_errors/parameter_move_empties_registers.asm:30:11: error: print of empty register: 1')

    def testSwapWithEmptyFirstRegister(self):
        runTestFailsToAssemble(self, 'swap_with_empty_first_register.asm', './sample/asm/static_analysis_errors/swap_with_empty_first_register.asm:21:10: error: swap with empty register: 1')

    def testSwapWithEmptySecondRegister(self):
        runTestFailsToAssemble(self, 'swap_with_empty_second_register.asm', './sample/asm/static_analysis_errors/swap_with_empty_second_register.asm:22:13: error: swap with empty register: 2')

    def testCaptureEmptyRegisterByCopy(self):
        runTestFailsToAssemble(self, 'capture_empty_register_by_copy.asm', './sample/asm/static_analysis_errors/capture_empty_register_by_copy.asm:21:20: error: closure of empty register: 1')

    def testCaptureEmptyRegisterByMove(self):
        runTestFailsToAssemble(self, 'capture_empty_register_by_move.asm', './sample/asm/static_analysis_errors/capture_empty_register_by_move.asm:21:20: error: closure of empty register: 1')

    def testCaptureEmptyRegisterByReference(self):
        runTestFailsToAssemble(self, 'capture_empty_register_by_reference.asm', './sample/asm/static_analysis_errors/capture_empty_register_by_reference.asm:21:16: error: closure of empty register: 1')

    def testEchoOfEmptyRegister(self):
        runTestFailsToAssemble(self, 'echo_of_empty_register.asm', './sample/asm/static_analysis_errors/echo_of_empty_register.asm:21:10: error: echo of empty register: 1')

    def testPrintOfEmptyRegister(self):
        runTestFailsToAssemble(self, 'print_of_empty_register.asm', './sample/asm/static_analysis_errors/print_of_empty_register.asm:21:11: error: print of empty register: 1')

    def testBranchDependsOnEmptyRegister(self):
        runTestFailsToAssemble(self, 'branch_depends_on_empty_register.asm', './sample/asm/static_analysis_errors/branch_depends_on_empty_register.asm:21:8: error: branch depends on empty register: 1')

    def testPackingVecEmptiesRegisters(self):
        runTestFailsToAssemble(self, 'packing_vec_empties_registers.asm', './sample/asm/static_analysis_errors/packing_vec_empties_registers.asm:26:11: error: print of empty register: 1')

    def testPackingEmptyRegister(self):
        runTestFailsToAssemble(self, 'packing_empty_register.asm', './sample/asm/static_analysis_errors/packing_empty_register.asm:23:5: error: packing empty register: 4')

    def testUseOfEmptyFirstRegisterInAnd(self):
        runTestFailsToAssemble(self, 'and_use_of_empty_register_1st.asm', './sample/asm/static_analysis_errors/and_use_of_empty_register_1st.asm:21:12: error: use of empty register: 1')

    def testUseOfEmptySecondRegisterInAnd(self):
        runTestFailsToAssemble(self, 'and_use_of_empty_register_2nd.asm', './sample/asm/static_analysis_errors/and_use_of_empty_register_2nd.asm:22:15: error: use of empty register: 2')

    def testUseOfEmptyFirstRegisterInOr(self):
        runTestFailsToAssemble(self, 'or_use_of_empty_register_1st.asm', './sample/asm/static_analysis_errors/or_use_of_empty_register_1st.asm:21:11: error: use of empty register: 1')

    def testUseOfEmptySecondRegisterInOr(self):
        runTestFailsToAssemble(self, 'or_use_of_empty_register_2nd.asm', './sample/asm/static_analysis_errors/or_use_of_empty_register_2nd.asm:22:14: error: use of empty register: 2')

    def testIaddOfEmptyRegisters(self):
        runTestFailsToAssemble(self, 'iadd_of_empty_registers.asm', './sample/asm/static_analysis_errors/iadd_of_empty_registers.asm:21:12: error: use of empty register: 1')

    def testNotOfEmptyRegisters(self):
        runTestFailsToAssemble(self, 'not_of_empty_register.asm', './sample/asm/static_analysis_errors/not_of_empty_register.asm:21:9: error: not of empty register: 1')

    def testCastOfEmptyRegistersFtoi(self):
        runTestFailsToAssemble(self, 'cast_of_empty_register_ftoi.asm', './sample/asm/static_analysis_errors/cast_of_empty_register_ftoi.asm:21:13: error: use of empty register: 1')

    def testCastOfEmptyRegistersItof(self):
        runTestFailsToAssemble(self, 'cast_of_empty_register_itof.asm', './sample/asm/static_analysis_errors/cast_of_empty_register_itof.asm:21:13: error: use of empty register: 1')

    def testCastOfEmptyRegistersStoi(self):
        runTestFailsToAssemble(self, 'cast_of_empty_register_stoi.asm', './sample/asm/static_analysis_errors/cast_of_empty_register_stoi.asm:21:13: error: use of empty register: 1')

    def testCastOfEmptyRegistersStof(self):
        runTestFailsToAssemble(self, 'cast_of_empty_register_stof.asm', './sample/asm/static_analysis_errors/cast_of_empty_register_stof.asm:21:13: error: use of empty register: 1')

    def testVinsertEmptiesRegisters(self):
        runTestFailsToAssemble(self, 'vinsert_empties_registers.asm', './sample/asm/static_analysis_errors/vinsert_empties_registers.asm:23:11: error: print of empty register: 1')

    def testVinsertOfEmptyRegister(self):
        runTestFailsToAssemble(self, 'vinsert_of_empty_register.asm', './sample/asm/static_analysis_errors/vinsert_of_empty_register.asm:21:22: error: vinsert from empty register: 1')

    def testVinsertIntoEmptyRegister(self):
        runTestFailsToAssemble(self, 'vinsert_into_empty_register.asm', './sample/asm/static_analysis_errors/vinsert_into_empty_register.asm:21:13: error: vinsert into empty register: 2')

    def testVpushEmptiesRegisters(self):
        runTestFailsToAssemble(self, 'vpush_empties_registers.asm', "./sample/asm/static_analysis_errors/vpush_empties_registers.asm:22:11: error: print of empty register: 1")

    def testVpushOfEmptyRegister(self):
        runTestFailsToAssemble(self, 'vpush_of_empty_register.asm', "./sample/asm/static_analysis_errors/vpush_of_empty_register.asm:21:20: error: vpush from empty register: 1")

    def testVpushIntoEmptyRegister(self):
        runTestFailsToAssemble(self, 'vpush_into_empty_register.asm', "./sample/asm/static_analysis_errors/vpush_into_empty_register.asm:21:11: error: vpush into empty register: 2")

    def testVpopFromEmptyRegister(self):
        runTestFailsToAssemble(self, 'vpop_from_empty_register.asm', "./sample/asm/static_analysis_errors/vpop_from_empty_register.asm:21:13: error: vpop from empty register: 1")

    def testVatOnEmptyRegister(self):
        runTestFailsToAssemble(self, 'vat_on_empty_register.asm', "./sample/asm/static_analysis_errors/vat_on_empty_register.asm:21:12: error: vat from empty register: 1")

    def testVlenOnEmptyRegister(self):
        runTestFailsToAssemble(self, 'vlen_on_empty_register.asm', "./sample/asm/static_analysis_errors/vlen_on_empty_register.asm:21:13: error: vlen from empty register: 1")

    def testInsertIntoEmptyRegister(self):
        runTestFailsToAssemble(self, 'insert_into_empty_register.asm', "./sample/asm/static_analysis_errors/insert_into_empty_register.asm:23:12: error: insert into empty register: target := 1")

    def testInsertKeyFromEmptyRegister(self):
        runTestFailsToAssemble(self, 'insert_key_from_empty_register.asm', "./sample/asm/static_analysis_errors/insert_key_from_empty_register.asm:23:28: error: insert key from empty register: key := 2")

    def testInsertFromEmptyRegister(self):
        runTestFailsToAssemble(self, 'insert_from_empty_register.asm', "./sample/asm/static_analysis_errors/insert_from_empty_register.asm:23:48: error: insert from empty register: value := 3")

    def testRemoveFromEmptyRegister(self):
        runTestFailsToAssemble(self, 'remove_from_empty_register.asm', "./sample/asm/static_analysis_errors/remove_from_empty_register.asm:25:20: error: remove from empty register: source := 2")

    def testPointerFromEmptyRegister(self):
        runTestFailsToAssemble(self, 'pointer_from_empty_register.asm', "./sample/asm/static_analysis_errors/pointer_from_empty_register.asm:21:12: error: pointer from empty register: 1")

    def testThrowFromEmptyRegister(self):
        runTestFailsToAssemble(self, 'throw_from_empty_register.asm', "./sample/asm/static_analysis_errors/throw_from_empty_register.asm:21:11: error: throw from empty register: 1")

    def testIsnullFailsOnNonemptyRegisters(self):
        runTestFailsToAssemble(self, 'isnull_fails_on_nonempty_registers.asm', "./sample/asm/static_analysis_errors/isnull_fails_on_nonempty_registers.asm:22:19: error: useless check, register will always be defined: 1")

    def testFcallFromEmptyRegister(self):
        runTestFailsToAssemble(self, 'fcall_from_empty_register.asm', "./sample/asm/static_analysis_errors/fcall_from_empty_register.asm:22:15: error: call from empty register: 1")

    def testJoinFromEmptyRegister(self):
        runTestFailsToAssemble(self, 'join_from_empty_register.asm', "./sample/asm/static_analysis_errors/join_from_empty_register.asm:21:13: error: join from empty register: 1")

    def testSendTargetFromEmptyRegister(self):
        runTestFailsToAssemble(self, 'send_target_from_empty_register.asm', "./sample/asm/static_analysis_errors/send_target_from_empty_register.asm:22:10: error: send target from empty register: pid := 1")

    def testSendFromEmptyRegister(self):
        runTestFailsToAssemble(self, 'send_from_empty_register.asm', "./sample/asm/static_analysis_errors/send_from_empty_register.asm:27:13: error: send from empty register: 2")

    def testRegisterNameAlreadyTaken(self):
        runTestFailsToAssemble(self, 'register_name_already_taken.asm', "./sample/asm/static_analysis_errors/register_name_already_taken.asm:22:14: error: register name already taken: named_register")

    def testRegisterUsedBeforeBeingNamed(self):
        runTestFailsToAssemble(self, 'register_defined_before_being_named.asm', "./sample/asm/static_analysis_errors/register_defined_before_being_named.asm:22:12: error: register defined before being named: 1 = named_register")

    def testUselessBranchSimpleMarker(self):
        runTestFailsToAssemble(self, 'useless_branch_simple_marker.asm', "./sample/asm/static_analysis_errors/useless_branch_simple_marker.asm:21:5: error: useless branch: both targets point to the same instruction")

    def testUselessBranchSimpleOffset(self):
        runTestFailsToAssemble(self, 'useless_branch_simple_offset.asm', "./sample/asm/static_analysis_errors/useless_branch_simple_offset.asm:21:5: error: useless branch: both targets point to the same instruction")

    def testUselessBranchSimpleIndex(self):
        runTestFailsToAssemble(self, 'useless_branch_simple_index.asm', "./sample/asm/static_analysis_errors/useless_branch_simple_index.asm:21:5: error: useless branch: both targets point to the same instruction")

    def testUselessBranchMixedIndexOffsetBackward(self):
        runTestFailsToAssemble(self, 'useless_branch_mixed_index_offset_backward.asm', "./sample/asm/static_analysis_errors/useless_branch_mixed_index_offset_backward.asm:24:5: error: useless branch: both targets point to the same instruction")

    def testUselessBranchMixedIndexOffsetForward(self):
        runTestFailsToAssemble(self, 'useless_branch_mixed_index_offset_forward.asm', "./sample/asm/static_analysis_errors/useless_branch_mixed_index_offset_forward.asm:21:5: error: useless branch: both targets point to the same instruction")

    def testUselessBranchMixedMarker(self):
        runTestFailsToAssemble(self, 'useless_branch_mixed_marker.asm', "./sample/asm/static_analysis_errors/useless_branch_mixed_marker.asm:21:5: error: useless branch: both targets point to the same instruction")

    def testUselessBranchMixedMarkerIndex(self):
        runTestFailsToAssemble(self, 'useless_branch_mixed_marker_index.asm', "./sample/asm/static_analysis_errors/useless_branch_mixed_marker_index.asm:21:5: error: useless branch: both targets point to the same instruction")

    def testUselessBranchMixedMarkerOffsetBackward(self):
        runTestFailsToAssemble(self, 'useless_branch_mixed_marker_offset_backward.asm', "./sample/asm/static_analysis_errors/useless_branch_mixed_marker_offset_backward.asm:27:5: error: useless branch: both targets point to the same instruction")

    def testUselessBranchMixedMarkerOffsetForward(self):
        runTestFailsToAssemble(self, 'useless_branch_mixed_marker_offset_forward.asm', "./sample/asm/static_analysis_errors/useless_branch_mixed_marker_offset_forward.asm:21:5: error: useless branch: both targets point to the same instruction")

    def testEmptyRegisterAccessAfterTakingBranchOffsetTrue(self):
        runTestFailsToAssembleDetailed(self, 'sa_taking_true_branch_forward_offset.asm', [
            '27:11: error: print of empty register: value := 1',
            '24:5: error: erased by:',
            '23:15: error: after taking true branch:',
            '20:12: error: in function main/0',
        ])

    def testEmptyRegisterAccessAfterTakingBranchOffsetFalse(self):
        runTestFailsToAssembleDetailed(self, 'sa_taking_false_branch_forward_offset.asm', [
            '27:11: error: print of empty register: value := 1',
            '25:5: error: erased by:',
            '23:18: error: after taking false branch:',
            '20:12: error: in function main/0',
        ])

    def testEmptyRegisterAccessAfterTakingBranchMarkerTrue(self):
        runTestFailsToAssembleDetailed(self, 'sa_taking_true_branch_forward_marker.asm', [
            '27:11: error: print of empty register: value := 1',
            '24:5: error: erased by:',
            '23:15: error: after taking true branch:',
            '20:12: error: in function main/0',
        ])

    def testEmptyRegisterAccessAfterTakingBranchMarkerFalse(self):
        runTestFailsToAssembleDetailed(self, 'sa_taking_false_branch_forward_marker.asm', [
            '27:11: error: print of empty register: value := 1',
            '25:5: error: erased by:',
            '23:18: error: after taking false branch:',
            '20:12: error: in function main/0',
        ])

    def testUseOfEmptyFirstOperandInIadd(self):
        runTestFailsToAssembleDetailed(self, 'use_of_empty_first_operand_in_iadd.asm', [
            '24:30: error: use of empty register: first := 1',
            '20:12: error: in function main/0',
        ])

    def testUseOfEmptySecondOperandInIadd(self):
        runTestFailsToAssembleDetailed(self, 'use_of_empty_second_operand_in_iadd.asm', [
            '24:37: error: use of empty register: second := 2',
            '20:12: error: in function main/0',
        ])

    def testUseOfVoidAsInputRegister(self):
        runTestFailsToAssembleDetailed(self, 'void_as_input_register.asm', [
            '26:17: error: use of void as input register:',
            '24:12: error: in function main/0',
        ])

    def testExpectedOperandFoundNewline(self):
        runTestFailsToAssembleDetailed(self, 'found_newline.asm', [
            '21:10: error: expected operand, found newline',
            '20:12: error: in function main/0',
        ])

    def testMainFunctionUsesInvalidRegisterSetToReturn(self):
        runTestFailsToAssemble(self, 'main_returns_to_invalid_rs_type.asm', "./sample/asm/static_analysis_errors/main_returns_to_invalid_rs_type.asm:21:5: error: main function uses invalid register set to return a value: static")


class AssemblerStaticAnalysisErrorTestsForNewSA(unittest.TestCase):
    PATH = './sample/asm/static_analysis_errors'
    ASM_FLAGS = ('--new-sa',)

    def testMoveFromEmptyRegister(self):
        runTestFailsToAssemble(self, 'move_from_empty_register.asm', './sample/asm/static_analysis_errors/move_from_empty_register.asm:21:13: error: move from empty current register "0"')

    def testCopyFromEmptyRegister(self):
        runTestFailsToAssemble(self, 'copy_from_empty_register.asm', './sample/asm/static_analysis_errors/copy_from_empty_register.asm:21:13: error: copy from empty current register "0"')

    def testDeleteOfEmptyRegister(self):
        runTestFailsToAssemble(self, 'delete_of_empty_register.asm', './sample/asm/static_analysis_errors/delete_of_empty_register.asm:21:12: error: delete of empty current register "1"')

    def testParameterPassFromEmptyRegister(self):
        runTestFailsToAssemble(self, 'parameter_pass_from_empty_register.asm', './sample/asm/static_analysis_errors/parameter_pass_from_empty_register.asm:26:14: error: use of empty current register "1"')

    def testParameterMoveFromEmptyRegister(self):
        runTestFailsToAssemble(self, 'parameter_move_from_empty_register.asm', './sample/asm/static_analysis_errors/parameter_move_from_empty_register.asm:26:13: error: use of empty current register "1"')

    def testParameterMoveEmptiesRegisters(self):
        runTestFailsToAssemble(self, 'parameter_move_empties_registers.asm', './sample/asm/static_analysis_errors/parameter_move_empties_registers.asm:30:11: error: use of erased current register "1"')

    def testSwapWithEmptyFirstRegister(self):
        runTestFailsToAssemble(self, 'swap_with_empty_first_register.asm', './sample/asm/static_analysis_errors/swap_with_empty_first_register.asm:21:10: error: swap with empty current register "1"')

    def testSwapWithEmptySecondRegister(self):
        runTestFailsToAssemble(self, 'swap_with_empty_second_register.asm', './sample/asm/static_analysis_errors/swap_with_empty_second_register.asm:22:13: error: swap with empty current register "2"')

    def testCaptureEmptyRegisterByCopy(self):
        runTestFailsToAssemble(self, 'capture_empty_register_by_copy.asm', './sample/asm/static_analysis_errors/capture_empty_register_by_copy.asm:21:17: error: use of empty current register "2"')

    def testCaptureEmptyRegisterByMove(self):
        runTestFailsToAssemble(self, 'capture_empty_register_by_move.asm', './sample/asm/static_analysis_errors/capture_empty_register_by_move.asm:21:17: error: use of empty current register "2"')

    def testCaptureEmptyRegisterByReference(self):
        runTestFailsToAssemble(self, 'capture_empty_register_by_reference.asm', './sample/asm/static_analysis_errors/capture_empty_register_by_reference.asm:21:13: error: use of empty current register "2"')

    def testEchoOfEmptyRegister(self):
        runTestFailsToAssemble(self, 'echo_of_empty_register.asm', './sample/asm/static_analysis_errors/echo_of_empty_register.asm:21:10: error: use of empty current register "1"')

    def testPrintOfEmptyRegister(self):
        runTestFailsToAssemble(self, 'print_of_empty_register.asm', './sample/asm/static_analysis_errors/print_of_empty_register.asm:21:11: error: use of empty current register "1"')

    def testBranchDependsOnEmptyRegister(self):
        runTestFailsToAssemble(self, 'branch_depends_on_empty_register.asm', './sample/asm/static_analysis_errors/branch_depends_on_empty_register.asm:21:8: error: branch depends on empty current register "1"')

    @unittest.skip('FIXME TODO SA for vector instructions not impemented yet')
    def testPackingVecEmptiesRegisters(self):
        runTestFailsToAssemble(self, 'packing_vec_empties_registers.asm', './sample/asm/static_analysis_errors/packing_vec_empties_registers.asm:26:11: error: use of empty current register "1"')

    @unittest.skip('FIXME TODO SA for vector instructions not impemented yet')
    def testPackingEmptyRegister(self):
        runTestFailsToAssemble(self, 'packing_empty_register.asm', './sample/asm/static_analysis_errors/packing_empty_register.asm:23:5: error: packing empty current register "1"')

    def testUseOfEmptyFirstRegisterInAnd(self):
        runTestFailsToAssemble(self, 'and_use_of_empty_register_1st.asm', './sample/asm/static_analysis_errors/and_use_of_empty_register_1st.asm:21:12: error: use of empty current register "1"')

    def testUseOfEmptySecondRegisterInAnd(self):
        runTestFailsToAssemble(self, 'and_use_of_empty_register_2nd.asm', './sample/asm/static_analysis_errors/and_use_of_empty_register_2nd.asm:22:15: error: use of empty current register "2"')

    def testUseOfEmptyFirstRegisterInOr(self):
        runTestFailsToAssemble(self, 'or_use_of_empty_register_1st.asm', './sample/asm/static_analysis_errors/or_use_of_empty_register_1st.asm:21:11: error: use of empty current register "1"')

    def testUseOfEmptySecondRegisterInOr(self):
        runTestFailsToAssemble(self, 'or_use_of_empty_register_2nd.asm', './sample/asm/static_analysis_errors/or_use_of_empty_register_2nd.asm:22:14: error: use of empty current register "2"')

    def testIaddOfEmptyRegisters(self):
        runTestFailsToAssemble(self, 'iadd_of_empty_registers.asm', './sample/asm/static_analysis_errors/iadd_of_empty_registers.asm:21:12: error: use of empty current register "1"')

    def testNotOfEmptyRegisters(self):
        runTestFailsToAssemble(self, 'not_of_empty_register.asm', './sample/asm/static_analysis_errors/not_of_empty_register.asm:21:9: error: use of empty current register "1"')

    def testCastOfEmptyRegistersFtoi(self):
        runTestFailsToAssemble(self, 'cast_of_empty_register_ftoi.asm', './sample/asm/static_analysis_errors/cast_of_empty_register_ftoi.asm:21:13: error: use of empty current register "1"')

    def testCastOfEmptyRegistersItof(self):
        runTestFailsToAssemble(self, 'cast_of_empty_register_itof.asm', './sample/asm/static_analysis_errors/cast_of_empty_register_itof.asm:21:13: error: use of empty current register "1"')

    def testCastOfEmptyRegistersStoi(self):
        runTestFailsToAssemble(self, 'cast_of_empty_register_stoi.asm', './sample/asm/static_analysis_errors/cast_of_empty_register_stoi.asm:21:13: error: use of empty current register "1"')

    def testCastOfEmptyRegistersStof(self):
        runTestFailsToAssemble(self, 'cast_of_empty_register_stof.asm', './sample/asm/static_analysis_errors/cast_of_empty_register_stof.asm:21:13: error: use of empty current register "1"')

    @unittest.skip('requires Valgrind suppression')
    def testVinsertEmptiesRegisters(self):
        runTestFailsToAssemble(self, 'vinsert_empties_registers.asm', './sample/asm/static_analysis_errors/vinsert_empties_registers.asm:23:11: error: use of empty current register "1"')

    def testVinsertOfEmptyRegister(self):
        runTestFailsToAssemble(self, 'vinsert_of_empty_register.asm', './sample/asm/static_analysis_errors/vinsert_of_empty_register.asm:21:25: error: use of empty current register "1"')

    def testVinsertIntoEmptyRegister(self):
        runTestFailsToAssemble(self, 'vinsert_into_empty_register.asm', './sample/asm/static_analysis_errors/vinsert_into_empty_register.asm:21:13: error: use of empty current register "2"')

    def testVpushEmptiesRegisters(self):
        runTestFailsToAssemble(self, 'vpush_empties_registers.asm', './sample/asm/static_analysis_errors/vpush_empties_registers.asm:22:11: error: use of erased current register "1"')

    def testVpushOfEmptyRegister(self):
        runTestFailsToAssemble(self, 'vpush_of_empty_register.asm', './sample/asm/static_analysis_errors/vpush_of_empty_register.asm:21:23: error: use of empty current register "1"')

    def testVpushIntoEmptyRegister(self):
        runTestFailsToAssemble(self, 'vpush_into_empty_register.asm', './sample/asm/static_analysis_errors/vpush_into_empty_register.asm:21:11: error: use of empty current register "2"')

    def testVpopFromEmptyRegister(self):
        runTestFailsToAssemble(self, 'vpop_from_empty_register.asm', './sample/asm/static_analysis_errors/vpop_from_empty_register.asm:21:13: error: use of empty current register "1"')

    def testVatOnEmptyRegister(self):
        runTestFailsToAssemble(self, 'vat_on_empty_register.asm', './sample/asm/static_analysis_errors/vat_on_empty_register.asm:21:12: error: use of empty current register "1"')

    def testVlenOnEmptyRegister(self):
        runTestFailsToAssemble(self, 'vlen_on_empty_register.asm', './sample/asm/static_analysis_errors/vlen_on_empty_register.asm:21:13: error: use of empty current register "1"')

    def testPointerFromEmptyRegister(self):
        runTestFailsToAssemble(self, 'pointer_from_empty_register.asm', './sample/asm/static_analysis_errors/pointer_from_empty_register.asm:21:12: error: pointer from empty current register "1"')

    def testThrowFromEmptyRegister(self):
        runTestFailsToAssemble(self, 'throw_from_empty_register.asm', './sample/asm/static_analysis_errors/throw_from_empty_register.asm:21:11: error: throw from empty current register "1"')

    def testIsnullFailsOnNonemptyRegisters(self):
        runTestFailsToAssemble(self, 'isnull_fails_on_nonempty_registers.asm', './sample/asm/static_analysis_errors/isnull_fails_on_nonempty_registers.asm:22:22: error: useless check, register will always be defined')

    def testFcallFromEmptyRegister(self):
        runTestFailsToAssemble(self, 'fcall_from_empty_register.asm', './sample/asm/static_analysis_errors/fcall_from_empty_register.asm:22:15: error: call from empty current register "1"')

    def testJoinFromEmptyRegister(self):
        runTestFailsToAssemble(self, 'join_from_empty_register.asm', './sample/asm/static_analysis_errors/join_from_empty_register.asm:21:13: error: use of empty current register "1"')

    def testSendTargetFromEmptyRegister(self):
        runTestFailsToAssemble(self, 'send_target_from_empty_register.asm', './sample/asm/static_analysis_errors/send_target_from_empty_register.asm:22:10: error: send target from empty current register "1" (named "pid")')

    def testSendFromEmptyRegister(self):
        runTestFailsToAssemble(self, 'send_from_empty_register.asm', './sample/asm/static_analysis_errors/send_from_empty_register.asm:27:13: error: send from empty current register "2"')

    def testRegisterNameAlreadyTaken(self):
        runTestFailsToAssemble(self, 'register_name_already_taken.asm', './sample/asm/static_analysis_errors/register_name_already_taken.asm:22:14: error: register name already taken: named_register')

    @unittest.skip('FIXME TODO skip this test for now')
    def testRegisterUsedBeforeBeingNamed(self):
        runTestFailsToAssemble(self, 'register_defined_before_being_named.asm', './sample/asm/static_analysis_errors/register_defined_before_being_named.asm:22:12: error: register defined before being named: 1 = named_register')

    def testUselessBranchSimpleMarker(self):
        runTestFailsToAssemble(self, 'useless_branch_simple_marker.asm', './sample/asm/static_analysis_errors/useless_branch_simple_marker.asm:21:5: error: useless branch: both targets point to the same instruction')

    def testUselessBranchSimpleOffset(self):
        runTestFailsToAssemble(self, 'useless_branch_simple_offset.asm', './sample/asm/static_analysis_errors/useless_branch_simple_offset.asm:21:5: error: useless branch: both targets point to the same instruction')

    def testUselessBranchSimpleIndex(self):
        runTestFailsToAssemble(self, 'useless_branch_simple_index.asm', './sample/asm/static_analysis_errors/useless_branch_simple_index.asm:21:5: error: useless branch: both targets point to the same instruction')

    def testUselessBranchMixedIndexOffsetBackward(self):
        runTestFailsToAssemble(self, 'useless_branch_mixed_index_offset_backward.asm', "./sample/asm/static_analysis_errors/useless_branch_mixed_index_offset_backward.asm:24:5: error: useless branch: both targets point to the same instruction")

    def testUselessBranchMixedIndexOffsetForward(self):
        runTestFailsToAssemble(self, 'useless_branch_mixed_index_offset_forward.asm', "./sample/asm/static_analysis_errors/useless_branch_mixed_index_offset_forward.asm:21:5: error: useless branch: both targets point to the same instruction")

    def testUselessBranchMixedMarker(self):
        runTestFailsToAssemble(self, 'useless_branch_mixed_marker.asm', './sample/asm/static_analysis_errors/useless_branch_mixed_marker.asm:21:5: error: useless branch: both targets point to the same instruction')

    def testUselessBranchMixedMarkerIndex(self):
        runTestFailsToAssemble(self, 'useless_branch_mixed_marker_index.asm', './sample/asm/static_analysis_errors/useless_branch_mixed_marker_index.asm:21:5: error: useless branch: both targets point to the same instruction')

    def testUselessBranchMixedMarkerOffsetBackward(self):
        runTestFailsToAssemble(self, 'useless_branch_mixed_marker_offset_backward.asm', './sample/asm/static_analysis_errors/useless_branch_mixed_marker_offset_backward.asm:27:5: error: useless branch: both targets point to the same instruction')

    def testUselessBranchMixedMarkerOffsetForward(self):
        runTestFailsToAssemble(self, 'useless_branch_mixed_marker_offset_forward.asm', './sample/asm/static_analysis_errors/useless_branch_mixed_marker_offset_forward.asm:21:5: error: useless branch: both targets point to the same instruction')

    def testEmptyRegisterAccessAfterTakingBranchOffsetTrue(self):
        runTestFailsToAssembleDetailed(self, 'sa_taking_true_branch_forward_offset.asm', [
            '27:11: error: use of erased current register "1" (named "value")',
            '24:5: note: erased here:',
            '23:5: error: after taking true branch here:',
            '20:12: error: in function main/0',
        ])

    def testEmptyRegisterAccessAfterTakingBranchOffsetFalse(self):
        runTestFailsToAssembleDetailed(self, 'sa_taking_false_branch_forward_offset.asm', [
            '27:11: error: use of erased current register "1" (named "value")',
            '25:5: note: erased here:',
            '23:5: error: after taking false branch here:',
            '20:12: error: in function main/0',
        ])

    def testEmptyRegisterAccessAfterTakingBranchMarkerTrue(self):
        runTestFailsToAssembleDetailed(self, 'sa_taking_true_branch_forward_marker.asm', [
            '27:11: error: use of erased current register "1" (named "value")',
            '24:5: note: erased here:',
            '23:5: error: after taking true branch here:',
            '20:12: error: in function main/0',
        ])

    def testEmptyRegisterAccessAfterTakingBranchMarkerFalse(self):
        runTestFailsToAssembleDetailed(self, 'sa_taking_false_branch_forward_marker.asm', [
            '27:11: error: use of erased current register "1" (named "value")',
            '25:5: note: erased here:',
            '23:5: error: after taking false branch here:',
            '20:12: error: in function main/0',
        ])

    def testUseOfEmptyFirstOperandInIadd(self):
        runTestFailsToAssembleDetailed(self, 'use_of_empty_first_operand_in_iadd.asm', [
            '24:30: error: use of empty current register "1" (named "first")',
            '20:12: error: in function main/0',
        ])

    def testUseOfEmptySecondOperandInIadd(self):
        runTestFailsToAssembleDetailed(self, 'use_of_empty_second_operand_in_iadd.asm', [
            '24:37: error: use of empty current register "2" (named "second")',
            '20:12: error: in function main/0',
        ])

    def testUseOfVoidAsInputRegister(self):
        runTestFailsToAssembleDetailed(self, 'void_as_input_register.asm', [
            '26:17: error: use of void as input register:',
            '24:12: error: in function main/0',
        ])

    def testExpectedOperandFoundNewline(self):
        runTestFailsToAssembleDetailed(self, 'found_newline.asm', [
            '21:5: error: not enough operands',
            '21:5: note: when extracting operand 0',
            '20:12: error: in function main/0',
        ])

    def testMainFunctionUsesInvalidRegisterSetToReturn(self):
        runTestFailsToAssemble(self, 'main_returns_to_invalid_rs_type.asm', './sample/asm/static_analysis_errors/main_returns_to_invalid_rs_type.asm:21:5: error: main function uses invalid register set to return a value: static')


class StaticAnalysis(unittest.TestCase):
    PATH = './sample/static_analysis'

    def testIzeroCreatesInteger(self):
        runTest(self, 'izero_creates_integer.asm', '1')

    def testIzeroReportedAsUnused(self):
        runTestFailsToAssembleDetailed(self, 'izero_reported_as_unused.asm', [
            '21:11: error: unused integer in register "1"',
            '20:12: error: in function main/0',
        ])

    def testAllowComparingIntegersAndFloats(self):
        runTest(self, 'allow_comparing_integers_and_floats.asm', 'true')

    def testPreventComparingNumbersAndText(self):
        runTestFailsToAssembleDetailed(self, 'prevent_comparing_numbers_and_text.asm', [
            '28:40: error: invalid type of value contained in register',
            '28:40: note: expected number, got text',
            '26:10: note: register defined here',
            '20:12: error: in function main/0',
        ])

    def testFstoreStoresFloats(self):
        runTestFailsToAssembleDetailed(self, 'fstore_stores_floats.asm', [
            '22:10: error: invalid type of value contained in register',
            '22:10: note: expected integer, got float',
            '21:11: note: register defined here',
            '20:12: error: in function main/0',
        ])

    def testVinsertErasesDirectlyAccessedRegisters(self):
        runTestFailsToAssembleDetailed(self, 'vinsert_erases_directly_accessed_registers.asm', [
            '26:11: error: use of erased local register "1"',
            '24:5: note: erased here:',
            '20:12: error: in function main/0',
        ])

    def testDoesNotEraseDereferencedSources(self):
        runTestSplitlines(self, 'vinsert_does_not_erase_dereferenced_sources.asm', [
            '1',
            '[1]',
            'IntegerPointer',
        ])

    def testInferringTypesForArgs(self):
        runTestFailsToAssembleDetailed(self, 'inferring_types_of_args.asm', [
            '32:22: error: invalid type of value contained in register',
            '32:22: note: expected string, got integer',
            '23:9: note: register defined here',
            '25:10: note: type inferred here',
            '                 ^ deduced type is \'integer\'',
            '20:12: error: in function main/1',
        ])

    def testInferenceIncludesPointeredTypes(self):
        runTestFailsToAssembleDetailed(self, 'inference_includes_pointered_types.asm', [
            '26:19: error: invalid type of value contained in register',
            '26:19: note: expected string, got pointer to integer',
            '21:9: note: register defined here',
            '23:10: note: type inferred here',
            '                 ^ deduced type is \'pointer to integer\'',
            '20:12: error: in function main/1',
        ])

    def testPartialPointernessInference(self):
        runTestFailsToAssembleDetailed(self, 'partial_pointerness_inference.asm', [
            '47:13: error: invalid type of value contained in register',
            '47:13: note: expected vector, got pointer to value',
            '24:9: note: register defined here',
            '33:11: note: type inferred here',
            '                  ^ deduced type is \'pointer to value\'',
            '20:12: error: in function main/1',
        ])

    def testTwoStagePointernessInference(self):
        runTestFailsToAssembleDetailed(self, 'two_stage_pointerness_inference.asm', [
            '50:13: error: invalid type of value contained in register',
            '50:13: note: expected vector, got integer',
            '24:9: note: register defined here',
            '42:10: note: type inferred here',
            '                 ^ deduced type is \'pointer to integer\'',
            '20:12: error: in function main/1',
        ])

    def testClosureCapturesByMoveMakeInaccessible(self):
        runTestFailsToAssembleDetailed(self, 'closure_captures_by_move_make_inaccessible.asm', [
            '34:11: error: use of erased local register "2"',
            '29:5: note: erased here:',
            '25:12: error: in function main/0',
        ])

    def testClosureCapturesInvalidType(self):
        runTestFailsToAssembleDetailed(self, 'closure_captures_invalid_type.asm', [
            '21:10: error: invalid type of value contained in register',
            '21:10: note: expected integer, got text',
            '30:26: note: register defined here',
            '20:11: error: in a closure defined here:',
            '27:13: error: when instantiated here:',
            '26:12: error: in function main/0',
        ])

    def testNestedClosureInvalidTypeError(self):
        runTestFailsToAssembleDetailed(self, 'nested_closure_invalid_type_error.asm', [
            '25:10: error: invalid type of value contained in register',
            '25:10: note: expected integer, got text',
            '36:26: note: register defined here',
            '20:11: error: in a closure defined here:',
            '32:13: error: when instantiated here:',
            '30:11: error: in a closure defined here:',
            '46:13: error: when instantiated here:',
            '44:12: error: in function main/1',
        ])

    def testCallToInvalidType(self):
        runTestFailsToAssembleDetailed(self, 'call_to_invalid_type.asm', [
            '33:15: error: invalid type of value contained in register',
            '33:15: note: expected invocable, got text',
            '29:10: note: register defined here',
            '27:12: error: in function main/1',
        ])

    def testTailCallToInvalidType(self):
        runTestFailsToAssembleDetailed(self, 'tailcall_to_invalid_type.asm', [
            '33:14: error: invalid type of value contained in register',
            '33:14: note: expected invocable, got text',
            '29:10: note: register defined here',
            '27:12: error: in function main/1',
        ])

    def testInvalidTypeForIndirectParameterPass(self):
        runTestFailsToAssembleDetailed(self, 'invalid_type_for_indirect_parameter_pass.asm', [
            '32:10: error: invalid type of value contained in register',
            '32:10: note: expected integer, got text',
            '29:10: note: register defined here',
            '26:12: error: in function main/1',
        ])

    def testJumpSkippingADefinitionInstruction(self):
        runTestFailsToAssembleDetailed(self, 'jump_skipping_a_definition_instruction.asm', [
            '24:11: error: use of empty local register "1"',
            '20:12: error: in function main/0',
        ])

    def testInvalidAccessTypeForSwap(self):
        runTestFailsToAssembleDetailed(self, 'invalid_access_type_for_swap.asm', [
            '24:19: error: invalid access mode',
            '24:19: note: can only swap using direct access mode',
            '                          ^ did you mean \'%2\'?',
            '20:12: error: in function main/0',
        ])

    def testOverwriteOfUnused(self):
        runTestFailsToAssembleDetailed(self, 'overwrite_of_unused_value.asm', [
            '22:13: error: overwrite of unused value:',
            '21:13: note: unused value defined here:',
            '20:12: error: in function main/0',
        ])



class AssemblerErrorTests(unittest.TestCase):
    """Tests for error-checking and reporting functionality.
    """
    PATH = './sample/asm/errors'

    def testInvalidOperandForJumpInstruction(self):
        runTestFailsToAssembleDetailed(self, 'invalid_operand_for_jump_instruction.asm', [
            "21:10: error: invalid operand for jump instruction",
            "21:10: note: expected a label or an offset",
            "20:12: error: in function main/0",
        ])

    def testDotBeforeEnd(self):
        runTestFailsToAssemble(self, 'no_dot_before_end.asm', "./sample/asm/errors/no_dot_before_end.asm:23:1: error: missing '.' character before 'end'")

    def testBranchWithoutOperands(self):
        runTestFailsToAssemble(self, 'branch_without_operands.asm', "./sample/asm/errors/branch_without_operands.asm:21:5: error: branch without operands")

    def testNoEndBetweenDefs(self):
        runTestFailsToAssemble(self, 'no_end_between_defs.asm', "./sample/asm/errors/no_end_between_defs.asm:23:1: error: another function opened before assembler reached .end after 'foo/0' function")

    def testHaltAsLastInstruction(self):
        runTestFailsToAssembleDetailed(self, 'halt_as_last_instruction.asm', [
            "22:5: error: invalid last mnemonic",
            "22:5: note: expected one of: leave, return, or tailcall",
            "20:12: error: in function main/1",
        ], asm_opts=('-c',))

    def testArityError(self):
        runTestFailsToAssemble(self, 'arity_error.asm', "./sample/asm/errors/arity_error.asm:27:5: error: invalid number of parameters in call to function foo/1: expected 1, got 0")

    def testIsNotAValidFunctionName(self):
        runTestFailsToAssembleDetailed(self, 'is_not_a_valid_function_name.asm', [
            "26:10: error: not a valid function name",
            "24:12: error: in function main/0",
        ])

    def testFrameWithGaps(self):
        runTestFailsToAssembleDetailed(self, 'frame_with_gaps.asm', [
            "25:5: error: gap in frame",
            "28:5: error: slot 1 left empty at",
            "24:12: error: in function main/0",
        ])

    def testPassingParameterToASlotWithTooHighIndex(self):
        runTestFailsToAssemble(self, 'passing_to_slot_with_too_high_index.asm', "./sample/asm/errors/passing_to_slot_with_too_high_index.asm:26:5: error: pass to parameter slot 3 in frame with only 3 slots available")

    def testDoublePassing(self):
        # FIXME test for all lines of traced error
        runTestFailsToAssemble(self, 'double_pass.asm', "./sample/asm/errors/double_pass.asm:29:5: error: double pass to parameter slot 2")

    def testNotAValidFunctionNameCall(self):
        runTestFailsToAssembleDetailed(self, 'not_a_valid_function_name_call.asm', [
            "22:15: error: not a valid function name",
            "20:12: error: in function main/0",
        ])

    def testNoReturnOrTailcallAtTheEndOfAFunctionError(self):
        runTestFailsToAssembleDetailed(self, 'no_return_at_the_end_of_a_function.asm', [
            # "22:1: error: function does not end with 'return' or 'tailcall': foo/0",  # FIXME this will be correct once SA is fixed to throw more specific errors
            "21:5: error: invalid last mnemonic",
            "21:5: note: expected one of: leave, return, or tailcall",
            "20:12: error: in function foo/0",
        ])

    def testBlockWithEmptyBody(self):
        runTestFailsToAssemble(self, 'empty_block_body.asm', "./sample/asm/errors/empty_block_body.asm:20:9: error: block with empty body: foo")

    def testCallToUndefinedFunction(self):
        runTestFailsToAssemble(self, 'call_to_undefined_function.asm', "./sample/asm/errors/call_to_undefined_function.asm:22:10: error: call to undefined function foo/1")

    def testTailCallToUndefinedFunction(self):
        runTestFailsToAssemble(self, 'tail_call_to_undefined_function.asm', "./sample/asm/errors/tail_call_to_undefined_function.asm:22:14: error: tail call to undefined function foo/0")

    def testProcessFromUndefinedFunction(self):
        runTestFailsToAssemble(self, 'process_from_undefined_function.asm', "./sample/asm/errors/process_from_undefined_function.asm:22:13: error: process from undefined function foo/0")

    def testInvalidFunctionName(self):
        runTestFailsToAssemble(self, 'invalid_function_name.asm', "./sample/asm/errors/invalid_function_name.asm:30:12: error: invalid function name: foo/x")

    def testExcessFrameSpawned(self):
        runTestFailsToAssembleDetailed(self, 'excess_frame_spawned.asm', [
            "27:5: error: excess frame spawned",
            "26:5: note: unused frame:",
            "25:12: error: in function another_valid/0",
        ])

    def testLeftoverFrameTriggeredByReturn(self):
        runTestFailsToAssembleDetailed(self, 'leftover_frame_return.asm', [
            "23:5: error: lost frame at:",
            "21:5: note: spawned here:",
            "20:12: error: in function main/0",
        ])

    def testLeftoverFrameTriggeredByThrow(self):
        runTestFailsToAssembleDetailed(self, 'leftover_frame_throw.asm', [
            "22:5: error: lost frame at:",
            "21:5: note: spawned here:",
            "20:12: error: in function main/0",
        ])

    def testLeftoverFrameTriggeredByLeave(self):
        runTestFailsToAssembleDetailed(self, 'leftover_frame_leave.asm', [
            "22:5: error: lost frame at:",
            "21:5: note: spawned here:",
            "20:9: error: in block foo",
        ])

    def testLeftoverFrameTriggeredByEnd(self):
        runTestFailsToAssembleDetailed(self, 'leftover_frame_end.asm', [
            "24:5: error: lost frame at:",
            "21:5: note: spawned here:",
            "20:12: error: in function main/0",
        ])

    def testCallWithoutAFrame(self):
        runTestFailsToAssemble(self, 'call_without_a_frame.asm', "./sample/asm/errors/call_without_a_frame.asm:28:5: error: call with 'tailcall' without a frame")

    def testCatchingWithUndefinedBlock(self):
        runTestFailsToAssemble(self, 'catching_with_undefined_block.asm', "./sample/asm/errors/catching_with_undefined_block.asm:27:21: error: cannot catch using undefined block: main/0__catch")

    def testEnteringUndefinedBlock(self):
        runTestFailsToAssemble(self, 'entering_undefined_block.asm', "./sample/asm/errors/entering_undefined_block.asm:22:11: error: cannot enter undefined block: foo")

    def testFunctionFromUndefinedFunction(self):
        runTestFailsToAssemble(self, 'function_from_undefined_function.asm', "./sample/asm/errors/function_from_undefined_function.asm:21:5: error: function from undefined function: foo/0")

    def testFunctionWithEmptyBody(self):
        runTestFailsToAssemble(self, 'empty_function_body.asm', "./sample/asm/errors/empty_function_body.asm:20:12: error: function with empty body: foo/0")

    def testStrayEndMarked(self):
        runTestFailsToAssemble(self, 'stray_end.asm', "./sample/asm/errors/stray_end.asm:20:1: error: stray .end marker")

    def testIllegalDirective(self):
        runTestFailsToAssemble(self, 'illegal_directive.asm', "./sample/asm/errors/illegal_directive.asm:20:1: error: illegal directive")

    def testUnknownInstruction(self):
        runTestFailsToAssembleDetailed(self, 'unknown_instruction.asm', [
            "21:5: error: unknown instruction",
            "            ^ did you mean 'print'?",
            "20:12: error: in function main/1",
        ])

    def testMoreThanOneMainFunction(self):
        name = 'more_than_one_main_function.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.bin'.format(self.PATH[2:].replace('/', '_'), name))
        output, error, exit_code = assemble(assembly_path, compiled_path, okcodes=(1,0))
        self.assertEqual(1, exit_code)
        error_lines = [
            "./sample/asm/errors/more_than_one_main_function.asm: note: main/1 function found in module ./sample/asm/errors/more_than_one_main_function.asm",
            "./sample/asm/errors/more_than_one_main_function.asm: note: main/2 function found in module ./sample/asm/errors/more_than_one_main_function.asm",
            "./sample/asm/errors/more_than_one_main_function.asm: error: more than one candidate for main function",
        ]
        self.assertEqual(error_lines, output.strip().splitlines())

    def testMainFunctionIsNotDefined(self):
        runTestFailsToAssemble(self, 'main_function_is_not_defined.asm', "./sample/asm/errors/main_function_is_not_defined.asm: error: main function is not defined")

    def testRegisterIndexesCannotBeNegative(self):
        runTestFailsToAssemble(self, 'register_indexes_cannot_be_negative.asm', "./sample/asm/errors/register_indexes_cannot_be_negative.asm:27:11: error: register indexes cannot be negative: -1")

    def testBackwardOutOfFunctionJump(self):
        runTestFailsToAssemble(self, 'backward_out_of_function_jump.asm', "./sample/asm/errors/backward_out_of_function_jump.asm:21:10: error: backward out-of-range jump")

    def testForwardOutOfFunctionJump(self):
        runTestFailsToAssemble(self, 'forward_out_of_function_jump.asm', "./sample/asm/errors/forward_out_of_function_jump.asm:21:10: error: forward out-of-range jump")

    def testJumpToUnrecognisedMarker(self):
        runTestFailsToAssemble(self, 'jump_to_unrecognised_marker.asm', "./sample/asm/errors/jump_to_unrecognised_marker.asm:21:10: error: jump to unrecognised marker: foo")

    def testBlocksEndWithReturningInstruction(self):
        runTestFailsToAssembleDetailed(self, 'blocks_end_with_returning_instruction.asm', [
            "21:5: error: invalid last mnemonic",
            "21:5: note: expected one of: leave, return, or tailcall",
            "20:9: error: in block foo__block",
        ])

    def testBranchWithoutTarget(self):
        runTestFailsToAssemble(self, 'branch_without_a_target.asm', "./sample/asm/errors/branch_without_a_target.asm:23:5: error: branch without a target")

    def testBranchTrueBackwardOutOfRange(self):
        runTestFailsToAssemble(self, 'branch_true_backward_out_of_range.asm', "./sample/asm/errors/branch_true_backward_out_of_range.asm:23:11: error: backward out-of-range jump")

    def testBranchTrueForwardOutOfRange(self):
        runTestFailsToAssemble(self, 'branch_true_forward_out_of_range.asm', "./sample/asm/errors/branch_true_forward_out_of_range.asm:23:11: error: forward out-of-range jump")

    def testBranchFalseBackwardOutOfRange(self):
        runTestFailsToAssemble(self, 'branch_false_backward_out_of_range.asm', "./sample/asm/errors/branch_false_backward_out_of_range.asm:23:14: error: backward out-of-range jump")

    def testBranchFalseForwardOutOfRange(self):
        runTestFailsToAssemble(self, 'branch_false_forward_out_of_range.asm', "./sample/asm/errors/branch_false_forward_out_of_range.asm:23:14: error: forward out-of-range jump")

    def testBranchTrueForwardOutOfRangeNonrelative(self):
        runTestFailsToAssemble(self, 'branch_true_forward_out_of_range_nonrelative.asm', "./sample/asm/errors/branch_true_forward_out_of_range_nonrelative.asm:23:11: error: forward out-of-range jump")

    def testBranchFalseForwardOutOfRangeNonrelative(self):
        runTestFailsToAssemble(self, 'branch_false_forward_out_of_range_nonrelative.asm', "./sample/asm/errors/branch_false_forward_out_of_range_nonrelative.asm:23:14: error: forward out-of-range jump")

    def testBranchTrueToUnrecognisedMarker(self):
        runTestFailsToAssemble(self, 'branch_true_to_unrecognised_marker.asm', "./sample/asm/errors/branch_true_to_unrecognised_marker.asm:23:11: error: jump to unrecognised marker: foo")

    def testBranchFalseToUnrecognisedMarker(self):
        runTestFailsToAssemble(self, 'branch_false_to_unrecognised_marker.asm', "./sample/asm/errors/branch_false_to_unrecognised_marker.asm:23:14: error: jump to unrecognised marker: foo")

    def testZeroDistanceBackwardFalseBranch(self):
        runTestFailsToAssemble(self, 'zero_distance_backward_false_branch.asm', "./sample/asm/errors/zero_distance_backward_false_branch.asm:21:13: error: zero-distance jump")

    def testZeroDistanceBackwardJump(self):
        runTestFailsToAssemble(self, 'zero_distance_backward_jump.asm', "./sample/asm/errors/zero_distance_backward_jump.asm:21:10: error: zero-distance jump")

    def testZeroDistanceBackwardTrueBranch(self):
        runTestFailsToAssemble(self, 'zero_distance_backward_true_branch.asm', "./sample/asm/errors/zero_distance_backward_true_branch.asm:21:11: error: zero-distance jump")

    def testZeroDistanceFalseBranch(self):
        runTestFailsToAssemble(self, 'zero_distance_false_branch.asm', "./sample/asm/errors/zero_distance_false_branch.asm:24:13: error: zero-distance jump")

    def testZeroDistanceForwardFalseBranch(self):
        runTestFailsToAssemble(self, 'zero_distance_forward_false_branch.asm', "./sample/asm/errors/zero_distance_forward_false_branch.asm:21:13: error: zero-distance jump")

    def testZeroDistanceForwardJump(self):
        runTestFailsToAssemble(self, 'zero_distance_forward_jump.asm', "./sample/asm/errors/zero_distance_forward_jump.asm:21:10: error: zero-distance jump")

    def testZeroDistanceForwardTrueBranch(self):
        runTestFailsToAssemble(self, 'zero_distance_forward_true_branch.asm', "./sample/asm/errors/zero_distance_forward_true_branch.asm:21:11: error: zero-distance jump")

    def testZeroDistanceJump(self):
        runTestFailsToAssemble(self, 'zero_distance_jump.asm', "./sample/asm/errors/zero_distance_jump.asm:24:10: error: zero-distance jump")

    def testZeroDistanceMarkerFalseBranch(self):
        runTestFailsToAssemble(self, 'zero_distance_marker_false_branch.asm', "./sample/asm/errors/zero_distance_marker_false_branch.asm:25:13: error: zero-distance jump")

    def testZeroDistanceMarkerJump(self):
        runTestFailsToAssemble(self, 'zero_distance_marker_jump.asm', "./sample/asm/errors/zero_distance_marker_jump.asm:24:10: error: zero-distance jump")

    def testZeroDistanceMarkerTrueBranch(self):
        runTestFailsToAssemble(self, 'zero_distance_marker_true_branch.asm', "./sample/asm/errors/zero_distance_marker_true_branch.asm:25:11: error: zero-distance jump")

    def testZeroDistanceTrueBranch(self):
        runTestFailsToAssemble(self, 'zero_distance_true_branch.asm', "./sample/asm/errors/zero_distance_true_branch.asm:24:11: error: zero-distance jump")

    def testAtLeastTwoTokensAreRequiredInAWrappedInstruction(self):
        runTestFailsToAssemble(self, 'at_least_two_tokens_required_in_a_wrapped_instruction.asm', "./sample/asm/errors/at_least_two_tokens_required_in_a_wrapped_instruction.asm:25:28: error: at least two tokens are required in a wrapped instruction")

    def testInvalidRegisterIndexInNameDirective(self):
        runTestFailsToAssemble(self, 'invalid_register_index_in_name_directive.asm', "./sample/asm/errors/invalid_register_index_in_name_directive.asm: error: in function 'main/0': invalid register index in name directive: named_register := \"bad\"")

    def testInvalidRegisterIndexInNameDirective(self):
        runTestFailsToAssemble(self, 'empty_link_directive.asm', "./sample/asm/errors/empty_link_directive.asm:21:13: error: missing module name in import directive")

    def testReservedWordAsBlockName(self):
        runTestFailsToAssemble(self, 'reserved_word_as_block_name.asm', "./sample/asm/errors/reserved_word_as_block_name.asm:20:9: error: invalid block name: 'iota' is a registered keyword")

    def testDuplicatedFunctionNames(self):
        runTestFailsToAssembleDetailed(self, 'duplicated_function_names.asm', [
            "24:12: error: duplicated name: foo/0",
            "20:12: error: already defined here:",
        ])

    def testDuplicatedBlockAndFunctionName(self):
        runTestFailsToAssembleDetailed(self, 'duplicated_block_and_function_name.asm', [
            "24:9: error: duplicated name: foo/0",
            "20:12: error: already defined here:",
        ])

    def testInvalidRegisterIndexInName(self):
        runTestFailsToAssembleDetailed(self, 'invalid_register_index_in_name.asm', [
            '21:12: error: invalid register index: a_name := "a"',
            '                   ^       ',
        ])


class KeywordIotaTests(unittest.TestCase):
    """Tests for `iota` keyword.
    """
    PATH = './sample/asm/keyword/iota'

    def testIotaInFrame(self):
        runTestSplitlines(self, 'iota_in_frame.asm', ['Hello World!', '42'])

    def testIotaInNames(self):
        # --no-sa because otherwise SA complains about accessing named registers using
        # bare indexes
        runTestSplitlines(self, 'iota_in_names.asm', ['Hello World!', '42'], assembly_opts=('--no-sa',))

    def testIotaInRegisterIndexes(self):
        runTestSplitlines(self, 'iota_in_register_indexes.asm', ['Hello World!', '42'])

    def testIotaInReceivingArguments(self):
        runTestSplitlines(self, 'iota_in_receiving_arguments.asm', ['Hello World!', '42'])

    def testInvalidArgumentToIota(self):
        runTestFailsToAssemble(self, 'invalid_argument_to_iota.asm', "./sample/asm/keyword/iota/invalid_argument_to_iota.asm:21:12: error: invalid argument to '.iota:' directive: foo")

    def testIotaDirectiveUsedOutsideOfIotaScope(self):
        runTestFailsToAssemble(self, 'iota_directive_used_outside_of_iota_scope.asm', "./sample/asm/keyword/iota/iota_directive_used_outside_of_iota_scope.asm:20:1: error: '.iota:' directive used outside of iota scope")


class KeywordVoidTests(unittest.TestCase):
    PATH = './sample/asm/keyword/void'

    def testVoidInArg(self):
        runTest(self, 'in_arg.asm', '')

    def testVoidInCall(self):
        runTest(self, 'in_call.asm', '')

    def testVoidInJoin(self):
        runTest(self, 'in_join.asm', '')

    def testVoidInMsg(self):
        runTest(self, 'in_receive.asm', '')

    def testVoidInProcess(self):
        runTest(self, 'in_process.asm', 'Hello World!')

    def testVoidInReceive(self):
        runTest(self, 'in_receive.asm', '')

    def testVpopVoidTarget(self):
        runTest(self, 'vpop_void_target.asm', '[]')


class KeywordDefaultTests(unittest.TestCase):
    """Tests for `default` keyword.
    """
    PATH = './sample/asm/keyword/default'

    def testDefaultInArg(self):
        runTestFailsToAssembleDetailed(self, 'arg.asm', [
                '21:16: error: use of void as input register:',
                '20:12: error: in function foo/1',
       ])

    def testDefaultInCall(self):
        runTest(self, 'call.asm', '')

    def testDefaultInIstore(self):
        runTest(self, 'integer.asm', '0')

    def testDefaultInFstore(self):
        runTest(self, 'float.asm', '0.000000')

    def testDefaultInStrstore(self):
        runTest(self, 'string.asm', 'default:')


class AssemblerErrorRejectingDuplicateSymbolsTests(unittest.TestCase):
    PATH = './sample/asm/errors/single_definition_rule'

    def testRejectingDuplicateSymbolsInLinkedFiles(self):
        lib_a_path = os.path.join(self.PATH, 'libA.vlib')
        lib_b_path = os.path.join(self.PATH, 'libB.vlib')
        assemble(os.path.join(self.PATH, 'lib.asm'), out=lib_a_path, opts=('--lib',))
        assemble(os.path.join(self.PATH, 'lib.asm'), out=lib_b_path, opts=('--lib',))
        self.assertRaises(ViuaAssemblerError, assemble, os.path.join(self.PATH, 'exec.asm'), links=(lib_a_path, lib_b_path))

    def testRejectingDuplicateLinksOnCommandline(self):
        lib_a_path = os.path.join(self.PATH, 'libA.vlib')
        assemble(os.path.join(self.PATH, 'lib.asm'), out=lib_a_path, opts=('--lib',))
        self.assertRaises(ViuaAssemblerError, assemble, os.path.join(self.PATH, 'exec.asm'), links=(lib_a_path, lib_a_path))


class ExceptionMechanismTests(unittest.TestCase):
    PATH = './sample/asm/exception_mechanism'

    def testThrowFromEmptyRegister(self):
        runTestThrowsException(self, 'throw_from_empty_register.asm', ('Exception', 'throw from null register',), assembly_opts=('--no-sa',))

class MiscExceptionTests(unittest.TestCase):
    PATH = './sample/asm/exceptions'

    @unittest.skip('watchdog does not play nice with new scheduling model')
    def testTerminatingProcessDoesNotBreakOtherProcesses(self):
        expected_output = [
            'Hello World from process 5',
            'Hello World from process 1',
            'Hello World from process 4',
            'Hello World from process 6',
            'Hello World from process 2',
            'uncaught object: Integer = 42',
            'Hello World from process 3',
            'Hello World from process 0',
        ]
        # FIXME: SA needs basic support for static register set
        runTest(self, 'terminating_processes.asm', sorted(expected_output), output_processing_function=lambda _: sorted(filter(lambda _: (_.startswith('Hello') or _.startswith('uncaught')), _.strip().splitlines())), assembly_opts=('--no-sa',))

    def testCatchingMachineThrownException(self):
        # pass --no-sa flag; we want to check runtime exception
        runTest(self, 'nullregister_access.asm', "exception encountered: read from null register: 1", assembly_opts=('--no-sa',))

    def testCatcherState(self):
        runTestSplitlines(self, 'restore_catcher_state.asm', ['42','100','42','100'], test_disasm=False)

    def testCatchingExceptionThrownInDifferentModule(self):
        source_lib = 'thrown_in_linked_caught_in_static_fun.asm'
        lib_path = 'test_module.vlib'
        assemble(os.path.join(self.PATH, source_lib), out=lib_path, opts=('--lib',))
        runTest(self, 'thrown_in_linked_caught_in_static_base.asm', 'looks falsey: 0')

    def testVectorOutOfRangeRead(self):
        runTestThrowsException(self, 'vector_out_of_range_read.asm', ('OutOfRangeException', 'positive vector index out of range',))

    def testVectorOutOfRangeReadFromEmpty(self):
        runTestThrowsException(self, 'vector_out_of_range_read_from_empty.asm', ('OutOfRangeException', 'empty vector index out of range',))

    def testDeleteOfEmptyRegister(self):
        runTestThrowsException(self, 'delete_of_empty_register.asm', ('Exception', 'delete of null register',), assembly_opts=('--no-sa',))


class MiscTests(unittest.TestCase):
    PATH = './sample/asm/misc'

    def testMain0AsMainFunction(self):
        runTestSplitlines(self, name='main0_as_main_function.asm', expected_output=['Hello World!', 'received 0 arguments'])

    def testMain2AsMainFunction(self):
        runTestSplitlines(self, name='main2_as_main_function.asm', expected_output=['Hello World!', 'received 2 arguments'])

    def testBrokenWatchdog(self):
        runTest(self, 'broken_watchdog.asm', 'main/1 exiting')

    def testMetaInformationEncoding(self):
        pth = 'meta_information.asm'
        output, error, exit_code = assemble(os.path.join(self.PATH, pth), out='/tmp/mi.bin')
        output, error, exit_code = disassemble('/tmp/mi.bin', out='/tmp/mi.asm')
        output, error, exit_code = assemble('/tmp/mi.asm', out='/tmp/mi.bin', opts=('--meta',))
        expected_output = [
            'key = "value"',
            'foo = "multiline\\nbar"',
        ]
        self.assertEqual(sorted(expected_output), sorted(output.strip().splitlines()))

    def testMangledNestedBlockNames(self):
        runTest(self, 'mangled_nested_block_names.asm', '')


class ExternalModulesTests(unittest.TestCase):
    """Tests for C/C++ module importing, and calling external functions.
    """
    PATH = './sample/asm/external'

    def testHelloWorldExample(self):
        global MEMORY_LEAK_CHECKS_EXTRA_ALLOWED_LEAK_VALUES
        # FIXME: Valgrind freaks out about dlopen() leaks, comment this line if you know what to do about it
        # or maybe the leak originates in Viua code but I haven't found the leak
        MEMORY_LEAK_CHECKS_EXTRA_ALLOWED_LEAK_VALUES = (72736,)
        runTest(self, 'hello_world.asm', "Hello World!")

    def testReturningAValue(self):
        global MEMORY_LEAK_CHECKS_EXTRA_ALLOWED_LEAK_VALUES
        # FIXME: Valgrind freaks out about dlopen() leaks, comment this line if you know what to do about it
        # or maybe the leak originates in Viua code but I haven't found the leak
        MEMORY_LEAK_CHECKS_EXTRA_ALLOWED_LEAK_VALUES = (72736,)
        runTest(self, 'sqrt.asm', 1.73, 0, lambda o: round(float(o.strip()), 2))

    def testThrowingExceptionHandledByWatchdog(self):
        global MEMORY_LEAK_CHECKS_EXTRA_ALLOWED_LEAK_VALUES
        # FIXME: Valgrind freaks out about dlopen() leaks, comment this line if you know what to do about it
        # or maybe the leak originates in Viua code but I haven't found the leak
        MEMORY_LEAK_CHECKS_EXTRA_ALLOWED_LEAK_VALUES = (72736,)
        runTest(self, 'throwing.asm', 'OH NOES!', 0)

    def testManyHelloWorld(self):
        # expected output must be sorted because it is not defined in what order the messages will be printed if
        # there is more than one FFI or VP scheduler running
        expected_output = sorted([
            'Hello Joe!',
            'Hello Robert!',
            'Hello Mike!',
            'Hello Bjarne!',
            'Hello Guido!',
            'Hello Dennis!',
            'Hello Bram!',
            'Hello Herb!',
            'Hello Anthony!',
            'Hello Alan!',
            'Hello Ada!',
            'Hello Charles!',
        ])
        runTest(self, 'many_hello_world.asm', expected_output, 0, output_processing_function=lambda _: sorted(_.strip().splitlines()))

    def testLongRunningFunctionBlocksOneScheduler(self):
        # expected output must be sorted because it is not defined in what order the messages will be printed if
        # there is more than one FFI or VP scheduler running
        expected_output = sorted([
            'sleeper::lazy_print/0: sleep for 5ms',
            'Hello Joe!',
            'Hello Robert!',
            'Hello Mike!',
            'Hello Bjarne!',
            'Hello Guido!',
            'Hello Dennis!',
            'Hello Bram!',
            'Hello Herb!',
            'Hello Anthony!',
            'Hello Alan!',
            'Hello Ada!',
            'Hello Charles!',
            'sleeper::lazy_print/0: sleep for 15ms',
            'sleeper::lazy_print/0: sleep for 25ms',
            'sleeper::lazy_print/0: sleep for 10ms',
            'sleeper::lazy_print/0: sleep for 100ms',
            'sleeper::lazy_print/0: done',
        ])
        runTest(self, 'sleeper.asm', expected_output, 0, output_processing_function=lambda _: sorted(_.strip().splitlines()))


class ProcessAbstractionTests(unittest.TestCase):
    PATH = './sample/asm/process_abstraction'

    def testProcessesHaveSeparateGlobalRegisterSets(self):
        # FIXME global registers should not be statically checked
        runTestReportsException(self, 'separate_global_rs.asm', ('Exception', 'read from null register: 1',), assembly_opts=('--no-sa',))


class ConcurrencyTests(unittest.TestCase):
    PATH = './sample/asm/concurrency'

    def testImmediatelyDetachingProcess(self):
        runTest(self, 'immediately_detached.asm', 'Hello World (from detached)!')

    def testHelloWorldExample(self):
        runTestReturnsUnorderedLines(self, 'hello_world.asm', ['Hello concurrent World! (2)', 'Hello concurrent World! (1)'], 0)

    def testJoiningProcess(self):
        runTestSplitlines(self, 'joining_a_process.asm', ['Hello concurrent World! (1)', 'Hello concurrent World! (2)'], 0)

    def testJoiningJoinedProcess(self):
        runTestThrowsException(self, 'joining_joined_process.asm', ('Exception', 'process cannot be joined',))

    def testJoiningDetachedProcess(self):
        runTestThrowsException(self, 'joining_detached_process.asm', ('Exception', 'process cannot be joined',))

    def testDetachingProcess(self):
        runTestReturnsUnorderedLines(
            self,
            'detaching_a_process.asm',
            [
                'main/1 exited',
                'Hello World! (from long-running detached process) 0',
                'Hello World! (from long-running detached process) 1',
                'Hello World! (from long-running detached process) 2',
                'Hello World! (from long-running detached process) 3',
            ],
            0
        )

    def testMessagePassing(self):
        runTest(self, 'message_passing.asm', 'Hello message passing World!')

    def testTransferringExceptionsOnJoin(self):
        def match_output(self, excode, output):
            pat = re.compile(r'^exception transferred from process Process: 0x[a-f0-9]+: Hello exception transferring World!$')
            wat = re.match(pat, output)
            self.assertTrue(wat is not None)
            self.assertEqual(0, excode)
        runTest(self, 'transferring_exceptions.asm', custom_assert = match_output)

    def testReturningValuesOnJoin(self):
        runTest(self, 'return_from_a_process.asm', '42')

    @unittest.skip('triggers a memory leak from a path that only allocates stack memory...?')
    def testProcessFromDynamicallyLinkedFunction(self):
        source_lib = 'process_from_linked_fun.asm'
        lib_path = 'test_module.vlib'
        assemble(os.path.join(self.PATH, source_lib), out=lib_path, opts=('--lib',))
        runTest(self, 'process_from_linked_base.asm', 'Hello World!')

    def testMigratingProcessesBetweenSchedulers(self):
        # sorted because the order is semi-random due to OS scheduling of
        # VP scheduler threads
        expected_output = ['Hello {}!'.format(i) for i in range(1, 65)]
        runTestReturnsUnorderedLines(self, 'migrating_processes_between_schedulers.asm', expected_output)

    def testObtainingSelfPid(self):
        runTest(self, 'obtaining_self_pid.asm', 'Hello World (from self)!')

    def testReceiveTimeoutZeroMilliseconds(self):
        runTestThrowsException(self, 'receive_timeout_zero_milliseconds.asm', ('Exception', 'no message received',))

    def testReceiveTimeout500ms(self):
        runTestThrowsException(self, 'receive_timeout_500ms.asm', ('Exception', 'no message received',))

    def testReceiveTimeout1s(self):
        runTestThrowsException(self, 'receive_timeout_1s.asm', ('Exception', 'no message received',))

    def testReceiveTimeoutInfinite(self):
        runTest(self, 'receive_timeout_infinite.asm', 'Hello World!')

    def testReceiveTimeoutDefault(self):
        runTest(self, 'receive_timeout_default.asm', 'Hello World!')

    def testReceiveTimeoutFailsToAssemble(self):
        runTestFailsToAssemble(self, 'receive_invalid_timeout.asm', './sample/asm/concurrency/receive_invalid_timeout.asm:21:18: error: invalid operand')

    def testJoinDefaultTimeout(self):
        runTest(self, 'join_timeout_default.asm', 'child process done')

    def testJoinInfiniteTimeout(self):
        runTest(self, 'join_timeout_infinite.asm', 'child process done')

    def testJoinDefaultKeywordTimeout(self):
        runTest(self, 'join_timeout_default_keyword.asm', 'child process done')

    def testJoinTimeout10ms(self):
        runTestThrowsException(self, 'join_timeout_10ms.asm', ('Exception', 'process did not join',))

    def testJoinTimeout0ms(self):
        runTestThrowsException(self, 'join_timeout_0ms.asm', ('Exception', 'process did not join',))


class WatchdogTests(unittest.TestCase):
    PATH = './sample/asm/watchdog'

    def testHelloWorldExample(self):
        runTest(self, 'hello_world.asm', 'process spawned with <Function: broken_process/0> died')

    def testWatchdogFromUndefinedFunctionCaughtByAssembler(self):
        runTestFailsToAssemble(self, 'from_undefined_function.asm', './sample/asm/watchdog/from_undefined_function.asm:59:14: error: watchdog from undefined function undefined_function/0')

    def testWatchdogFromUndefinedFunctionCaughtAtRuntime(self):
        runTestThrowsException(self, 'from_undefined_function_at_runtime.asm', ('Exception', 'watchdog process from undefined function: undefined_function/0',))

    def testWatchdogAlreadySpawnedCaughtAtRuntime(self):
        runTest(self, 'already_spawned.asm', 'process spawned with <Function: __entry> killed by >>>watchdog already set<<<')

    def testWatchdogMustBeANativeFunction(self):
        runTestThrowsException(self, 'must_be_a_native_function.asm', ('Exception', 'watchdog process must be a native function, used foreign World::print_hello/0',))

    @unittest.skip('if watchdog dies, process enters infinite loop')
    def testWatchdogTerminatedByARunawayExceptionDoesNotLeak(self):
        runTest(self, 'terminated_watchdog.asm', 'watchdog process terminated by: Function: \'Function: broken_process/0\'')

    def testServicingRunawayExceptionWhileOtherProcessesAreRunning(self):
        runTestReturnsUnorderedLines(self, 'death_message.asm', [
            "Hello World (from detached process)!",
            "Hello World (from joined process)!",
            "process [ joined ]: 'a_joined_concurrent_process' exiting",
            "[WARNING] process 'Function: __entry' killed by >>>OH NOES!<<<",
            "Hello World (from detached process) after a runaway exception!",
            "process [detached]: 'a_detached_concurrent_process' exiting",
        ])

    def testRestartingProcessesAfterAbortedByRunawayException(self):
        runTestReturnsUnorderedLines(self, 'restarting_process.asm', [
            "process [  main  ]: 'main' exiting",
            "Hello World (from detached process)!",
            "[WARNING] process 'Function: a_division_executing_process/2[42, 0]' killed by >>>cannot divide by zero<<<",
            "42 / 1 = 42",
            "Hello World (from detached process) after a runaway exception!",
            "process [detached]: 'a_detached_concurrent_process' exiting",
        ])


class StructTests(unittest.TestCase):
    PATH = './sample/asm/structs'

    def testCreatingEmptyStruct(self):
        runTest(self, 'creating_empty_struct.asm', '{}')

    def testInsertingAValueIntoAStruct(self):
        runTest(self, 'inserting_a_value_into_a_struct.asm', "{'answer': 42}")

    def testRemovingAValueFromAStruct(self):
        runTestSplitlines(self, 'removing_a_value_from_a_struct.asm', ["{'answer': 42}", '{}'])

    def testOverwritingAValueInAStruct(self):
        runTestSplitlines(self, 'overwriting_a_value_in_a_struct.asm', ["{'answer': 666}", "{'answer': 42}"])

    def testObrainingListOfKeysInAStruct(self):
        runTest(self, 'obtaining_list_of_keys_in_a_struct.asm', "['answer', 'foo']")

    def testStructOfStructs(self):
        runTest(self, 'struct_of_structs.asm', "{'bad': {'answer': 666}, 'good': {'answer': 42}}")


class AtomTests(unittest.TestCase):
    PATH = './sample/asm/atoms'

    def testPrintingAnAtom(self):
        runTest(self, 'printing_an_atom.asm', "'an_atom'")

    def testComparingAtoms(self):
        runTestSplitlines(self, 'comparing_atoms.asm', ['true', 'false'])

    def testComparingWithDifferentType(self):
        # This was before the "new SA".
        # runTestThrowsException(self, 'comparing_with_different_type.asm', ('Exception', "fetched invalid type: expected 'viua::types::Atom' but got 'Integer'"))
        runTestFailsToAssembleDetailed(self, 'comparing_with_different_type.asm', [
            '24:40: error: invalid type of value contained in register',
            '24:40: note: expected atom, got integer',
            '22:21: note: register defined here',
            '20:12: error: in function main/0',
        ])


class DeferredCallsTests(unittest.TestCase):
    PATH = './sample/asm/deferred'

    def testDeferredHelloWorld(self):
        runTest(self, 'hello_world.asm', "Hello World!")

    def testDeferredCallsInvokedInReverseOrder(self):
        runTestSplitlines(self, 'reverse_order.asm', ['bar', 'foo'])

    def testNestedDeferredCalls(self):
        runTestSplitlines(self, 'nested.asm', ['bar', 'baz', 'bay', 'foo'])

    def testDeferredCallsActivatedOnTailCall(self):
        runTestSplitlines(self, 'tailcall.asm', ['Hello from deferred!', '42'])

    def testDeferredCallsInvokedBeforeTailCall(self):
        runTestSplitlines(self, 'before_tailcall.asm', ['Hello World!'])

    def testDeferredCallsInvokedBeforeFrameIsPopped(self):
        runTestSplitlines(self, 'before_return.asm', ['Hello World!'])

    def testDeferredCallsActivatedOnStackUnwindingWhenExceptionUncaught(self):
        runTestSplitlines(
            self,
            name = 'on_uncaught_exception.asm',
            expected_output = ['Hello deferred Foo!', 'Hello deferred Bar!'],
            expected_error = ('Integer', '42',),
            error_processing_function = extractFirstException,
            expected_exit_code = 1,
        )

    def testDeferredCallsActivatedOnStackUnwindingWhenExceptionCaught(self):
        runTestSplitlines(self, 'on_caught_exception.asm', ['Hello bar World!', 'Hello foo World!', '42'])

    def testDeferredCallsAreInvokedBeforeStackIsUnwoundOnCaughtException(self):
        runTestSplitlines(self,
            name = 'before_unwind_on_caught.asm',
            expected_output = [ 'Hello World before stack unwinding!', '666', ],
        )

    def testDeferredCallsAreInvokedBeforeStackIsUnwoundOnUncaughtException(self):
        runTestSplitlines(self,
            name = 'before_unwind_on_uncaught.asm',
            expected_output = [ 'Hello World before stack unwinding!', ],
            expected_error = ('Integer', '666',),
            error_processing_function = extractFirstException,
            expected_exit_code = 1,
        )

    def testDeferredRunningBeforeFrameIsDropped(self):
        runTest(self, 'calls_running_before_frame_is_dropped.asm', 'Hello World!')

    def testDeepUncaught(self):
        runTestSplitlines(
            self,
            name = 'deep_uncaught.asm',
            expected_output = [
                'Hello from by_quux/0',
                'Hello from by_bax/0',
                'Hello from by_bay/0',
                'Hello from by_baz/0',
                'Hello from by_bar/0',
                'Hello from by_foo/0',
                'Hello from by_main/0',
            ],
            expected_error = ('Integer', '666',),
            error_processing_function = extractFirstException,
            expected_exit_code = 1,
        )

    def testDeepCaught(self):
        runTestSplitlines(
            self,
            name = 'deep_caught.asm',
            expected_output = [
                'Hello from by_quux/0',
                'Hello from by_bax/0',
                'Hello from by_bay/0',
                'Hello from by_baz/0',
                'Hello from by_bar/0',
                'Hello from by_foo/0',
                '666',
                'Hello from by_main/0',
            ],
        )


class StandardRuntimeLibraryModuleVector(unittest.TestCase):
    PATH = './sample/standard_library/vector'

    def testVectorOfInts(self):
        runTest(self, 'of_ints.asm', '[0, 1, 2, 3, 4, 5, 6, 7]')

    def testVectorOf(self):
        runTest(self, 'of.asm', '[0, 1, 2, 3, 4, 5, 6, 7]')

    def testVectorReverse(self):
        runTest(self, 'reverse.asm', ['[0, 1, 2, 3, 4, 5, 6, 7]', '[7, 6, 5, 4, 3, 2, 1, 0]'], output_processing_function=lambda o: o.strip().splitlines())

    def testVectorReverseInPlace(self):
        runTest(self, 'reverse_in_place.asm', ['[0, 1, 2, 3, 4, 5, 6, 7]', '[7, 6, 5, 4, 3, 2, 1, 0]'], output_processing_function=lambda o: o.strip().splitlines())

    def testVectorEveryReturnsTrue(self):
        runTest(self, 'every_returns_true.asm', 'true')

    def testVectorEveryReturnsFalse(self):
        runTest(self, 'every_returns_false.asm', 'false')

    def testVectorAnyReturnsTrue(self):
        runTest(self, 'any_returns_true.asm', 'true')

    def testVectorAnyReturnsFalse(self):
        runTest(self, 'any_returns_false.asm', 'false')


class StandardRuntimeLibraryModuleFunctional(unittest.TestCase):
    PATH = './sample/standard_library/functional'

    def testApplyHelloWorldHelloJoeHelloMike(self):
        runTestSplitlines(self, 'hello_guys.asm', ['Hello World!', 'Hello Joe!', 'Hello Mike!'])

    def testApplyThatReturnsAValue(self):
        runTest(self, 'apply_simple.asm', '42')


class TypePointerTests(unittest.TestCase):
    PATH = './sample/types/Pointer'

    def testCheckingIfIsExpired(self):
        runTest(self, 'check_if_is_expired.asm', 'expired: false\nexpired: true')

    def testExpiredPointerType(self):
        global MEMORY_LEAK_CHECKS_EXTRA_ALLOWED_LEAK_VALUES
        # FIXME: Valgrind freaks out about dlopen() leaks, comment this line if you know what to do about it
        MEMORY_LEAK_CHECKS_EXTRA_ALLOWED_LEAK_VALUES = (72736, 74399, 74375)
        runTest(self, 'type_of_expired.asm', 'ExpiredPointer')
        MEMORY_LEAK_CHECKS_EXTRA_ALLOWED_LEAK_VALUES = ()


class ExplicitRegisterSetsTests(unittest.TestCase):
    PATH = './sample/asm/explicit_register_sets'

    def testHelloWorld(self):
        runTestSplitlines(self, 'hello_world.asm', [
            'Hello local World!',
            'Hello static World!',
            'Hello global World!',
        ])

    def testMoveBetween(self):
        runTestSplitlines(self, 'move_between.asm', [
            'Hello World!',
            'Hello World!',
            'Hello World!',
        ])



if __name__ == '__main__':
    if getCPUArchitecture() == 'aarch64':
        # we're running on ARM 64 and
        # Valgrind acts funny (i.e. showing leaks from itself)
        MEMORY_LEAK_CHECKS_ALLOWED_LEAK_VALUES += (74351, 74343)
    successful = unittest.main(exit=False).result.wasSuccessful()
    print('average run time for test: {}'.format(sum(measured_run_times, datetime.timedelta()) / max(1, len(measured_run_times))))
    print('summed run time for test: {}'.format(sum(measured_run_times, datetime.timedelta())))
    if not successful:
        exit(1)
    if MEMORY_LEAK_CHECKS_ENABLE:
        print('note: {0} memory leak check{2} ({1} check{3} skipped)'.format(MEMORY_LEAK_CHECKS_RUN, MEMORY_LEAK_CHECKS_SKIPPED, ('' if MEMORY_LEAK_CHECKS_RUN == 1 else 's'), ('' if MEMORY_LEAK_CHECKS_SKIPPED == 1 else 's')))
        print('note: maximum heap usage: {} bytes, {} allocs, {} frees; test: {}'.format(MEMORY_LEAK_REPORT_MAX_HEAP_USAGE['bytes'], MEMORY_LEAK_REPORT_MAX_HEAP_USAGE['allocs'], MEMORY_LEAK_REPORT_MAX_HEAP_USAGE['frees'], MEMORY_LEAK_REPORT_MAX_HEAP_USAGE['test']))
    else:
        print('memory leak checks disabled for this run')
