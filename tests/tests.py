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
MEMORY_LEAK_CHECKS_ALLOWED_LEAK_VALUES = (0, 72704)
MEMORY_LEAK_CHECKS_EXTRA_ALLOWED_LEAK_VALUES = ()
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
    # also, we shall sometimes allow (sigh...) additional memory to "leak" if Valgrind freaks out
    if summary['leak']['still_reachable']['bytes'] not in (MEMORY_LEAK_CHECKS_ALLOWED_LEAK_VALUES + MEMORY_LEAK_CHECKS_EXTRA_ALLOWED_LEAK_VALUES): memory_was_leaked = True
    if summary['leak']['suppressed']['bytes']: memory_was_leaked = True

    if memory_was_leaked:
        print(error)

    total_leak_bytes  = summary['leak']['definitely_lost']['bytes']
    total_leak_bytes += summary['leak']['indirectly_lost']['bytes']
    total_leak_bytes += summary['leak']['possibly_lost']['bytes']
    total_leak_bytes += summary['leak']['still_reachable']['bytes']
    total_leak_bytes += summary['leak']['suppressed']['bytes']
    self.assertIn(total_leak_bytes, (MEMORY_LEAK_CHECKS_ALLOWED_LEAK_VALUES + MEMORY_LEAK_CHECKS_EXTRA_ALLOWED_LEAK_VALUES))

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

def runTest(self, name, expected_output=None, expected_exit_code = 0, output_processing_function = None, check_memory_leaks = True, custom_assert=None):
    if expected_output is None and custom_assert is None:
        raise TypeError('`expected_output` and `custom_assert` cannot be both None')
    assembly_path = os.path.join(self.PATH, name)
    compiled_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.bin'.format(self.PATH[2:].replace('/', '_'), name))
    assemble(assembly_path, compiled_path)
    excode, output = run(compiled_path, expected_exit_code)
    got_output = (output.strip() if output_processing_function is None else output_processing_function(output))
    if custom_assert is not None:
        custom_assert(self, excode, got_output)
    else:
        self.assertEqual(expected_output, got_output)
        self.assertEqual(expected_exit_code, excode)
    runMemoryLeakCheck(self, compiled_path, check_memory_leaks)

    disasm_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.dis.asm'.format(self.PATH[2:].replace('/', '_'), name))
    compiled_disasm_path = '{0}.bin'.format(disasm_path)
    disassemble(compiled_path, disasm_path)
    assemble(disasm_path, compiled_disasm_path)
    dis_excode, dis_output = run(compiled_disasm_path, expected_exit_code)
    if custom_assert is not None:
        custom_assert(self, dis_excode, (dis_output.strip() if output_processing_function is None else output_processing_function(dis_output)))
    else:
        self.assertEqual(got_output, (dis_output.strip() if output_processing_function is None else output_processing_function(dis_output)))
        self.assertEqual(excode, dis_excode)

def runTestNoDisassemblyRerun(self, name, expected_output, expected_exit_code = 0, output_processing_function = None, check_memory_leaks = True):
    assembly_path = os.path.join(self.PATH, name)
    compiled_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.bin'.format(self.PATH[2:].replace('/', '_'), name))
    assemble(assembly_path, compiled_path)
    excode, output = run(compiled_path, expected_exit_code)
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

def runTestReturnsUnorderedLines(self, name, expected_output, expected_exit_code = 0):
    runTest(self, name, sorted(expected_output), expected_exit_code, output_processing_function = lambda o: sorted(o.strip().splitlines()))

def runTestReturnsUnorderedLinesNoDisassemblyRerun(self, name, expected_output, expected_exit_code = 0):
    runTestNoDisassemblyRerun(self, name, sorted(expected_output), expected_exit_code, output_processing_function = lambda o: sorted(o.strip().splitlines()))

