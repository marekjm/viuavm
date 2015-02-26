#!/usr/bin/env python3

"""This is initial unit Tests suite for Wudoo virtual machine.
It uses sample asm code (samples can also be compiled and run directly).

Each unit passes if:

    * the sample compiles,
    * compiled code runs,
    * compiled code returns correct output,

Returning correct may mean raising an exception in some cases.
"""

import os
import subprocess
import sys
import unittest


COMPILED_SAMPLES_PATH = './tests/compiled'


class WudooError(Exception):
    """Generic Wudoo exception.
    """
    pass

class WudooAssemblerError(WudooError):
    """Base class for exceptions related to Wudoo assembler.
    """
    pass

class WudooCPUError(WudooError):
    """Base class for exceptions related to Wudoo CPU.
    """
    pass


def assemble(asm, out=None, opts=(), okcodes=(0,)):
    """Assemble path given as `asm` and put binary in `out`.
    Raises exception if compilation is not successful.
    """
    asmargs = ('./bin/vm/asm',) + opts + (asm,)
    if out is not None: asmargs += (out,)
    p = subprocess.Popen(asmargs, stdout=subprocess.PIPE)
    output, error = p.communicate()
    exit_code = p.wait()
    if exit_code not in okcodes:
        raise WudooAssemblerError('{0}: {1}'.format(asm, output.decode('utf-8').strip()))
    return (output, error, exit_code)

def run(path, expected_exit_code=0):
    """Run given file with Wudoo CPU and return its output.
    """
    p = subprocess.Popen(('./bin/vm/cpu', path), stdout=subprocess.PIPE)
    output, error = p.communicate()
    exit_code = p.wait()
    if exit_code not in (expected_exit_code if type(expected_exit_code) in [list, tuple] else (expected_exit_code,)):
        raise WudooCPUError('{0}: {1}'.format(path, output.decode('utf-8').strip()))
    return (exit_code, output.decode('utf-8'))


class IntegerInstructionsTests(unittest.TestCase):
    """Tests for integer instructions.
    """
    PATH = './sample/asm/int'

    def testIADD(self):
        name = 'add.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, (name + '.bin'))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual('1', output.strip())
        self.assertEqual(0, excode)

    def testISUB(self):
        name = 'sub.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, (name + '.bin'))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual('1', output.strip())
        self.assertEqual(0, excode)

    def testIMUL(self):
        name = 'mul.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, (name + '.bin'))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual('1', output.strip())
        self.assertEqual(0, excode)

    def testIDIV(self):
        name = 'div.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, (name + '.bin'))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual('1', output.strip())
        self.assertEqual(0, excode)

    def testIDEC(self):
        name = 'dec.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, (name + '.bin'))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual('1', output.strip())
        self.assertEqual(0, excode)

    def testIINC(self):
        name = 'inc.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, (name + '.bin'))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual('1', output.strip())
        self.assertEqual(0, excode)

    def testILT(self):
        name = 'lt.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, (name + '.bin'))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual('true', output.strip())
        self.assertEqual(0, excode)

    def testILTE(self):
        name = 'lte.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, (name + '.bin'))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual('true', output.strip())
        self.assertEqual(0, excode)

    def testIGT(self):
        name = 'gt.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, (name + '.bin'))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual('true', output.strip())
        self.assertEqual(0, excode)

    def testIGTE(self):
        name = 'gte.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, (name + '.bin'))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual('true', output.strip())
        self.assertEqual(0, excode)

    def testIEQ(self):
        name = 'eq.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, (name + '.bin'))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual('true', output.strip())
        self.assertEqual(0, excode)

    def testCalculatingModulo(self):
        name = 'modulo.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, (name + '.bin'))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual('65', output.strip())
        self.assertEqual(0, excode)

    def testIntegersInCondition(self):
        name = 'in_condition.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, (name + '.bin'))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual('true', output.strip())
        self.assertEqual(0, excode)


