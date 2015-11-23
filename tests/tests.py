#!/usr/bin/env python3

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
    p = subprocess.Popen(('./build/bin/vm/cpu', path), stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    output, error = p.communicate()
    exit_code = p.wait()
    if exit_code not in (expected_exit_code if type(expected_exit_code) in [list, tuple] else (expected_exit_code,)):
        raise ViuaCPUError('{0} [{1}]: {2}'.format(path, exit_code, output.decode('utf-8').strip()))
    return (exit_code, output.decode('utf-8'))

MEMORY_LEAK_CHECKS_SKIPPED = 0
MEMORY_LEAK_CHECKS_RUN = 0
MEMORY_LEAK_CHECKS_ENABLE = True
MEMORY_LEAK_CHECKS_SKIP_LIST = []
valgrind_regex_heap_summary_in_use_at_exit = re.compile('in use at exit: (\d+(?:,\d+)?) bytes in (\d+) blocks')
valgrind_regex_heap_summary_total_heap_usage = re.compile('total heap usage: (\d+(?:,\d+)?) allocs, (\d+(?:,\d+)?) frees, (\d+(?:,\d+)?) bytes allocated')
valgrind_regex_leak_summary_definitely_lost = re.compile('definitely lost: (\d+(?:,\d+)?) bytes in (\d+) blocks')
valgrind_regex_leak_summary_indirectly_lost = re.compile('indirectly lost: (\d+(?:,\d+)?) bytes in (\d+) blocks')
valgrind_regex_leak_summary_possibly_lost = re.compile('possibly lost: (\d+(?:,\d+)?) bytes in (\d+) blocks')
valgrind_regex_leak_summary_still_reachable = re.compile('still reachable: (\d+(?:,\d+)?) bytes in (\d+) blocks')
valgrind_regex_leak_summary_suppressed = re.compile('suppressed: (\d+(?:,\d+)?) bytes in (\d+) blocks')
def valgrindBytesInBlocks(line, regex):
    matched = regex.search(line)
    return {'bytes': int(matched.group(1).replace(',', '')), 'blocks': int(matched.group(2))}

def valgrindSummary(text):
    output_lines = text.splitlines()
    valprefix = output_lines[0].split(' ')[0]
    interesting_lines = [line[len(valprefix):].strip() for line in output_lines[output_lines.index('{0} HEAP SUMMARY:'.format(valprefix)):]]

    in_use_at_exit = valgrindBytesInBlocks(interesting_lines[1], valgrind_regex_heap_summary_in_use_at_exit)
    # print(interesting_lines[1], in_use_at_exit)

    total_heap_usage_matched = valgrind_regex_heap_summary_total_heap_usage.search(interesting_lines[2])
    total_heap_usage = {
        'allocs': int(total_heap_usage_matched.group(1).replace(',', '')),
        'frees': int(total_heap_usage_matched.group(2).replace(',', '')),
        'bytes': int(total_heap_usage_matched.group(3).replace(',', '')),
    }
    # print(interesting_lines[2], total_heap_usage)

    definitely_lost = valgrindBytesInBlocks(interesting_lines[5], valgrind_regex_leak_summary_definitely_lost)
    # print(interesting_lines[5], definitely_lost)

    indirectly_lost = valgrindBytesInBlocks(interesting_lines[6], valgrind_regex_leak_summary_indirectly_lost)
    # print(interesting_lines[6], indirectly_lost)

    possibly_lost = valgrindBytesInBlocks(interesting_lines[7], valgrind_regex_leak_summary_possibly_lost)
    # print(interesting_lines[7], possibly_lost)

    still_reachable = valgrindBytesInBlocks(interesting_lines[8], valgrind_regex_leak_summary_still_reachable)
    # print(interesting_lines[8], still_reachable)

    suppressed = valgrindBytesInBlocks(interesting_lines[9], valgrind_regex_leak_summary_suppressed)
    # print(interesting_lines[9], suppressed)

    summary = {'heap': {}, 'leak': {}}

    summary['heap']['in_use_at_exit'] = in_use_at_exit
    summary['heap']['total_heap_usage'] = total_heap_usage

    summary['leak']['definitely_lost'] = definitely_lost
    summary['leak']['indirectly_lost'] = indirectly_lost
    summary['leak']['possibly_lost'] = possibly_lost
    summary['leak']['still_reachable'] = still_reachable
    summary['leak']['suppressed'] = suppressed
    return summary

def valgrindCheck(self, path):
    """Run compiled code under Valgrind to check for memory leaks.
    """
    p = subprocess.Popen(('valgrind', './build/bin/vm/cpu', path), stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    output, error = p.communicate()
    exit_code = p.wait()

    error = error.decode('utf-8')
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
    if summary['leak']['still_reachable']['bytes'] not in (0, 72704): memory_was_leaked = True
    if summary['leak']['suppressed']['bytes']: memory_was_leaked = True

    if memory_was_leaked:
        print(error)

    total_leak_bytes  = summary['leak']['definitely_lost']['bytes']
    total_leak_bytes += summary['leak']['indirectly_lost']['bytes']
    total_leak_bytes += summary['leak']['possibly_lost']['bytes']
    total_leak_bytes += summary['leak']['still_reachable']['bytes']
    total_leak_bytes += summary['leak']['suppressed']['bytes']
    self.assertIn(total_leak_bytes, (0, 72704))

    return 0


def runMemoryLeakCheck(self, compiled_path, check_memory_leaks):
    if not MEMORY_LEAK_CHECKS_ENABLE: return
    global MEMORY_LEAK_CHECKS_RUN, MEMORY_LEAK_CHECKS_SKIPPED

    if self in MEMORY_LEAK_CHECKS_SKIP_LIST or not check_memory_leaks:
        print('skipped memory leak check for: {0} ({1}.{2})'.format(compiled_path, self.__class__.__name__, str(self).split()[0]))
        MEMORY_LEAK_CHECKS_SKIPPED += 1
    else:
        MEMORY_LEAK_CHECKS_RUN += 1
        valgrindCheck(self, compiled_path)

def runTest(self, name, expected_output, expected_exit_code = 0, output_processing_function = None, check_memory_leaks = True):
    assembly_path = os.path.join(self.PATH, name)
    compiled_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.bin'.format(self.PATH[2:].replace('/', '_'), name))
    assemble(assembly_path, compiled_path)
    excode, output = run(compiled_path, expected_exit_code)
    got_output = (output.strip() if output_processing_function is None else output_processing_function(output))
    self.assertEqual(expected_output, got_output)
    self.assertEqual(expected_exit_code, excode)

    runMemoryLeakCheck(self, compiled_path, check_memory_leaks)

    disasm_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.dis.asm'.format(self.PATH[2:].replace('/', '_'), name))
    compiled_disasm_path = '{0}.bin'.format(disasm_path)
    disassemble(compiled_path, disasm_path)
    assemble(disasm_path, compiled_disasm_path)
    dis_excode, dis_output = run(compiled_disasm_path, expected_exit_code)
    self.assertEqual(got_output, (dis_output.strip() if output_processing_function is None else output_processing_function(dis_output)))
    self.assertEqual(excode, dis_excode)

def runTestNoDisassemblyRerun(self, name, expected_output, expected_exit_code = 0, output_processing_function = None, check_memory_leaks = True):
    assembly_path = os.path.join(self.PATH, name)
    compiled_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.bin'.format(self.PATH[2:].replace('/', '_'), name))
    assemble(assembly_path, compiled_path)
    excode, output = run(compiled_path)
    got_output = (output.strip() if output_processing_function is None else output_processing_function(output))
    self.assertEqual(expected_output, got_output)
    self.assertEqual(expected_exit_code, excode)
    runMemoryLeakCheck(self, compiled_path, check_memory_leaks)

def runTestCustomAssertsNoDisassemblyRerun(self, name, assertions_callback, check_memory_leaks = True):
    assembly_path = os.path.join(self.PATH, name)
    compiled_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.bin'.format(self.PATH[2:].replace('/', '_'), name))
    assemble(assembly_path, compiled_path)
    excode, output = run(compiled_path)
    assertions_callback(self, excode, output)
    runMemoryLeakCheck(self, compiled_path, check_memory_leaks)

def runTestSplitlines(self, name, expected_output, expected_exit_code = 0):
    runTest(self, name, expected_output, expected_exit_code, output_processing_function = lambda o: o.strip().splitlines())

def runTestSplitlinesNoDisassemblyRerun(self, name, expected_output, expected_exit_code = 0, check_memory_leaks = True):
    runTestNoDisassemblyRerun(self, name, expected_output, expected_exit_code, output_processing_function = lambda o: o.strip().splitlines(), check_memory_leaks=check_memory_leaks)

def runTestReturnsIntegers(self, name, expected_output, expected_exit_code = 0, check_memory_leaks=True):
    runTest(self, name, expected_output, expected_exit_code, output_processing_function = lambda o: [int(i) for i in o.strip().splitlines()], check_memory_leaks=check_memory_leaks)

def runTestThrowsException(self, name, expected_output):
    assembly_path = os.path.join(self.PATH, name)
    compiled_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.bin'.format(self.PATH[2:].replace('/', '_'), name))
    assemble(assembly_path, compiled_path)
    excode, output = run(compiled_path, 1)
    got_exception = [line for line in output.strip().splitlines() if line.startswith('uncaught object:')][0]
    self.assertEqual(1, excode)
    self.assertEqual(got_exception, expected_output)


class IntegerInstructionsTests(unittest.TestCase):
    """Tests for integer instructions.
    """
    PATH = './sample/asm/int'

    def testIADD(self):
        runTest(self, 'add.asm', '1', 0)

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

    def testFADD(self):
        runTest(self, 'add.asm', '0.5', 0)

    def testFSUB(self):
        runTest(self, 'sub.asm', '1.015', 0)

    def testFMUL(self):
        runTest(self, 'mul.asm', '8.004', 0)

    def testFDIV(self):
        runTest(self, 'div.asm', '1.57', 0)

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

    def testVLEN(self):
        runTest(self, 'vlen.asm', '8', 0)

    def testVINSERT(self):
        runTest(self, 'vinsert.asm', ['Hurr', 'durr', 'Im\'a', 'sheep!'], 0, lambda o: o.strip().splitlines())

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
        runTest(self, 'itof.asm', '4.0')

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

    def testFREE(self):
        runTest(self, 'free.asm', 'true')

    def testEMPTY(self):
        runTest(self, 'empty.asm', 'true')


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

    def testReferences(self):
        runTestReturnsIntegers(self, 'refs.asm', [2, 16])

    def testRegisterReferencesInIntegerOperands(self):
        runTestReturnsIntegers(self, 'registerref.asm', [16, 1, 1, 16])

    def testCalculatingFactorial(self):
        """The code that is tested by this unit is not the best implementation of factorial calculation.
        However, it tests passing parameters by value and by reference;
        so we got that going for us what is nice.
        """
        runTest(self, 'factorial.asm', '40320')

    def testIterativeFibonacciNumbers(self):
        """45. Fibonacci number calculated iteratively.
        """
        runTest(self, 'iterfib.asm', 1134903170, 0, lambda o: int(o.strip()))


class FunctionTests(unittest.TestCase):
    """Tests for function related parts of the VM.
    """
    PATH = './sample/asm/functions'

    def testBasicFunctionSupport(self):
        name = 'definition.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.bin'.format(self.PATH[2:].replace('/', '_'), name))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual('42', output.strip())
        self.assertEqual(0, excode)
        # for now disassembler can't figure out what function is used as main
        # disasm_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.dis.asm'.format(self.PATH[2:].replace('/', '_'), name))
        # compiled_disasm_path = '{0}.bin'.format(disasm_path)
        # disassemble(compiled_path, disasm_path)
        # assemble(disasm_path, compiled_disasm_path)
        # dis_excode, dis_output = run(compiled_disasm_path)
        # self.assertEqual(output.strip(), dis_output.strip())
        # self.assertEqual(excode, dis_excode)

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
        runTestNoDisassemblyRerun(self, 'parameters_vector.asm', '[0, 1, 2, 3]')

    def testReturningReferences(self):
        runTest(self, 'return_by_reference.asm', 42, 0, lambda o: int(o.strip()))

    def testStaticRegisters(self):
        runTestReturnsIntegers(self, 'static_registers.asm', [i for i in range(0, 10)])

    def testNeverendingFunction(self):
        runTestSplitlines(self, 'neverending.asm', ['42', '48'])

    def testNeverendingFunction0(self):
        runTestThrowsException(self, 'neverending0.asm', 'uncaught object: Exception = stack size (8192) exceeded with call to \'one/0\'')


class HigherOrderFunctionTests(unittest.TestCase):
    """Tests for higher-order function support.
    """
    PATH = './sample/asm/functions/higher_order'

    def testApply(self):
        runTest(self, 'apply.asm', '25')

    def testInvoke(self):
        runTestSplitlines(self, 'invoke.asm', ['42', '42'])

    def testMap(self):
        runTest(self, 'map.asm', [[1, 2, 3, 4, 5], [1, 4, 9, 16, 25]], 0, lambda o: [json.loads(i) for i in o.splitlines()])

    def testFilter(self):
        runTest(self, 'filter.asm', [[1, 2, 3, 4, 5], [2, 4]], 0, lambda o: [json.loads(i) for i in o.splitlines()])

    def testFilterByClosure(self):
        runTest(self, 'filter_closure.asm', [[1, 2, 3, 4, 5], [2, 4]], 0, lambda o: [json.loads(i) for i in o.splitlines()])


class ClosureTests(unittest.TestCase):
    """Tests for closures.
    """
    PATH = './sample/asm/functions/closures'

    def testSimpleClosure(self):
        runTest(self, 'simple.asm', '42', output_processing_function=None)

    def testVariableSharingBetweenTwoClosures(self):
        runTestReturnsIntegers(self, 'shared_variables.asm', [42, 69])


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
        runTest(self, 'basic.asm', '42')

    def testCatchingBuiltinType(self):
        runTest(self, 'catching_builtin_type.asm', '42')


class CatchingMachineThrownExceptionTests(unittest.TestCase):
    """Tests for catching machine-thrown exceptions.
    """
    PATH = './sample/asm/exceptions'

    def testCatchingMachineThrownException(self):
        runTest(self, 'nullregister_access.asm', "exception encountered: (get) read from null register: 1")

    def testCatcherState(self):
        runTestSplitlines(self, 'restore_catcher_state.asm', ['42','100','42','100'])


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

    def testDynamicDispatch(self):
        runTestSplitlinesNoDisassemblyRerun(self, 'dynamic_method_dispatch.asm',
            [
                'Good day from Derived',
                'Hello from Derived',
                '',
                'Good day from MoreDerived',
                'Hello from MoreDerived',
                'Hi from MoreDerived',
            ],
        )


class AssemblerErrorTests(unittest.TestCase):
    """Tests for error-checking and reporting functionality.
    """
    PATH = './sample/asm/errors'

    def testNoEndBetweenDefs(self):
        name = 'no_end_between_defs.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.bin'.format(self.PATH[2:].replace('/', '_'), name))
        output, error, exit_code = assemble(assembly_path, compiled_path, okcodes=(1,))
        self.assertEqual("error: function gathering failed: another function opened before assembler reached .end after 'foo' function", output.strip())
        self.assertEqual(1, exit_code)

    def testHaltAsLastInstruction(self):
        name = 'halt_as_last_instruction.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.bin'.format(self.PATH[2:].replace('/', '_'), name))
        output, error, exit_code = assemble(assembly_path, compiled_path, opts=('--Ehalt-is-last',), okcodes=(1,0))
        self.assertEqual(1, exit_code)
        self.assertEqual("error: using 'halt' instead of 'end' as last instruction in main function leads to memory leaks", output.strip())