def runTestReturnsIntegers(self, name, expected_output, expected_exit_code = 0, check_memory_leaks=True):
    runTest(self, name, expected_output, expected_exit_code, output_processing_function = lambda o: [int(i) for i in o.strip().splitlines()], check_memory_leaks=check_memory_leaks)

def runTestThrowsException(self, name, expected_output):
    assembly_path = os.path.join(self.PATH, name)
    compiled_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.bin'.format(self.PATH[2:].replace('/', '_'), name))
    assemble(assembly_path, compiled_path)
    excode, output = run(compiled_path, 1)
    got_exception = list(filter(lambda line: line.startswith('uncaught object:'), output.strip().splitlines()))[0]
    self.assertEqual(1, excode)
    self.assertEqual(got_exception, expected_output)

def runTestFailsToAssemble(self, name, expected_output):
    assembly_path = os.path.join(self.PATH, name)
    compiled_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.bin'.format(self.PATH[2:].replace('/', '_'), name))
    output, error, exit_code = assemble(assembly_path, compiled_path, okcodes=(0, 1))
    self.assertEqual(1, exit_code)
    self.assertEqual(output.strip(), expected_output)


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

    def testDELETE(self):
        runTest(self, 'delete.asm', 'true')

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
        runTestNoDisassemblyRerun(self, 'parameters_vector.asm', '[0, 1, 2, 3]')

    def testReturningReferences(self):
        runTest(self, 'return_by_reference.asm', 42, 0, lambda o: int(o.strip()))

    def testStaticRegisters(self):
        runTestReturnsIntegers(self, 'static_registers.asm', [i for i in range(0, 10)])

    def testCallWithPassByMove(self):
        runTest(self, 'pass_by_move.asm', None, custom_assert=partiallyAppliedSameLines(3))

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
        runTest(self, 'filter_closure.asm', [[1, 2, 3, 4, 5], [2, 4]], 0, lambda o: [json.loads(i) for i in o.splitlines()])

    def testFilterByClosureVectorByMove(self):
        runTest(self, 'filter_closure_vector_by_move.asm', [[1, 2, 3, 4, 5], [2, 4]], 0, lambda o: [json.loads(i) for i in o.splitlines()])


class ClosureTests(unittest.TestCase):
    """Tests for closures.
    """
    PATH = './sample/asm/functions/closures'

    def testSimpleClosure(self):
        runTest(self, 'simple.asm', '42', output_processing_function=None)

    def testVariableSharingBetweenTwoClosures(self):
        runTestReturnsIntegers(self, 'shared_variables.asm', [42, 69])

    def testAdder(self):
        runTestReturnsIntegers(self, 'adder.asm', [5, 8, 16])

    def testEnclosedVariableLeftInScope(self):
        runTestSplitlines(self, 'enclosed_variable_left_in_scope.asm', ['Hello World!', '42'])

    def testChangeEnclosedVariableFromClosure(self):
        runTestSplitlines(self, 'change_enclosed_variable_from_closure.asm', ['Hello World!', '42'])

    def testNestedClosures(self):
        runTest(self, 'nested_closures.asm', '10')

    def testSimpleEncloseByCopy(self):
        runTest(self, 'simple_enclose_by_copy.asm', '42')

    def testEncloseCopyCreatesIndependentObjects(self):
        runTestSplitlines(self, 'enclosecopy_creates_independent_objects.asm', ['Hello World!', 'Hello World!', '42', 'Hello World!'])

    def testSimpleEncloseByMove(self):
        runTestSplitlines(self, 'simple_enclose_by_move.asm', ['true', 'Hello World!'])