class FloatInstructionsTests(unittest.TestCase):
    """Tests for float instructions.
    """
    PATH = './sample/asm/float'

    def testFADD(self):
        name = 'add.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, (name + '.bin'))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual('0.5', output.strip())
        self.assertEqual(0, excode)

    def testFSUB(self):
        name = 'sub.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, (name + '.bin'))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual('1.015', output.strip())
        self.assertEqual(0, excode)

    def testFMUL(self):
        name = 'mul.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, (name + '.bin'))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual('8.004', output.strip())
        self.assertEqual(0, excode)

    def testFDIV(self):
        name = 'div.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, (name + '.bin'))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual('1.57', output.strip())
        self.assertEqual(0, excode)

    def testFLT(self):
        name = 'lt.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, (name + '.bin'))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual('true', output.strip())
        self.assertEqual(0, excode)

    def testFLTE(self):
        name = 'lte.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, (name + '.bin'))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual('true', output.strip())
        self.assertEqual(0, excode)

    def testFGT(self):
        name = 'gt.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, (name + '.bin'))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual('true', output.strip())
        self.assertEqual(0, excode)

    def testFGTE(self):
        name = 'gte.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, (name + '.bin'))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual('true', output.strip())
        self.assertEqual(0, excode)

    def testFEQ(self):
        name = 'eq.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, (name + '.bin'))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual('true', output.strip())
        self.assertEqual(0, excode)

    def testFloatsInCondition(self):
        name = 'in_condition.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, (name + '.bin'))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual('true', output.strip())
        self.assertEqual(0, excode)


class ByteInstructionsTests(unittest.TestCase):
    """Tests for byte instructions.
    """
    PATH = './sample/asm/byte'

    def testHelloWorld(self):
        name = 'helloworld.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, (name + '.bin'))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual('Hello World!', output.strip())
        self.assertEqual(0, excode)


class StringInstructionsTests(unittest.TestCase):
    """Tests for string instructions.
    """
    PATH = './sample/asm/string'

    def testHelloWorld(self):
        name = 'hello_world.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, (name + '.bin'))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual('Hello World!', output.strip())
        self.assertEqual(0, excode)


class CastingInstructionsTests(unittest.TestCase):
    """Tests for byte instructions.
    """
    PATH = './sample/asm/casts'

    def testITOF(self):
        name = 'itof.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, (name + '.bin'))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual('4.0', output.strip())
        self.assertEqual(0, excode)

    def testFTOI(self):
        name = 'ftoi.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, (name + '.bin'))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual('3', output.strip())
        self.assertEqual(0, excode)


class RegisterManipulationInstructionsTests(unittest.TestCase):
    """Tests for register-manipulation instructions.
    """
    PATH = './sample/asm/regmod'

    def testCOPY(self):
        name = 'copy.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, (name + '.bin'))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual('1', output.strip())
        self.assertEqual(0, excode)

    def testMOVE(self):
        name = 'move.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, (name + '.bin'))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual('1', output.strip())
        self.assertEqual(0, excode)

    def testSWAP(self):
        name = 'swap.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, (name + '.bin'))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual([1, 0], [int(i) for i in output.strip().splitlines()])
        self.assertEqual(0, excode)

    def testISNULL(self):
        name = 'isnull.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, (name + '.bin'))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual('true', output.strip())
        self.assertEqual(0, excode)

    def testFREE(self):
        name = 'free.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, (name + '.bin'))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual('true', output.strip())
        self.assertEqual(0, excode)

    def testEMPTY(self):
        name = 'empty.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, (name + '.bin'))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual('true', output.strip())
        self.assertEqual(0, excode)