class ExternalModulesTests(unittest.TestCase):
    """Tests for C/C++ module importing, and calling external functions.
    """
    PATH = './sample/asm/external'

    def testHelloWorldExample(self):
        runTestNoDisassemblyRerun(self, 'hello_world.asm', "Hello World!")

    def testReturningAValue(self):
        runTestNoDisassemblyRerun(self, 'sqrt.asm', 1.73, 0, lambda o: round(float(o.strip()), 2))


class MultithreadingTests(unittest.TestCase):
    PATH = './sample/asm/threading'

    def testHelloWorldExample(self):
        runTestSplitlines(self, 'hello_world.asm', ['Hello multithreaded World! (2)', 'Hello multithreaded World! (1)'], 0)

    def testJoiningThread(self):
        runTestSplitlines(self, 'joining_a_thread.asm', ['Hello multithreaded World! (1)', 'Hello multithreaded World! (2)'], 0)

    def testDetachingThread(self):
        runTestSplitlines(
            self,
            'detaching_a_thread.asm',
            [
                'false',
                'main/1 exited',
                'Hello World! (from long-running detached thread) 0',
                'Hello World! (from long-running detached thread) 1',
                'Hello World! (from long-running detached thread) 2',
                'Hello World! (from long-running detached thread) 3',
            ],
            0
        )

    def testStackCorruptedOnMainOrphaningThreads(self):
        # this will of course generate leaks, but we are not interested in them since
        # after process termination operating system will automatically reclaim memory
        MEMORY_LEAK_CHECKS_SKIP_LIST.append(self)
        runTestSplitlines(self, 'main_orhpaning_threads.asm', ['Hello multithreaded World! (2)', 'fatal: aborting execution: main/1 orphaned threads, stack corrupted'], 1)

    def testGettingPriorityOfAThread(self):
        runTest(self, 'get_priority.asm', '1')

    def testSettingPriorityOfAThread(self):
        runTestSplitlines(self, 'set_priority.asm', [
            '40',
            'Hello concurrent World!',
            'Hello sequential World!',
        ])

    def testMessagePassing(self):
        runTest(self, 'message_passing.asm', 'Hello message passing World!')

    def testTransferringExceptionsOnJoin(self):
        runTest(self, 'transferring_exceptions.asm', 'exception transferred from thread Thread: Hello exception transferring World!')