class InvalidInstructionOperandTypeTests(unittest.TestCase):
    """Tests checking detection of invalid operand type during instruction execution.
    """
    PATH = './sample/asm/invalid_operand_types'

    def testIADD(self):
        runTestThrowsException(self, 'iadd.asm', 'uncaught object: Exception = invalid operand types: expected (_, Integer, Integer), got (_, Integer, Float)')

    def testISUB(self):
        runTestThrowsException(self, 'isub.asm', 'uncaught object: Exception = invalid operand types: expected (_, Integer, Integer), got (_, Integer, String)')

    def testIMUL(self):
        runTestThrowsException(self, 'imul.asm', 'uncaught object: Exception = invalid operand types: expected (_, Integer, Integer), got (_, Float, String)')

    def testIDIV(self):
        runTestThrowsException(self, 'idiv.asm', 'uncaught object: Exception = invalid operand types: expected (_, Integer, Integer), got (_, String, Float)')

    def testILT(self):
        runTestThrowsException(self, 'ilt.asm', 'uncaught object: Exception = invalid operand types: expected (_, Integer, Integer), got (_, String, String)')

    def testILTE(self):
        runTestThrowsException(self, 'ilte.asm', 'uncaught object: Exception = invalid operand types: expected (_, Integer, Integer), got (_, Foo, String)')

    def testIGT(self):
        runTestThrowsException(self, 'igt.asm', 'uncaught object: Exception = invalid operand types: expected (_, Integer, Integer), got (_, Foo, Bar)')

    def testIGTE(self):
        runTestThrowsException(self, 'igte.asm', 'uncaught object: Exception = invalid operand types: expected (_, Integer, Integer), got (_, Foo, Bar)')

    def testIEQ(self):
        runTestThrowsException(self, 'ieq.asm', 'uncaught object: Exception = invalid operand types: expected (_, Integer, Integer), got (_, Foo, Bar)')

    def testIINC(self):
        runTestThrowsException(self, 'iinc.asm', 'uncaught object: Exception = invalid operand types: expected (Integer), got (Foo)')

    def testIDEC(self):
        runTestThrowsException(self, 'idec.asm', 'uncaught object: Exception = invalid operand types: expected (Integer), got (Function)')

    def testFADD(self):
        runTestThrowsException(self, 'fadd.asm', 'uncaught object: Exception = invalid operand types: expected (_, Float, Float), got (_, Foo, Float)')

    def testFSUB(self):
        runTestThrowsException(self, 'fsub.asm', 'uncaught object: Exception = invalid operand types: expected (_, Float, Float), got (_, Foo, Float)')

    def testFMUL(self):
        runTestThrowsException(self, 'fmul.asm', 'uncaught object: Exception = invalid operand types: expected (_, Float, Float), got (_, Foo, Float)')

    def testFDIV(self):
        runTestThrowsException(self, 'fdiv.asm', 'uncaught object: Exception = invalid operand types: expected (_, Float, Float), got (_, Foo, Float)')

    def testFLT(self):
        runTestThrowsException(self, 'flt.asm', 'uncaught object: Exception = invalid operand types: expected (_, Float, Float), got (_, Foo, Float)')

    def testFLTE(self):
        runTestThrowsException(self, 'flte.asm', 'uncaught object: Exception = invalid operand types: expected (_, Float, Float), got (_, Foo, Float)')

    def testFGT(self):
        runTestThrowsException(self, 'fgt.asm', 'uncaught object: Exception = invalid operand types: expected (_, Float, Float), got (_, Foo, Float)')

    def testFGTE(self):
        runTestThrowsException(self, 'fgte.asm', 'uncaught object: Exception = invalid operand types: expected (_, Float, Float), got (_, Foo, Float)')

    def testFEQ(self):
        runTestThrowsException(self, 'feq.asm', 'uncaught object: Exception = invalid operand types: expected (_, Float, Float), got (_, Foo, Float)')


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

    def testCatchingObjectsUsingMultipleInheritanceWithSharedBases(self):
        runTest(self, 'shared_bases.asm', "<'Combined' object at", 0, lambda o: ' '.join(o.split()[:-1]))

    def testDynamicDispatch(self):
        global MEMORY_LEAK_CHECKS_EXTRA_ALLOWED_LEAK_VALUES
        # FIXME: Valgrind freaks out about dlopen() leaks, comment this line if you know what to do about it
        MEMORY_LEAK_CHECKS_EXTRA_ALLOWED_LEAK_VALUES = (74351, 74367, 74383)
        skipValgrind(self)
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
        MEMORY_LEAK_CHECKS_EXTRA_ALLOWED_LEAK_VALUES = ()

    def testOverridingMethods(self):
        runTestSplitlinesNoDisassemblyRerun(self, 'overriding_methods.asm',
            [
                'Hello Base World!',
                'Hello Derived World!',
                'Hello Base World!',
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
        self.assertEqual("./sample/asm/errors/halt_as_last_instruction.asm: error: using 'halt' instead of 'return' as last instruction in main function leads to memory leaks", output.strip())

    def testArityError(self):
        name = 'arity_error.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.bin'.format(self.PATH[2:].replace('/', '_'), name))
        output, error, exit_code = assemble(assembly_path, compiled_path, opts=('--Ehalt-is-last',), okcodes=(1,0))
        self.assertEqual(1, exit_code)
        self.assertEqual("./sample/asm/errors/arity_error.asm:8: error: invalid number of parameters in call to function foo/1: expected 1 got 0", output.strip())

    def testNoReturnAtTheEndOfAFunctionError(self):
        name = 'no_return_at_the_end_of_a_function.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.bin'.format(self.PATH[2:].replace('/', '_'), name))
        output, error, exit_code = assemble(assembly_path, compiled_path, opts=('--Emissing-return',), okcodes=(1,0))
        self.assertEqual(1, exit_code)
        self.assertEqual("./sample/asm/errors/no_return_at_the_end_of_a_function.asm:3: error: missing 'return' at the end of function foo/0", output.strip())

    def testBlockWithEmptyBody(self):
        name = 'empty_block_body.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.bin'.format(self.PATH[2:].replace('/', '_'), name))
        output, error, exit_code = assemble(assembly_path, compiled_path, okcodes=(1,0))
        self.assertEqual(1, exit_code)
        self.assertEqual("./sample/asm/errors/empty_block_body.asm:1: error: block with empty body: foo", output.strip())

    def testCallToUndefinedFunction(self):
        name = 'call_to_undefined_function.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.bin'.format(self.PATH[2:].replace('/', '_'), name))
        output, error, exit_code = assemble(assembly_path, compiled_path, okcodes=(1,0))
        self.assertEqual(1, exit_code)
        self.assertEqual("./sample/asm/errors/call_to_undefined_function.asm:3: error: call to undefined function foo/1", output.strip())

    def testInvalidFunctionName(self):
        name = 'invalid_function_name.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.bin'.format(self.PATH[2:].replace('/', '_'), name))
        output, error, exit_code = assemble(assembly_path, compiled_path, okcodes=(1,0))
        self.assertEqual(1, exit_code)
        self.assertEqual("./sample/asm/errors/invalid_function_name.asm:11: error: invalid function name: foo/x", output.strip())

    def testExcessFrameSpawned(self):
        name = 'excess_frame_spawned.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.bin'.format(self.PATH[2:].replace('/', '_'), name))
        output, error, exit_code = assemble(assembly_path, compiled_path, okcodes=(1,0))
        self.assertEqual(1, exit_code)
        self.assertEqual("./sample/asm/errors/excess_frame_spawned.asm:8: error: excess frame spawned (unused frame spawned at line 7)", output.strip())

    def testCallWithoutAFrame(self):
        name = 'call_without_a_frame.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.bin'.format(self.PATH[2:].replace('/', '_'), name))
        output, error, exit_code = assemble(assembly_path, compiled_path, okcodes=(1,0))
        self.assertEqual(1, exit_code)
        self.assertEqual("./sample/asm/errors/call_without_a_frame.asm:9: error: call with 'tailcall' without a frame", output.strip())

    def testEnteringUndefinedBlock(self):
        name = 'entering_undefined_block.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.bin'.format(self.PATH[2:].replace('/', '_'), name))
        output, error, exit_code = assemble(assembly_path, compiled_path, okcodes=(1,0))
        self.assertEqual(1, exit_code)
        self.assertEqual("./sample/asm/errors/entering_undefined_block.asm:3: error: cannot enter undefined block: foo", output.strip())

    def testFunctionFromUndefinedFunction(self):
        name = 'function_from_undefined_function.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.bin'.format(self.PATH[2:].replace('/', '_'), name))
        output, error, exit_code = assemble(assembly_path, compiled_path, okcodes=(1,0))
        self.assertEqual(1, exit_code)
        self.assertEqual("./sample/asm/errors/function_from_undefined_function.asm:2: error: function from undefined function: foo/0", output.strip())

    def testInvalidRegisterSetName(self):
        name = 'invalid_ress_instruction.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.bin'.format(self.PATH[2:].replace('/', '_'), name))
        output, error, exit_code = assemble(assembly_path, compiled_path, okcodes=(1,0))
        self.assertEqual(1, exit_code)
        self.assertEqual("./sample/asm/errors/invalid_ress_instruction.asm:2: error: illegal register set name in ress instruction 'foo' in function main", output.strip())

    def testGlobalRegisterSetUsedInLibraryFunction(self):
        name = 'global_rs_used_in_lib.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.bin'.format(self.PATH[2:].replace('/', '_'), name))
        output, error, exit_code = assemble(assembly_path, compiled_path, opts=('-c',), okcodes=(1,0))
        self.assertEqual(1, exit_code)
        self.assertEqual("./sample/asm/errors/global_rs_used_in_lib.asm:2: error: global registers used in library function foo", output.strip())

    def testFunctionWithEmptyBody(self):
        name = 'empty_function_body.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.bin'.format(self.PATH[2:].replace('/', '_'), name))
        output, error, exit_code = assemble(assembly_path, compiled_path, okcodes=(1,0))
        self.assertEqual(1, exit_code)
        self.assertEqual("./sample/asm/errors/empty_function_body.asm:1: error: function with empty body: foo/0", output.strip())

    def testStrayEndMarked(self):
        name = 'stray_end.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.bin'.format(self.PATH[2:].replace('/', '_'), name))
        output, error, exit_code = assemble(assembly_path, compiled_path, okcodes=(1,0))
        self.assertEqual(1, exit_code)
        self.assertEqual("./sample/asm/errors/stray_end.asm:1: error: stray .end marker", output.strip())

    def testIllegalDirective(self):
        name = 'illegal_directive.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.bin'.format(self.PATH[2:].replace('/', '_'), name))
        output, error, exit_code = assemble(assembly_path, compiled_path, okcodes=(1,0))
        self.assertEqual(1, exit_code)
        self.assertEqual("./sample/asm/errors/illegal_directive.asm:1: error: illegal directive on line : '.fuction:'", output.strip())


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


