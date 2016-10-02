#!/usr/bin/env python3

#
#   Copyright (C) 2015, 2016 Marek Marecki
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

import functools
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

def run(path, expected_exit_code=0):
    """Run given file with Viua CPU and return its output.
    """
    p = subprocess.Popen((VIUA_KERNEL_PATH, path), stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    output, error = p.communicate()
    exit_code = p.wait()
    if exit_code not in (expected_exit_code if type(expected_exit_code) in [list, tuple] else (expected_exit_code,)):
        raise ViuaCPUError('{0} [{1}]: {2}'.format(path, exit_code, output.decode('utf-8').strip()))
    return (exit_code, output.decode('utf-8'))

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
    interesting_lines = [line[len(valprefix):].strip() for line in output_lines[output_lines.index('{0} HEAP SUMMARY:'.format(valprefix)):]]

    total_heap_usage_matched = valgrind_regex_heap_summary_total_heap_usage.search(interesting_lines[2])
    total_heap_usage = {
        'allocs': int(total_heap_usage_matched.group(1).replace(',', '')),
        'frees': int(total_heap_usage_matched.group(2).replace(',', '')),
        'bytes': int(total_heap_usage_matched.group(3).replace(',', '')),
    }

    summary = {'heap': {}, 'leak': {}}
    summary['heap']['in_use_at_exit'] = valgrindBytesInBlocks(interesting_lines[1], valgrind_regex_heap_summary_in_use_at_exit)
    summary['heap']['total_heap_usage'] = total_heap_usage
    summary['leak']['definitely_lost'] = valgrindBytesInBlocks(interesting_lines[5], valgrind_regex_leak_summary_definitely_lost)
    summary['leak']['indirectly_lost'] = valgrindBytesInBlocks(interesting_lines[6], valgrind_regex_leak_summary_indirectly_lost)
    summary['leak']['possibly_lost'] = valgrindBytesInBlocks(interesting_lines[7], valgrind_regex_leak_summary_possibly_lost)
    summary['leak']['still_reachable'] = valgrindBytesInBlocks(interesting_lines[8], valgrind_regex_leak_summary_still_reachable)
    summary['leak']['suppressed'] = valgrindBytesInBlocks(interesting_lines[9], valgrind_regex_leak_summary_suppressed)
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

def runTest(self, name, expected_output=None, expected_exit_code = 0, output_processing_function = None, check_memory_leaks = True, custom_assert=None, assembly_opts=None, valgrind_enable=True):
    if assembly_opts is None:
        assembly_opts = ()
    if expected_output is None and custom_assert is None:
        raise TypeError('`expected_output` and `custom_assert` cannot be both None')
    assembly_path = os.path.join(self.PATH, name)
    compiled_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.bin'.format(self.PATH[2:].replace('/', '_'), name))
    if assembly_opts is None:
        assemble(assembly_path, compiled_path)
    else:
        assemble(assembly_path, compiled_path, opts=assembly_opts)
    if not FLAG_TEST_ONLY_ASSEMBLING:
        excode, output = run(compiled_path, expected_exit_code)
        got_output = (output.strip() if output_processing_function is None else output_processing_function(output))
        if custom_assert is not None:
            custom_assert(self, excode, got_output)
        else:
            self.assertEqual(expected_output, got_output)
            self.assertEqual(expected_exit_code, excode)

        if valgrind_enable:
            runMemoryLeakCheck(self, compiled_path, check_memory_leaks)

    disasm_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.dis.asm'.format(self.PATH[2:].replace('/', '_'), name))
    compiled_disasm_path = '{0}.bin'.format(disasm_path)
    disassemble(compiled_path, disasm_path)
    if assembly_opts is None:
        assemble(disasm_path, compiled_disasm_path)
    else:
        assemble(disasm_path, compiled_disasm_path, opts=assembly_opts)
    if not FLAG_TEST_ONLY_ASSEMBLING:
        dis_excode, dis_output = run(compiled_disasm_path, expected_exit_code)
        if custom_assert is not None:
            custom_assert(self, dis_excode, (dis_output.strip() if output_processing_function is None else output_processing_function(dis_output)))
        else:
            self.assertEqual(got_output, (dis_output.strip() if output_processing_function is None else output_processing_function(dis_output)))
            self.assertEqual(excode, dis_excode)

def runTestCustomAsserts(self, name, assertions_callback, check_memory_leaks = True):
    assembly_path = os.path.join(self.PATH, name)
    compiled_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.bin'.format(self.PATH[2:].replace('/', '_'), name))
    assemble(assembly_path, compiled_path)
    if not FLAG_TEST_ONLY_ASSEMBLING:
        excode, output = run(compiled_path)
        assertions_callback(self, excode, output)
        runMemoryLeakCheck(self, compiled_path, check_memory_leaks)

def runTestSplitlines(self, name, expected_output, expected_exit_code = 0, assembly_opts=None):
    runTest(self, name, expected_output, expected_exit_code, output_processing_function = lambda o: o.strip().splitlines(), assembly_opts=assembly_opts)

def runTestReturnsUnorderedLines(self, name, expected_output, expected_exit_code = 0):
    runTest(self, name, sorted(expected_output), expected_exit_code, output_processing_function = lambda o: sorted(o.strip().splitlines()))

def runTestReturnsIntegers(self, name, expected_output, expected_exit_code = 0, check_memory_leaks=True, assembly_opts=None):
    runTest(self, name, expected_output, expected_exit_code, output_processing_function = lambda o: [int(i) for i in o.strip().splitlines()], check_memory_leaks=check_memory_leaks, assembly_opts=assembly_opts)

def extractExceptionsThrown(output):
    uncaught_object_prefix = 'uncaught object: '
    return list(map(lambda _: (_[0].strip(), _[1].strip(),),
                    map(lambda _: _.split(' = ', 1),
                        map(lambda _: _[len(uncaught_object_prefix):],
                            filter(lambda _: _.startswith(uncaught_object_prefix),
                                output.strip().splitlines())))))

def extractFirstException(output):
    return extractExceptionsThrown(output)[0]

def runTestThrowsException(self, name, expected_output, assembly_opts=None):
    runTest(self, name, expected_output, expected_exit_code=1, output_processing_function=extractFirstException, valgrind_enable=False, assembly_opts=assembly_opts)

def runTestReportsException(self, name, expected_output, assembly_opts=None):
    runTest(self, name, expected_output, expected_exit_code=0, output_processing_function=extractFirstException, assembly_opts=assembly_opts)

def runTestFailsToAssemble(self, name, expected_output, asm_opts=()):
    assembly_path = os.path.join(self.PATH, name)
    compiled_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.bin'.format(self.PATH[2:].replace('/', '_'), name))
    output, error, exit_code = assemble(assembly_path, compiled_path, okcodes=(0, 1), opts=asm_opts)
    self.assertEqual(1, exit_code)
    self.assertEqual(output.strip().splitlines()[0], expected_output)


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
        runTest(self, 'add_with_rreferences.asm', '0', 0)

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


class ByteInstructionsTests(unittest.TestCase):
    """Tests for byte instructions.
    """
    PATH = './sample/asm/byte'

    def testHelloWorld(self):
        runTest(self, 'helloworld.asm', 'Hello World!', 0)


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


class VectorInstructionsTests(unittest.TestCase):
    """Tests for vector-related instructions.

    VEC instruction does not get its own test, but is used in every other vector test
    so it gets pretty good coverage.
    """
    PATH = './sample/asm/vector'

    def testPackingVec(self):
        runTest(self, 'vec_packing.asm', '["answer to life", 42]', 0, lambda o: o.strip())

    def testPackingVecRefusesToPackItself(self):
        # pass --no-sa because we want to test runtime exception
        runTestThrowsException(self, 'vec_packing_self_pack.asm', ('Exception', 'vec would pack itself',), assembly_opts=('--no-sa',))

    def testPackingVecRefusesToOutOfRegisterSetRange(self):
        # pass --no-sa because we want to test runtime exception
        runTestThrowsException(self, 'vec_packing_out_of_range.asm', ('Exception', 'vec: packing outside of register set range',), assembly_opts=('--no-sa',))

    def testPackingVecRefusesToPackNullRegister(self):
        # pass --no-sa because we want to test runtime exception
        runTestThrowsException(self, 'vec_packing_null.asm', ('Exception', 'vec: cannot pack null register',), assembly_opts=('--no-sa',))

    def testVLEN(self):
        runTest(self, 'vlen.asm', '8', 0)

    def testVINSERT(self):
        runTest(self, 'vinsert.asm', ['Hurr', 'durr', 'Im\'a', 'sheep!'], 0, lambda o: o.strip().splitlines())

    def testInsertingOutOfRangeWithPositiveIndex(self):
        runTestThrowsException(self, 'out_of_range_index_positive.asm', ('OutOfRangeException', 'positive vector index out of range',))

    def testVPUSH(self):
        runTest(self, 'vpush.asm', ['0', '1', 'Hello World!'], 0, lambda o: o.strip().splitlines())

    def testVPOP(self):
        runTest(self, 'vpop.asm', ['0', '1', '0', 'Hello World!'], 0, lambda o: o.strip().splitlines())

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

    def testFetchingFromEmptyTemporaryRegister(self):
        runTestThrowsException(self, 'fetching_from_empty_tmp_register.asm', ('Exception', 'temporary register set is empty',))


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
        runTest(self, 'iterfib.asm', 1134903170, 0, lambda o: int(o.strip()))


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
        runTestReturnsIntegers(self, 'local_registers.asm', [42, 69])

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
        runTestReturnsIntegers(self, 'static_registers.asm', [i for i in range(0, 10)])

    def testCallWithPassByMove(self):
        runTest(self, 'pass_by_move.asm', None, custom_assert=partiallyAppliedSameLines(3))

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

    def testEnclosedVariableLeftInScope(self):
        # FIXME: passing custom assembler options will not be needed once .closure: support is completely implemented
        runTestSplitlines(self, 'enclosed_variable_left_in_scope.asm', ['Hello World!', '42'], assembly_opts=('--no-sa',))

    def testChangeEnclosedVariableFromClosure(self):
        # FIXME: passing custom assembler options will not be needed once .closure: support is completely implemented
        runTestSplitlines(self, 'change_enclosed_variable_from_closure.asm', ['Hello World!', '42'], assembly_opts=('--no-sa',))

    def testNestedClosures(self):
        # FIXME: passing custom assembler options will not be needed once .closure: support is completely implemented
        runTest(self, 'nested_closures.asm', '10', assembly_opts=('--no-sa',))

    def testSimpleEncloseByCopy(self):
        # FIXME: passing custom assembler options will not be needed once .closure: support is completely implemented
        runTest(self, 'simple_enclose_by_copy.asm', '42', assembly_opts=('--no-sa',))

    def testEncloseCopyCreatesIndependentObjects(self):
        # FIXME: passing custom assembler options will not be needed once .closure: support is completely implemented
        runTestSplitlines(self, 'enclosecopy_creates_independent_objects.asm', ['Hello World!', 'Hello World!', '42', 'Hello World!'], assembly_opts=('--no-sa',))

    def testSimpleEncloseByMove(self):
        # FIXME: passing custom assembler options will not be needed once .closure: support is completely implemented
        runTestSplitlines(self, 'simple_enclose_by_move.asm', ['true', 'Hello World!'], assembly_opts=('--no-sa',))


class InvalidInstructionOperandTypeTests(unittest.TestCase):
    """Tests checking detection of invalid operand type during instruction execution.
    """
    PATH = './sample/asm/invalid_operand_types'

    def testIADD(self):
        runTestThrowsException(self, 'iadd.asm', ('Exception', 'invalid operand types: expected (_, Integer, Integer), got (_, Integer, Float)',))

    def testISUB(self):
        runTestThrowsException(self, 'isub.asm', ('Exception', 'invalid operand types: expected (_, Integer, Integer), got (_, Integer, String)',))

    def testIMUL(self):
        runTestThrowsException(self, 'imul.asm', ('Exception', 'invalid operand types: expected (_, Integer, Integer), got (_, Float, String)',))

    def testIDIV(self):
        runTestThrowsException(self, 'idiv.asm', ('Exception', 'invalid operand types: expected (_, Integer, Integer), got (_, String, Float)',))

    def testILT(self):
        runTestThrowsException(self, 'ilt.asm', ('Exception', 'invalid operand types: expected (_, Integer, Integer), got (_, String, String)',))

    def testILTE(self):
        runTestThrowsException(self, 'ilte.asm', ('Exception', 'invalid operand types: expected (_, Integer, Integer), got (_, Foo, String)',))

    def testIGT(self):
        runTestThrowsException(self, 'igt.asm', ('Exception', 'invalid operand types: expected (_, Integer, Integer), got (_, Foo, Bar)',))

    def testIGTE(self):
        runTestThrowsException(self, 'igte.asm', ('Exception', 'invalid operand types: expected (_, Integer, Integer), got (_, Foo, Bar)',))

    def testIEQ(self):
        runTestThrowsException(self, 'ieq.asm', ('Exception', 'invalid operand types: expected (_, Integer, Integer), got (_, Foo, Bar)',))

    def testIINC(self):
        runTestThrowsException(self, 'iinc.asm', ('Exception', 'invalid operand types: expected (Integer), got (Foo)',))

    def testIDEC(self):
        runTestThrowsException(self, 'idec.asm', ('Exception', 'invalid operand types: expected (Integer), got (Function)',))

    def testFADD(self):
        runTestThrowsException(self, 'fadd.asm', ('Exception', 'invalid operand types: expected (_, Float, Float), got (_, Foo, Float)',))

    def testFSUB(self):
        runTestThrowsException(self, 'fsub.asm', ('Exception', 'invalid operand types: expected (_, Float, Float), got (_, Foo, Float)',))

    def testFMUL(self):
        runTestThrowsException(self, 'fmul.asm', ('Exception', 'invalid operand types: expected (_, Float, Float), got (_, Foo, Float)',))

    def testFDIV(self):
        runTestThrowsException(self, 'fdiv.asm', ('Exception', 'invalid operand types: expected (_, Float, Float), got (_, Foo, Float)',))

    def testFLT(self):
        runTestThrowsException(self, 'flt.asm', ('Exception', 'invalid operand types: expected (_, Float, Float), got (_, Foo, Float)',))

    def testFLTE(self):
        runTestThrowsException(self, 'flte.asm', ('Exception', 'invalid operand types: expected (_, Float, Float), got (_, Foo, Float)',))

    def testFGT(self):
        runTestThrowsException(self, 'fgt.asm', ('Exception', 'invalid operand types: expected (_, Float, Float), got (_, Foo, Float)',))

    def testFGTE(self):
        runTestThrowsException(self, 'fgte.asm', ('Exception', 'invalid operand types: expected (_, Float, Float), got (_, Foo, Float)',))

    def testFEQ(self):
        runTestThrowsException(self, 'feq.asm', ('Exception', 'invalid operand types: expected (_, Float, Float), got (_, Foo, Float)',))


class ObjectInstructionsTests(unittest.TestCase):
    """Tests checking detection of invalid operand type during instruction execution.
    """
    PATH = './sample/asm/objects'

    def testInsertRemoveInstructions(self):
        runTestReturnsIntegers(self, 'basic_insert_remove.asm', [42, 42])

    def testInsertMoves(self):
        runTest(self, 'insert_moves.asm', 'true')

    def testMoveSemanticsForInsertAndRemove(self):
        runTest(self, 'move_semantics.asm', custom_assert=partiallyAppliedSameLines(2))


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
        excode, output = run(compiled_bin_path)
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
        excode, output = run(compiled_bin_path)
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
        excode, output = run(compiled_bin_path)
        self.assertEqual(['42', ':-)'], output.strip().splitlines())
        self.assertEqual(0, excode)


class JumpingTests(unittest.TestCase):
    """
    """
    PATH = './sample/asm/absolute_jumping'

    def testAbsoluteJump(self):
        runTest(self, 'absolute_jump.asm', "Hey babe, I'm absolute.")

    def testAbsoluteBranch(self):
        runTest(self, 'absolute_branch.asm', "Hey babe, I'm absolute.")

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


class PrototypeSystemTests(unittest.TestCase):
    """Tests for prototype system inside the machine.
    """
    PATH = './sample/asm/prototype'

    def testSimplePrototypeRegistrationAndInstantation(self):
        runTest(self, 'simple.asm', ["<'Prototype' object at", "<'Custom' object at"], 0, lambda o: [' '.join(i.split()[:-1]) for i in o.splitlines()])

    def testExceptionThrownOnUnknownTypeInstantation(self):
        runTest(self, 'unregistered_type_instantation.asm', "cannot create new instance of unregistered type: Nonexistent")

    def testCatchingDerivedTypesWithBaseClassHandlers(self):
        runTest(self, 'derived_class_catching.asm', "<'Derived' object at", 0, lambda o: ' '.join(o.split()[:-1]))

    def testCatchingDeeplyDerivedTypesWithBaseClassHandlers(self):
        runTest(self, 'deeply_derived_class_catching.asm', "<'DeeplyDerived' object at", 0, lambda o: ' '.join(o.split()[:-1]))

    def testCatchingObjectsUsingMultipleInheritanceWithNoSharedBases(self):
        runTest(self, 'multiple_inheritance_with_no_shared_base_classes.asm', "<'Combined' object at", 0, lambda o: ' '.join(o.split()[:-1]))

    def testCatchingObjectsUsingMultipleInheritanceWithSharedBases(self):
        runTest(self, 'shared_bases.asm', "<'Combined' object at", 0, lambda o: ' '.join(o.split()[:-1]))

    def testDynamicDispatch(self):
        global MEMORY_LEAK_CHECKS_EXTRA_ALLOWED_LEAK_VALUES
        # FIXME: Valgrind freaks out about dlopen() leaks, comment this line if you know what to do about it
        MEMORY_LEAK_CHECKS_EXTRA_ALLOWED_LEAK_VALUES = (74351, 74367, 74383, 72736, 74399, 74375)
        #skipValgrind(self)
        runTestSplitlines(self, 'dynamic_method_dispatch.asm',
            [
                'Good day from Derived',
                'Hello from Derived',
                '',
                'Good day from MoreDerived',
                'Hello from MoreDerived',
                'Hi from MoreDerived',
            ],
        )
        MEMORY_LEAK_CHECKS_EXTRA_ALLOWED_LEAK_VALUES = ()

    def testOverridingMethods(self):
        runTestSplitlines(self, 'overriding_methods.asm',
            [
                'Hello Base World!',
                'Hello Derived World!',
                'Hello Base World!',
            ],
        )


class AssemblerStaticAnalysisErrorTests(unittest.TestCase):
    PATH = './sample/asm/static_analysis_errors'

    def testMoveFromEmptyRegister(self):
        runTestFailsToAssemble(self, 'move_from_empty_register.asm', "./sample/asm/static_analysis_errors/move_from_empty_register.asm:21:12: error: move from empty register: 0")

    def testCopyFromEmptyRegister(self):
        runTestFailsToAssemble(self, 'copy_from_empty_register.asm', "./sample/asm/static_analysis_errors/copy_from_empty_register.asm:21:12: error: copy from empty register: 0")

    def testDeleteOfEmptyRegister(self):
        runTestFailsToAssemble(self, 'delete_of_empty_register.asm', "./sample/asm/static_analysis_errors/delete_of_empty_register.asm:21:12: error: delete of empty register: 1")

    def testParameterPassFromEmptyRegister(self):
        runTestFailsToAssemble(self, 'parameter_pass_from_empty_register.asm', "./sample/asm/static_analysis_errors/parameter_pass_from_empty_register.asm:26:13: error: parameter pass from empty register: 1")

    def testParameterMoveFromEmptyRegister(self):
        runTestFailsToAssemble(self, 'parameter_move_from_empty_register.asm', "./sample/asm/static_analysis_errors/parameter_move_from_empty_register.asm:26:12: error: parameter move from empty register: 1")

    def testParameterMoveEmptiesRegisters(self):
        runTestFailsToAssemble(self, 'parameter_move_empties_registers.asm', "./sample/asm/static_analysis_errors/parameter_move_empties_registers.asm:30:11: error: print of empty register: 1")

    def testSwapWithEmptyFirstRegister(self):
        runTestFailsToAssemble(self, 'swap_with_empty_first_register.asm', "./sample/asm/static_analysis_errors/swap_with_empty_first_register.asm:21:10: error: swap with empty register: 1")

    def testSwapWithEmptySecondRegister(self):
        runTestFailsToAssemble(self, 'swap_with_empty_second_register.asm', "./sample/asm/static_analysis_errors/swap_with_empty_second_register.asm:22:12: error: swap with empty register: 2")

    def testEncloseEmptyRegisterByCopy(self):
        runTestFailsToAssemble(self, 'enclose_empty_register_by_copy.asm', "./sample/asm/static_analysis_errors/enclose_empty_register_by_copy.asm:21:19: error: closure of empty register: 1")

    def testEncloseEmptyRegisterByMove(self):
        runTestFailsToAssemble(self, 'enclose_empty_register_by_move.asm', "./sample/asm/static_analysis_errors/enclose_empty_register_by_move.asm:21:19: error: closure of empty register: 1")

    def testEncloseEmptyRegisterByReference(self):
        runTestFailsToAssemble(self, 'enclose_empty_register_by_reference.asm', "./sample/asm/static_analysis_errors/enclose_empty_register_by_reference.asm:21:15: error: closure of empty register: 1")

    def testEchoOfEmptyRegister(self):
        runTestFailsToAssemble(self, 'echo_of_empty_register.asm', "./sample/asm/static_analysis_errors/echo_of_empty_register.asm:21:10: error: echo of empty register: 1")

    def testPrintOfEmptyRegister(self):
        runTestFailsToAssemble(self, 'print_of_empty_register.asm', "./sample/asm/static_analysis_errors/print_of_empty_register.asm:21:11: error: print of empty register: 1")

    def testBranchDependsOnEmptyRegister(self):
        runTestFailsToAssemble(self, 'branch_depends_on_empty_register.asm', "./sample/asm/static_analysis_errors/branch_depends_on_empty_register.asm:21:12: error: branch depends on empty register: 1")

    def testPackingVecEmptiesRegisters(self):
        runTestFailsToAssemble(self, 'packing_vec_empties_registers.asm', "./sample/asm/static_analysis_errors/packing_vec_empties_registers.asm:26:11: error: print of empty register: 1")

    def testPackingEmptyRegister(self):
        runTestFailsToAssemble(self, 'packing_empty_register.asm', "./sample/asm/static_analysis_errors/packing_empty_register.asm:23:5: error: packing empty register: 4")

    def testUseOfEmptyFirstRegisterInAnd(self):
        runTestFailsToAssemble(self, 'and_use_of_empty_register_1st.asm', "./sample/asm/static_analysis_errors/and_use_of_empty_register_1st.asm:21:11: error: use of empty register: 1")

    def testUseOfEmptySecondRegisterInAnd(self):
        runTestFailsToAssemble(self, 'and_use_of_empty_register_2nd.asm', "./sample/asm/static_analysis_errors/and_use_of_empty_register_2nd.asm:22:13: error: use of empty register: 2")

    def testUseOfEmptyFirstRegisterInOr(self):
        runTestFailsToAssemble(self, 'or_use_of_empty_register_1st.asm', "./sample/asm/static_analysis_errors/or_use_of_empty_register_1st.asm:21:10: error: use of empty register: 1")

    def testUseOfEmptySecondRegisterInOr(self):
        runTestFailsToAssemble(self, 'or_use_of_empty_register_2nd.asm', "./sample/asm/static_analysis_errors/or_use_of_empty_register_2nd.asm:22:12: error: use of empty register: 2")

    def testIaddOfEmptyRegisters(self):
        runTestFailsToAssemble(self, 'iadd_of_empty_registers.asm', "./sample/asm/static_analysis_errors/iadd_of_empty_registers.asm:21:12: error: use of empty register: 1")

    def testNotOfEmptyRegisters(self):
        runTestFailsToAssemble(self, 'not_of_empty_register.asm', "./sample/asm/static_analysis_errors/not_of_empty_register.asm:21:9: error: not of empty register: 1")

    def testCastOfEmptyRegistersFtoi(self):
        runTestFailsToAssemble(self, 'cast_of_empty_register_ftoi.asm', "./sample/asm/static_analysis_errors/cast_of_empty_register_ftoi.asm:21:12: error: use of empty register: 1")

    def testCastOfEmptyRegistersItof(self):
        runTestFailsToAssemble(self, 'cast_of_empty_register_itof.asm', "./sample/asm/static_analysis_errors/cast_of_empty_register_itof.asm:21:12: error: use of empty register: 1")

    def testCastOfEmptyRegistersStoi(self):
        runTestFailsToAssemble(self, 'cast_of_empty_register_stoi.asm', "./sample/asm/static_analysis_errors/cast_of_empty_register_stoi.asm:21:12: error: use of empty register: 1")

    def testCastOfEmptyRegistersStof(self):
        runTestFailsToAssemble(self, 'cast_of_empty_register_stof.asm', "./sample/asm/static_analysis_errors/cast_of_empty_register_stof.asm:21:12: error: use of empty register: 1")

    def testVinsertEmptiesRegisters(self):
        runTestFailsToAssemble(self, 'vinsert_empties_registers.asm', "./sample/asm/static_analysis_errors/vinsert_empties_registers.asm:22:11: error: print of empty register: 1")

    def testVinsertOfEmptyRegister(self):
        runTestFailsToAssemble(self, 'vinsert_of_empty_register.asm', "./sample/asm/static_analysis_errors/vinsert_of_empty_register.asm:21:21: error: vinsert from empty register: 1")

    def testVinsertIntoEmptyRegister(self):
        runTestFailsToAssemble(self, 'vinsert_into_empty_register.asm', "./sample/asm/static_analysis_errors/vinsert_into_empty_register.asm:21:13: error: vinsert into empty register: 2")

    def testVpushEmptiesRegisters(self):
        runTestFailsToAssemble(self, 'vpush_empties_registers.asm', "./sample/asm/static_analysis_errors/vpush_empties_registers.asm:22:11: error: print of empty register: 1")

    def testVpushOfEmptyRegister(self):
        runTestFailsToAssemble(self, 'vpush_of_empty_register.asm', "./sample/asm/static_analysis_errors/vpush_of_empty_register.asm:21:19: error: vpush from empty register: 1")

    def testVpushIntoEmptyRegister(self):
        runTestFailsToAssemble(self, 'vpush_into_empty_register.asm', "./sample/asm/static_analysis_errors/vpush_into_empty_register.asm:21:11: error: vpush into empty register: 2")

    def testVpopFromEmptyRegister(self):
        runTestFailsToAssemble(self, 'vpop_from_empty_register.asm', "./sample/asm/static_analysis_errors/vpop_from_empty_register.asm:21:12: error: vpop from empty register: 1")

    def testVatOnEmptyRegister(self):
        runTestFailsToAssemble(self, 'vat_on_empty_register.asm', "./sample/asm/static_analysis_errors/vat_on_empty_register.asm:21:11: error: vat from empty register: 1")

    def testVlenOnEmptyRegister(self):
        runTestFailsToAssemble(self, 'vlen_on_empty_register.asm', "./sample/asm/static_analysis_errors/vlen_on_empty_register.asm:21:12: error: vlen from empty register: 1")

    def testInsertIntoEmptyRegister(self):
        runTestFailsToAssemble(self, 'insert_into_empty_register.asm', "./sample/asm/static_analysis_errors/insert_into_empty_register.asm:23:12: error: insert into empty register: target := 1")

    def testInsertKeyFromEmptyRegister(self):
        runTestFailsToAssemble(self, 'insert_key_from_empty_register.asm', "./sample/asm/static_analysis_errors/insert_key_from_empty_register.asm:23:27: error: insert key from empty register: key := 2")

    def testInsertFromEmptyRegister(self):
        runTestFailsToAssemble(self, 'insert_from_empty_register.asm', "./sample/asm/static_analysis_errors/insert_from_empty_register.asm:23:46: error: insert from empty register: value := 3")

    def testRemoveKeyFromEmptyRegister(self):
        runTestFailsToAssemble(self, 'remove_key_from_empty_register.asm', "./sample/asm/static_analysis_errors/remove_key_from_empty_register.asm:25:39: error: remove key from empty register: key := 3")

    def testRemoveFromEmptyRegister(self):
        runTestFailsToAssemble(self, 'remove_from_empty_register.asm', "./sample/asm/static_analysis_errors/remove_from_empty_register.asm:25:19: error: remove from empty register: source := 2")

    def testPointerFromEmptyRegister(self):
        runTestFailsToAssemble(self, 'pointer_from_empty_register.asm', "./sample/asm/static_analysis_errors/pointer_from_empty_register.asm:21:11: error: pointer from empty register: 1")

    def testThrowFromEmptyRegister(self):
        runTestFailsToAssemble(self, 'throw_from_empty_register.asm', "./sample/asm/static_analysis_errors/throw_from_empty_register.asm:21:11: error: throw from empty register: 1")

    def testTmpriEmptiesRegisters(self):
        runTestFailsToAssemble(self, 'tmpri_empties_registers.asm', "./sample/asm/static_analysis_errors/tmpri_empties_registers.asm:21:25: error: print of empty register: 1")

    def testTmpriFromEmptyRegister(self):
        runTestFailsToAssemble(self, 'tmpri_from_empty_register.asm', "./sample/asm/static_analysis_errors/tmpri_from_empty_register.asm:21:11: error: move to tmp register from empty register: 1")

    def testTmproMakesRegistersNonempty(self):
        runTestThrowsException(self, 'tmpro_makes_registers_nonempty.asm', ('Exception', 'temporary register set is empty'))

    def testIsnullFailsOnNonemptyRegisters(self):
        runTestFailsToAssemble(self, 'isnull_fails_on_nonempty_registers.asm', "./sample/asm/static_analysis_errors/isnull_fails_on_nonempty_registers.asm:22:19: error: useless check, register will always be defined: 1")

    def testFcallFromEmptyRegister(self):
        runTestFailsToAssemble(self, 'fcall_from_empty_register.asm', "./sample/asm/static_analysis_errors/fcall_from_empty_register.asm:22:13: error: fcall from empty register: 1")

    def testJoinFromEmptyRegister(self):
        runTestFailsToAssemble(self, 'join_from_empty_register.asm', "./sample/asm/static_analysis_errors/join_from_empty_register.asm:21:12: error: join from empty register: 1")

    def testSendTargetFromEmptyRegister(self):
        runTestFailsToAssemble(self, 'send_target_from_empty_register.asm', "./sample/asm/static_analysis_errors/send_target_from_empty_register.asm:22:10: error: send target from empty register: pid := 1")

    def testSendFromEmptyRegister(self):
        runTestFailsToAssemble(self, 'send_from_empty_register.asm', "./sample/asm/static_analysis_errors/send_from_empty_register.asm:27:12: error: send from empty register: 2")

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


class AssemblerErrorTests(unittest.TestCase):
    """Tests for error-checking and reporting functionality.
    """
    PATH = './sample/asm/errors'

    def testBranchWithoutOperands(self):
        runTestFailsToAssemble(self, 'branch_without_operands.asm', "./sample/asm/errors/branch_without_operands.asm:21:5: error: branch without operands")

    def testNoEndBetweenDefs(self):
        runTestFailsToAssemble(self, 'no_end_between_defs.asm', "./sample/asm/errors/no_end_between_defs.asm:23:1: error: another function opened before assembler reached .end after 'foo/0' function")

    def testHaltAsLastInstruction(self):
        runTestFailsToAssemble(self, 'halt_as_last_instruction.asm', "./sample/asm/errors/halt_as_last_instruction.asm:23:1: error: function does not end with 'return' or 'tailcall': main/1")

    def testArityError(self):
        runTestFailsToAssemble(self, 'arity_error.asm', "./sample/asm/errors/arity_error.asm:27:5: error: invalid number of parameters in call to function foo/1: expected 1 got 0")

    def testIsNotAValidFunctionName(self):
        runTestFailsToAssemble(self, 'is_not_a_valid_function_name.asm', "./sample/asm/errors/is_not_a_valid_function_name.asm:26:10: error: not a valid function name: foo/x")

    def testFrameWithGaps(self):
        runTestFailsToAssemble(self, 'frame_with_gaps.asm', "./sample/asm/errors/frame_with_gaps.asm:28:5: error: gap in frame defined at line 25, slot 1 left empty")

    def testPassingParameterToASlotWithTooHighIndex(self):
        runTestFailsToAssemble(self, 'passing_to_slot_with_too_high_index.asm', "./sample/asm/errors/passing_to_slot_with_too_high_index.asm:26:5: error: pass to parameter slot 3 in frame with only 3 slots available")

    def testDoublePassing(self):
        runTestFailsToAssemble(self, 'double_pass.asm', "./sample/asm/errors/double_pass.asm:29:5: error: double pass to parameter slot 2 in frame defined at line 25, first pass at line 28")

    def testMsgRequiresAtLeastOneParameter(self):
        runTestFailsToAssemble(self, 'msg_requires_at_least_one_parameter.asm', "./sample/asm/errors/msg_requires_at_least_one_parameter.asm:22:5: error: invalid number of parameters in dynamic dispatch of foo: expected at least 1, got 0")

    def testNotAValidFunctionNameMsg(self):
        runTestFailsToAssemble(self, 'not_a_valid_function_name_msg.asm', "./sample/asm/errors/not_a_valid_function_name_msg.asm:22:11: error: not a valid function name: foo/x")

    def testMsgArityMismatch(self):
        runTestFailsToAssemble(self, 'msg_arity_mismatch.asm', "./sample/asm/errors/msg_arity_mismatch.asm:22:5: error: invalid number of parameters in dynamic dispatch of add/2: expected 2 got 1")

    def testNoReturnOrTailcallAtTheEndOfAFunctionError(self):
        runTestFailsToAssemble(self, 'no_return_at_the_end_of_a_function.asm', "./sample/asm/errors/no_return_at_the_end_of_a_function.asm:22:1: error: function does not end with 'return' or 'tailcall': foo/0")

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
        runTestFailsToAssemble(self, 'excess_frame_spawned.asm', "./sample/asm/errors/excess_frame_spawned.asm:27:5: error: excess frame spawned (unused frame spawned at line 26)")

    def testLeftoverFrameTriggeredByReturn(self):
        runTestFailsToAssemble(self, 'leftover_frame_return.asm', "./sample/asm/errors/leftover_frame_return.asm:23:5: error: leftover frame (spawned at line 21)")

    def testLeftoverFrameTriggeredByThrow(self):
        runTestFailsToAssemble(self, 'leftover_frame_throw.asm', "./sample/asm/errors/leftover_frame_throw.asm:22:5: error: leftover frame (spawned at line 21)")

    def testLeftoverFrameTriggeredByLeave(self):
        runTestFailsToAssemble(self, 'leftover_frame_leave.asm', "./sample/asm/errors/leftover_frame_leave.asm:22:5: error: leftover frame (spawned at line 21)")

    def testLeftoverFrameTriggeredByEnd(self):
        runTestFailsToAssemble(self, 'leftover_frame_end.asm', "./sample/asm/errors/leftover_frame_end.asm:24:5: error: leftover frame (spawned at line 21)")

    def testCallWithoutAFrame(self):
        runTestFailsToAssemble(self, 'call_without_a_frame.asm', "./sample/asm/errors/call_without_a_frame.asm:28:5: error: call with 'tailcall' without a frame")

    def testCatchingWithUndefinedBlock(self):
        runTestFailsToAssemble(self, 'catching_with_undefined_block.asm', "./sample/asm/errors/catching_with_undefined_block.asm:27:21: error: cannot catch using undefined block: main/0__catch")

    def testEnteringUndefinedBlock(self):
        runTestFailsToAssemble(self, 'entering_undefined_block.asm', "./sample/asm/errors/entering_undefined_block.asm:22:11: error: cannot enter undefined block: foo")

    def testFunctionFromUndefinedFunction(self):
        runTestFailsToAssemble(self, 'function_from_undefined_function.asm', "./sample/asm/errors/function_from_undefined_function.asm:21:5: error: function from undefined function: foo/0")

    def testInvalidRegisterSetName(self):
        runTestFailsToAssemble(self, 'invalid_ress_instruction.asm', "./sample/asm/errors/invalid_ress_instruction.asm:21:10: error: illegal register set name in ress instruction 'foo' in function main/1")

    def testGlobalRegisterSetUsedInLibraryFunction(self):
        runTestFailsToAssemble(self, 'global_rs_used_in_lib.asm', "./sample/asm/errors/global_rs_used_in_lib.asm:21:10: error: global registers used in library function foo/0", asm_opts=('-c',))

    def testFunctionWithEmptyBody(self):
        runTestFailsToAssemble(self, 'empty_function_body.asm', "./sample/asm/errors/empty_function_body.asm:20:12: error: function with empty body: foo/0")

    def testStrayEndMarked(self):
        runTestFailsToAssemble(self, 'stray_end.asm', "./sample/asm/errors/stray_end.asm:20:1: error: stray .end marker")

    def testIllegalDirective(self):
        runTestFailsToAssemble(self, 'illegal_directive.asm', "./sample/asm/errors/illegal_directive.asm:20:1: error: illegal directive")

    def testUnknownInstruction(self):
        runTestFailsToAssemble(self, 'unknown_instruction.asm', "./sample/asm/errors/unknown_instruction.asm:21:5: error: unknown instruction: 'prnt'")

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

    def testAbsoluteJumpWithNegativeValue(self):
        runTestFailsToAssemble(self, 'absolute_jump_negative.asm', "./sample/asm/errors/absolute_jump_negative.asm:21:10: error: absolute jump with negative value")

    def testBlocksEndWithReturningInstruction(self):
        runTestFailsToAssemble(self, 'blocks_end_with_returning_instruction.asm', "./sample/asm/errors/blocks_end_with_returning_instruction.asm:22:1: error: missing returning instruction (leave, return, tailcall or halt) at the end of block: foo__block")

    def testBranchWithoutTarget(self):
        runTestFailsToAssemble(self, 'branch_without_a_target.asm', "./sample/asm/errors/branch_without_a_target.asm:23:5: error: branch without a target")

    def testBranchTrueBackwardOutOfRange(self):
        runTestFailsToAssemble(self, 'branch_true_backward_out_of_range.asm', "./sample/asm/errors/branch_true_backward_out_of_range.asm:23:14: error: backward out-of-range jump")

    def testBranchTrueForwardOutOfRange(self):
        runTestFailsToAssemble(self, 'branch_true_forward_out_of_range.asm', "./sample/asm/errors/branch_true_forward_out_of_range.asm:23:14: error: forward out-of-range jump")

    def testBranchFalseBackwardOutOfRange(self):
        runTestFailsToAssemble(self, 'branch_false_backward_out_of_range.asm', "./sample/asm/errors/branch_false_backward_out_of_range.asm:23:17: error: backward out-of-range jump")

    def testBranchFalseForwardOutOfRange(self):
        runTestFailsToAssemble(self, 'branch_false_forward_out_of_range.asm', "./sample/asm/errors/branch_false_forward_out_of_range.asm:23:17: error: forward out-of-range jump")

    def testBranchTrueForwardOutOfRangeNonrelative(self):
        runTestFailsToAssemble(self, 'branch_true_forward_out_of_range_nonrelative.asm', "./sample/asm/errors/branch_true_forward_out_of_range_nonrelative.asm:23:14: error: forward out-of-range jump")

    def testBranchFalseForwardOutOfRangeNonrelative(self):
        runTestFailsToAssemble(self, 'branch_false_forward_out_of_range_nonrelative.asm', "./sample/asm/errors/branch_false_forward_out_of_range_nonrelative.asm:23:17: error: forward out-of-range jump")

    def testBranchTrueNegativeAbsolute(self):
        runTestFailsToAssemble(self, 'branch_true_negative_absolute.asm', "./sample/asm/errors/branch_true_negative_absolute.asm:23:14: error: absolute jump with negative value")

    def testBranchFalseNegativeAbsolute(self):
        runTestFailsToAssemble(self, 'branch_false_negative_absolute.asm', "./sample/asm/errors/branch_false_negative_absolute.asm:23:17: error: absolute jump with negative value")

    def testBranchTrueToUnrecognisedMarker(self):
        runTestFailsToAssemble(self, 'branch_true_to_unrecognised_marker.asm', "./sample/asm/errors/branch_true_to_unrecognised_marker.asm:23:14: error: jump to unrecognised marker: foo")

    def testBranchFalseToUnrecognisedMarker(self):
        runTestFailsToAssemble(self, 'branch_false_to_unrecognised_marker.asm', "./sample/asm/errors/branch_false_to_unrecognised_marker.asm:23:17: error: jump to unrecognised marker: foo")

    def testZeroDistanceAbsoluteFalseBranch(self):
        runTestFailsToAssemble(self, 'zero_distance_absolute_false_branch.asm', "./sample/asm/errors/zero_distance_absolute_false_branch.asm:21:16: error: zero-distance jump")

    def testZeroDistanceAbsoluteJump(self):
        runTestFailsToAssemble(self, 'zero_distance_absolute_jump.asm', "./sample/asm/errors/zero_distance_absolute_jump.asm:21:10: error: zero-distance jump")

    def testZeroDistanceAbsoluteTrueBranch(self):
        runTestFailsToAssemble(self, 'zero_distance_absolute_true_branch.asm', "./sample/asm/errors/zero_distance_absolute_true_branch.asm:21:14: error: zero-distance jump")

    def testZeroDistanceBackwardFalseBranch(self):
        runTestFailsToAssemble(self, 'zero_distance_backward_false_branch.asm', "./sample/asm/errors/zero_distance_backward_false_branch.asm:21:16: error: zero-distance jump")

    def testZeroDistanceBackwardJump(self):
        runTestFailsToAssemble(self, 'zero_distance_backward_jump.asm', "./sample/asm/errors/zero_distance_backward_jump.asm:21:10: error: zero-distance jump")

    def testZeroDistanceBackwardTrueBranch(self):
        runTestFailsToAssemble(self, 'zero_distance_backward_true_branch.asm', "./sample/asm/errors/zero_distance_backward_true_branch.asm:21:14: error: zero-distance jump")

    def testZeroDistanceFalseBranch(self):
        runTestFailsToAssemble(self, 'zero_distance_false_branch.asm', "./sample/asm/errors/zero_distance_false_branch.asm:24:16: error: zero-distance jump")

    def testZeroDistanceForwardFalseBranch(self):
        runTestFailsToAssemble(self, 'zero_distance_forward_false_branch.asm', "./sample/asm/errors/zero_distance_forward_false_branch.asm:21:16: error: zero-distance jump")

    def testZeroDistanceForwardJump(self):
        runTestFailsToAssemble(self, 'zero_distance_forward_jump.asm', "./sample/asm/errors/zero_distance_forward_jump.asm:21:10: error: zero-distance jump")

    def testZeroDistanceForwardTrueBranch(self):
        runTestFailsToAssemble(self, 'zero_distance_forward_true_branch.asm', "./sample/asm/errors/zero_distance_forward_true_branch.asm:21:14: error: zero-distance jump")

    def testZeroDistanceJump(self):
        runTestFailsToAssemble(self, 'zero_distance_jump.asm', "./sample/asm/errors/zero_distance_jump.asm:24:10: error: zero-distance jump")

    def testZeroDistanceMarkerFalseBranch(self):
        runTestFailsToAssemble(self, 'zero_distance_marker_false_branch.asm', "./sample/asm/errors/zero_distance_marker_false_branch.asm:25:16: error: zero-distance jump")

    def testZeroDistanceMarkerJump(self):
        runTestFailsToAssemble(self, 'zero_distance_marker_jump.asm', "./sample/asm/errors/zero_distance_marker_jump.asm:24:10: error: zero-distance jump")

    def testZeroDistanceMarkerTrueBranch(self):
        runTestFailsToAssemble(self, 'zero_distance_marker_true_branch.asm', "./sample/asm/errors/zero_distance_marker_true_branch.asm:25:14: error: zero-distance jump")

    def testZeroDistanceTrueBranch(self):
        runTestFailsToAssemble(self, 'zero_distance_true_branch.asm', "./sample/asm/errors/zero_distance_true_branch.asm:24:14: error: zero-distance jump")

    def testAtLeastTwoTokensAreRequiredInAWrappedInstruction(self):
        runTestFailsToAssemble(self, 'at_least_two_tokens_required_in_a_wrapped_instruction.asm', "./sample/asm/errors/at_least_two_tokens_required_in_a_wrapped_instruction.asm:25:27: error: at least two tokens are required in a wrapped instruction")

    def testInvalidRegisterIndexInNameDirective(self):
        runTestFailsToAssemble(self, 'invalid_register_index_in_name_directive.asm', "./sample/asm/errors/invalid_register_index_in_name_directive.asm: error: in function 'main/0': invalid register index in name directive: named_register := \"bad\"")

    def testInvalidRegisterIndexInNameDirective(self):
        runTestFailsToAssemble(self, 'empty_link_directive.asm', "./sample/asm/errors/empty_link_directive.asm:21:11: error: missing module name in link directive")


class KeywordIotaTests(unittest.TestCase):
    """Tests for `iota` keyword.
    """
    PATH = './sample/asm/keyword/iota'

    def testIotaInFrame(self):
        runTestSplitlines(self, 'iota_in_frame.asm', ['Hello World!', '42'])

    def testIotaInNames(self):
        runTestSplitlines(self, 'iota_in_names.asm', ['Hello World!', '42'])

    def testIotaInRegisterIndexes(self):
        runTestSplitlines(self, 'iota_in_register_indexes.asm', ['Hello World!', '42'])

    def testIotaInReceivingArguments(self):
        runTestSplitlines(self, 'iota_in_receiving_arguments.asm', ['Hello World!', '42'])

    def testInvalidArgumentToIota(self):
        runTestFailsToAssemble(self, 'invalid_argument_to_iota.asm', "./sample/asm/keyword/iota/invalid_argument_to_iota.asm:21:12: error: invalid argument to '.iota:' directive: foo")

    def testIotaDirectiveUsedOutsideOfIotaScope(self):
        runTestFailsToAssemble(self, 'iota_directive_used_outside_of_iota_scope.asm', "./sample/asm/keyword/iota/iota_directive_used_outside_of_iota_scope.asm:20:1: error: '.iota:' directive used outside of iota scope")


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


class MiscExceptionTests(unittest.TestCase):
    PATH = './sample/asm/exceptions'

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
        runTest(self, 'terminating_processes.asm', sorted(expected_output), output_processing_function=lambda _: sorted(filter(lambda _: (_.startswith('Hello') or _.startswith('uncaught')), _.strip().splitlines())))

    def testClosureFromGlobalResgisterSet(self):
        # FIXME: passing custom assembler options will not be needed once .closure: support is completely implemented
        runTestThrowsException(self, 'closure_from_nonlocal_registers.asm', ('Exception', 'creating closures from nonlocal registers is forbidden',), assembly_opts=('--no-sa',))

    def testCatchingMachineThrownException(self):
        # pass --no-sa flag; we want to check runtime exception
        runTest(self, 'nullregister_access.asm', "exception encountered: (get) read from null register: 1", assembly_opts=('--no-sa',))

    def testCatcherState(self):
        runTestSplitlines(self, 'restore_catcher_state.asm', ['42','100','42','100'])

    def testCatchingExceptionThrownInDifferentModule(self):
        source_lib = 'thrown_in_linked_caught_in_static_fun.asm'
        lib_path = 'test_module.vlib'
        assemble(os.path.join(self.PATH, source_lib), out=lib_path, opts=('--lib',))
        runTest(self, 'thrown_in_linked_caught_in_static_base.asm', 'looks falsey: 0')

    def testVectorOutOfRangeRead(self):
        runTestThrowsException(self, 'vector_out_of_range_read.asm', ('OutOfRangeException', 'positive vector index out of range',))


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
        runTestReportsException(self, 'separate_global_rs.asm', ('Exception', '(get) read from null register: 1',), assembly_opts=('--no-sa',))


class ConcurrencyTests(unittest.TestCase):
    PATH = './sample/asm/concurrency'

    def testImmediatelyDetachingProcess(self):
        runTest(self, 'immediately_detached.asm', 'Hello World (from detached)!')

    def testHelloWorldExample(self):
        runTestSplitlines(self, 'hello_world.asm', ['Hello concurrent World! (2)', 'Hello concurrent World! (1)'], 0)

    def testJoiningProcess(self):
        runTestSplitlines(self, 'joining_a_process.asm', ['Hello concurrent World! (1)', 'Hello concurrent World! (2)'], 0)

    def testJoiningJoinedProcess(self):
        runTestThrowsException(self, 'joining_joined_process.asm', ('Exception', 'process cannot be joined',))

    def testJoiningDetachedProcess(self):
        runTestThrowsException(self, 'joining_detached_process.asm', ('Exception', 'process cannot be joined',))

    def testDetachingProcess(self):
        runTestSplitlines(
            self,
            'detaching_a_process.asm',
            [
                'false',
                'main/1 exited',
                'Hello World! (from long-running detached process) 0',
                'Hello World! (from long-running detached process) 1',
                'Hello World! (from long-running detached process) 2',
                'Hello World! (from long-running detached process) 3',
            ],
            0
        )

    def testStackCorruptedOnMainOrphaningProcess(self):
        # this will of course generate leaks, but we are not interested in them since
        # after process termination operating system will automatically reclaim memory
        runTestThrowsException(self, 'main_orphaning_processes.asm', ('Exception', 'joinable process in dropped frame',))

    def testStackCorruptedOnNonMainFunctionOrphaningProcess(self):
        # this will of course generate leaks, but we are not interested in them since
        # after process termination operating system will automatically reclaim memory
        runTestThrowsException(self, 'non_main_orphaning_processes.asm', ('Exception', 'joinable process in dropped frame',))

    def testGettingPriorityOfAProcess(self):
        runTest(self, 'get_priority.asm', '1')

    def testSettingPriorityOfAProcess(self):
        runTestSplitlines(self, 'set_priority.asm', [
            '40',
            'Hello concurrent World!',
            'Hello sequential World!',
        ])

    def testMessagePassing(self):
        runTest(self, 'message_passing.asm', 'Hello message passing World!')

    def testTransferringExceptionsOnJoin(self):
        runTest(self, 'transferring_exceptions.asm', 'exception transferred from process Process: Hello exception transferring World!')

    def testReturningValuesOnJoin(self):
        runTest(self, 'return_from_a_process.asm', '42')

    def testSuspendAndWakeup(self):
        runTestSplitlines(self, 'short_suspend_and_wakeup.asm', [
            'suspending process 0',
            'hi, I am process 1',
            'waking up process 0',
            'hi, I am process 0',
        ])

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

    def testReceiveTimeoutFailsToAssemble(self):
        runTestFailsToAssemble(self, 'receive_invalid_timeout.asm', './sample/asm/concurrency/receive_invalid_timeout.asm:21:15: error: invalid timeout operand')

    def testJoinDefaultTimeout(self):
        runTest(self, 'join_timeout_default.asm', 'child process done')

    def testJoinInfiniteTimeout(self):
        runTest(self, 'join_timeout_infinite.asm', 'child process done')

    def testJoinTimeout500ms(self):
        runTestThrowsException(self, 'join_timeout_500ms.asm', ('Exception', 'process did not join',))

    def testJoinTimeout0ms(self):
        runTestThrowsException(self, 'join_timeout_0ms.asm', ('Exception', 'process did not join',))


class WatchdogTests(unittest.TestCase):
    PATH = './sample/asm/watchdog'

    def testHelloWorldExample(self):
        runTest(self, 'hello_world.asm', 'process spawned with <Function: broken_process/0> died')

    def testWatchdogFromUndefinedFunctionCaughtByAssembler(self):
        runTestFailsToAssemble(self, 'from_undefined_function.asm', './sample/asm/watchdog/from_undefined_function.asm:60:14: error: watchdog from undefined function undefined_function/0')

    def testWatchdogFromUndefinedFunctionCaughtAtRuntime(self):
        runTestThrowsException(self, 'from_undefined_function_at_runtime.asm', ('Exception', 'watchdog process from undefined function: undefined_function/0',))

    def testWatchdogAlreadySpawnedCaughtAtRuntime(self):
        runTest(self, 'already_spawned.asm', 'process spawned with <Function: __entry> died')

    def testWatchdogMustBeANativeFunction(self):
        runTestThrowsException(self, 'must_be_a_native_function.asm', ('Exception', 'watchdog process must be a native function, used foreign World::print_hello/0',))

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


class StandardRuntimeLibraryModuleString(unittest.TestCase):
    PATH = './sample/standard_library/string'

    def testStringifyFunction(self):
        runTestCustomAsserts(self, 'stringify.asm', partiallyAppliedSameLines(2))

    def testRepresentFunction(self):
        runTestCustomAsserts(self, 'represent.asm', partiallyAppliedSameLines(2))


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


class TypeStringTests(unittest.TestCase):
    PATH = './sample/types/String'

    def testMessageSize(self):
        runTest(self, 'size.asm', '12')

    def testMessageConcatenate(self):
        runTest(self, 'concatenate.asm', ['Hello ', 'World!', 'Hello World!'], 0, lambda o: o.splitlines())

    def testMessageFormat(self):
        runTest(self, 'format.asm', 'Hello, formatted World!')

    def testMessageSubstr(self):
        runTest(self, 'substr.asm', 'Hello, World!\nHello\nWorld!')

    def testMessageStartswith(self):
        runTest(self, 'startswith.asm', 'true\nfalse')

    def testMessageEndswith(self):
        runTest(self, 'endswith.asm', 'true\nfalse')


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


class RuntimeAssertionsTests(unittest.TestCase):
    PATH = './sample/vm_runtime_assertions'

    def testAssertArity(self):
        runTest(self, 'assert_arity.asm', 'got arity 4, expected one of {1, 2, 3}')

    def testAssertTypeof(self):
        runTest(self, 'assert_typeof.asm', 'expected Integer, got String')


if __name__ == '__main__':
    if getCPUArchitecture() == 'aarch64':
        # we're running on ARM 64 and
        # Valgrind acts funny (i.e. showing leaks from itself)
        MEMORY_LEAK_CHECKS_ALLOWED_LEAK_VALUES += (74351, 74343)
    if not unittest.main(exit=False).result.wasSuccessful():
        exit(1)
    if MEMORY_LEAK_CHECKS_ENABLE:
        print('note: {0} memory leak check{2} ({1} check{3} skipped)'.format(MEMORY_LEAK_CHECKS_RUN, MEMORY_LEAK_CHECKS_SKIPPED, ('' if MEMORY_LEAK_CHECKS_RUN == 1 else 's'), ('' if MEMORY_LEAK_CHECKS_SKIPPED == 1 else 's')))
        print('note: maximum heap usage: {} bytes, {} allocs, {} frees; test: {}'.format(MEMORY_LEAK_REPORT_MAX_HEAP_USAGE['bytes'], MEMORY_LEAK_REPORT_MAX_HEAP_USAGE['allocs'], MEMORY_LEAK_REPORT_MAX_HEAP_USAGE['frees'], MEMORY_LEAK_REPORT_MAX_HEAP_USAGE['test']))
    else:
        print('memory leak checks disabled for this run')