def sameLines(self, excode, output, no_of_lines):
    lines = output.splitlines()
    self.assertTrue(len(lines) == no_of_lines)
    for i in range(1, no_of_lines):
        self.assertEqual(lines[0], lines[i])

def partiallyAppliedSameLines(n):
    return functools.partial(sameLines, no_of_lines=n)

class StandardRuntimeLibraryModuleString(unittest.TestCase):
    PATH = './sample/standard_library/string'

    def testStringifyFunction(self):
        runTestCustomAssertsNoDisassemblyRerun(self, 'stringify.asm', partiallyAppliedSameLines(2))

    def testRepresentFunction(self):
        runTestCustomAssertsNoDisassemblyRerun(self, 'represent.asm', partiallyAppliedSameLines(2))


class TypeStringTests(unittest.TestCase):
    PATH = './sample/types/String'

    def testMessageSize(self):
        runTest(self, 'size.asm', '12')

    def testMessageConcatenate(self):
        runTest(self, 'concatenate.asm', ['Hello ', 'World!', 'Hello World!'], 0, lambda o: o.splitlines())


if __name__ == '__main__':
    if not unittest.main(exit=False).result.wasSuccessful():
        exit(1)
    if MEMORY_LEAK_CHECKS_ENABLE:
        print('{0} memory leak check{2} ({1} check{3} skipped)'.format(MEMORY_LEAK_CHECKS_RUN, MEMORY_LEAK_CHECKS_SKIPPED, ('' if MEMORY_LEAK_CHECKS_RUN == 1 else 's'), ('' if MEMORY_LEAK_CHECKS_SKIPPED == 1 else 's')))
    else:
        print('memory leak checks disabled for this run')