class ExternalModulesTests(unittest.TestCase):
    """Tests for C/C++ module importing, and calling external functions.
    """
    PATH = './sample/asm/external'

    def testHelloWorldExample(self):
        global MEMORY_LEAK_CHECKS_EXTRA_ALLOWED_LEAK_VALUES
        # FIXME: Valgrind freaks out about dlopen() leaks, comment this line if you know what to do about it
        # or maybe the leak originates in Viua code but I haven't found the leak
        MEMORY_LEAK_CHECKS_EXTRA_ALLOWED_LEAK_VALUES = (72736,)
        runTestNoDisassemblyRerun(self, 'hello_world.asm', "Hello World!")

    def testReturningAValue(self):
        global MEMORY_LEAK_CHECKS_EXTRA_ALLOWED_LEAK_VALUES
        # FIXME: Valgrind freaks out about dlopen() leaks, comment this line if you know what to do about it
        # or maybe the leak originates in Viua code but I haven't found the leak
        MEMORY_LEAK_CHECKS_EXTRA_ALLOWED_LEAK_VALUES = (72736,)
        runTestNoDisassemblyRerun(self, 'sqrt.asm', 1.73, 0, lambda o: round(float(o.strip()), 2))

    def testThrowingExceptionHandledByWatchdog(self):
        global MEMORY_LEAK_CHECKS_EXTRA_ALLOWED_LEAK_VALUES
        # FIXME: Valgrind freaks out about dlopen() leaks, comment this line if you know what to do about it
        # or maybe the leak originates in Viua code but I haven't found the leak
        MEMORY_LEAK_CHECKS_EXTRA_ALLOWED_LEAK_VALUES = (72736,)
        runTestNoDisassemblyRerun(self, 'throwing.asm', 'OH NOES!', 0)