class SampleProgramsTests(unittest.TestCase):
    """Tests for various sample programs.

    These tests (as well as samples) use several types of instructions and/or
    assembler directives and as such are more stressing for both assembler and CPU.
    """
    PATH = './sample/asm'

    def testCalculatingIntegerPowerOf(self):
        name = 'power_of.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, (name + '.bin'))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual('64', output.strip())
        self.assertEqual(0, excode)

    def testCalculatingAbsoluteValueOfAnInteger(self):
        name = 'abs.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, (name + '.bin'))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual('17', output.strip())
        self.assertEqual(0, excode)

    def testLooping(self):
        name = 'looping.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, (name + '.bin'))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual([i for i in range(0, 11)], [int(i) for i in output.strip().splitlines()])
        self.assertEqual(0, excode)

    def testReferences(self):
        name = 'refs.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, (name + '.bin'))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual([2, 16], [int(i) for i in output.strip().splitlines()])
        self.assertEqual(0, excode)

    def testRegisterReferencesInIntegerOperands(self):
        name = 'registerref.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, (name + '.bin'))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual([16, 1, 1, 16], [int(i) for i in output.strip().splitlines()])
        self.assertEqual(0, excode)

    def testCalculatingFactorial(self):
        """The code that is tested by this unit is not the best implementation of factorial calculation.
        However, it tests passing parameters by value and by reference;
        so we got that going for us what is nice.
        """
        name = 'factorial.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, (name + '.bin'))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual('40320', output.strip())
        self.assertEqual(0, excode)

    def testFibonacciNumbers(self):
        """We count Fibonacci numbers from 1 to 16.
        It takes some time, you know. So, wait for it...
        """
        name = 'fibonacci.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, (name + '.bin'))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual(int(output.strip()), 1134903170)
        self.assertEqual(0, excode)


class FunctionTests(unittest.TestCase):
    """Tests for various sample programs.

    These tests (as well as samples) use several types of instructions and/or
    assembler directives and as such are more stressing for both assembler and CPU.
    """
    PATH = './sample/asm/functions'

    def testBasicFunctionSupport(self):
        name = 'definition.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, (name + '.bin'))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual('42', output.strip())
        #self.assertEqual([1, 0], [int(i) for i in output.strip().splitlines()])
        self.assertEqual(0, excode)

    def testNestedFunctionCallSupport(self):
        name = 'nested_calls.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, (name + '.bin'))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual([2015, 1995, 69, 42], [int(i) for i in output.strip().splitlines()])
        self.assertEqual(0, excode)

    def testRecursiveCallFunctionSupport(self):
        name = 'recursive.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, (name + '.bin'))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual([i for i in range(9, -1, -1)], [int(i) for i in output.strip().splitlines()])
        self.assertEqual(0, excode)

    def testLocalRegistersInFunctions(self):
        name = 'local_registers.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, (name + '.bin'))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual([42, 69], [int(i) for i in output.strip().splitlines()])
        self.assertEqual(0, excode)

    def testReturningReferences(self):
        name = 'return_by_reference.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, (name + '.bin'))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual(42, int(output.strip()))
        self.assertEqual(0, excode)

    def testStaticRegisters(self):
        name = 'static_registers.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, (name + '.bin'))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual([0, 1, 2, 3, 4, 5, 6, 7, 8, 9], [int(i) for i in output.strip().splitlines()])
        self.assertEqual(0, excode)


class ErrorTests(unittest.TestCase):
    """Tests for error-checking and reporting functionality.
    """
    PATH = './sample/asm/errors'

    @unittest.skip('due to changes in report formatting')
    def testStackTracePrinting(self):
        name = 'stacktrace.asm'
        lines = [
            'stack trace: from entry point...',
            "  called function: 'main'",
            "  called function: 'baz'",
            "  called function: 'bar'",
            "  called function: 'foo'",
            "exception in function 'foo': RuntimeException: read from null register: 1",
        ]
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, (name + '.bin'))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path, 1)
        self.assertEqual(lines, output.strip().splitlines())
        self.assertEqual(1, excode)


if __name__ == '__main__':
    unittest.main()