class ConcurrencyTests(unittest.TestCase):
    PATH = './sample/asm/concurrency'

    def testHelloWorldExample(self):
        runTestSplitlines(self, 'hello_world.asm', ['Hello concurrent World! (2)', 'Hello concurrent World! (1)'], 0)

    def testJoiningProcess(self):
        runTestSplitlines(self, 'joining_a_process.asm', ['Hello concurrent World! (1)', 'Hello concurrent World! (2)'], 0)

    def testJoiningJoinedProcess(self):
        runTestThrowsException(self, 'joining_joined_process.asm', 'uncaught object: Exception = process cannot be joined')

    def testJoiningDetachedProcess(self):
        runTestThrowsException(self, 'joining_detached_process.asm', 'uncaught object: Exception = process cannot be joined')

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
        runTestThrowsException(self, 'main_orphaning_processes.asm', 'uncaught object: Exception = joinable process in dropped frame')

    def testStackCorruptedOnNonMainFunctionOrphaningProcess(self):
        # this will of course generate leaks, but we are not interested in them since
        # after process termination operating system will automatically reclaim memory
        runTestThrowsException(self, 'non_main_orphaning_processes.asm', 'uncaught object: Exception = joinable process in dropped frame')

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


class WatchdogTests(unittest.TestCase):
    PATH = './sample/asm/watchdog'

    def testHelloWorldExample(self):
        runTest(self, 'hello_world.asm', 'process spawned with <Function: broken_process> died')

    def testWatchdogFromUndefinedFunctionCaughtByAssembler(self):
        runTestFailsToAssemble(self, 'from_undefined_function.asm', './sample/asm/watchdog/from_undefined_function.asm:41: error: watchdog from undefined function undefined_function/0')

    def testWatchdogFromUndefinedFunctionCaughtAtRuntime(self):
        runTestThrowsException(self, 'from_undefined_function_at_runtime.asm', 'uncaught object: Exception = watchdog process from undefined function: undefined_function')

    def testWatchdogAlreadySpawnedCaughtAtRuntime(self):
        runTest(self, 'already_spawned.asm', 'process spawned with <Function: __entry> died')

    def testWatchdogMustBeANativeFunction(self):
        runTestThrowsException(self, 'must_be_a_native_function.asm', 'uncaught object: Exception = watchdog process must be native function, used foreign World::print_hello')

    def testWatchdogTerminatedByARunawayExceptionDoesNotLeak(self):
        runTest(self, 'terminated_watchdog.asm', 'watchdog process terminated by: Function: \'Function: broken_process\'')

    def testServicingRunawayExceptionWhileOtherProcessesAreRunning(self):
        runTestReturnsUnorderedLinesNoDisassemblyRerun(self, 'death_message.asm', [
            "Hello World (from detached process)!",
            "Hello World (from joined process)!",
            "process [ joined ]: 'a_joined_concurrent_process' exiting",
            "[WARNING] process 'Function: __entry' killed by >>>OH NOES!<<<",
            "Hello World (from detached process) after a runaway exception!",
            "process [detached]: 'a_detached_concurrent_process' exiting",
        ])

    def testRestartingProcessesAfterAbortedByRunawayException(self):
        runTestReturnsUnorderedLinesNoDisassemblyRerun(self, 'restarting_process.asm', [
            "process [  main  ]: 'main' exiting",
            "Hello World (from detached process)!",
            "[WARNING] process 'Function: a_division_executing_process[42, 0]' killed by >>>cannot divide by zero<<<",
            "42 / 1 = 42",
            "Hello World (from detached process) after a runaway exception!",
            "process [detached]: 'a_detached_concurrent_process' exiting",
        ])


class StandardRuntimeLibraryModuleString(unittest.TestCase):
    PATH = './sample/standard_library/string'

    def testStringifyFunction(self):
        runTestCustomAssertsNoDisassemblyRerun(self, 'stringify.asm', partiallyAppliedSameLines(2))

    def testRepresentFunction(self):
        runTestCustomAssertsNoDisassemblyRerun(self, 'represent.asm', partiallyAppliedSameLines(2))


class StandardRuntimeLibraryModuleVector(unittest.TestCase):
    PATH = './sample/standard_library/vector'

    def testVectorOfInts(self):
        runTestNoDisassemblyRerun(self, 'of_ints.asm', '[0, 1, 2, 3, 4, 5, 6, 7]')

    def testVectorOf(self):
        runTestNoDisassemblyRerun(self, 'of.asm', '[0, 1, 2, 3, 4, 5, 6, 7]')

    def testVectorReverse(self):
        runTestNoDisassemblyRerun(self, 'reverse.asm', ['[0, 1, 2, 3, 4, 5, 6, 7]', '[7, 6, 5, 4, 3, 2, 1, 0]'], output_processing_function=lambda o: o.strip().splitlines())

    def testVectorReverseInPlace(self):
        runTestNoDisassemblyRerun(self, 'reverse_in_place.asm', ['[0, 1, 2, 3, 4, 5, 6, 7]', '[7, 6, 5, 4, 3, 2, 1, 0]'], output_processing_function=lambda o: o.strip().splitlines())

    def testVectorEveryReturnsTrue(self):
        runTestNoDisassemblyRerun(self, 'every_returns_true.asm', 'true')

    def testVectorEveryReturnsFalse(self):
        runTestNoDisassemblyRerun(self, 'every_returns_false.asm', 'false')

    def testVectorAnyReturnsTrue(self):
        runTestNoDisassemblyRerun(self, 'any_returns_true.asm', 'true')

    def testVectorAnyReturnsFalse(self):
        runTestNoDisassemblyRerun(self, 'any_returns_false.asm', 'false')


class StandardRuntimeLibraryModuleFunctional(unittest.TestCase):
    PATH = './sample/standard_library/functional'

    def testApplyHelloWorldHelloJoeHelloMike(self):
        runTestSplitlinesNoDisassemblyRerun(self, 'hello_guys.asm', ['Hello World!', 'Hello Joe!', 'Hello Mike!'])

    def testApplyThatReturnsAValue(self):
        runTestNoDisassemblyRerun(self, 'apply_simple.asm', '42')


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
        runTestNoDisassemblyRerun(self, 'check_if_is_expired.asm', 'expired: false\nexpired: true')


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
        print('{0} memory leak check{2} ({1} check{3} skipped)'.format(MEMORY_LEAK_CHECKS_RUN, MEMORY_LEAK_CHECKS_SKIPPED, ('' if MEMORY_LEAK_CHECKS_RUN == 1 else 's'), ('' if MEMORY_LEAK_CHECKS_SKIPPED == 1 else 's')))
    else:
        print('memory leak checks disabled for this run')
