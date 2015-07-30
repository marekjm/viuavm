#!/usr/bin/env python3

"""This is initial unit Tests suite for Viua virtual machine.
It uses sample asm code (samples can also be compiled and run directly).

Each unit passes if:

    * the sample compiles,
    * compiled code runs,
    * compiled code returns correct output,

Returning correct may mean raising an exception in some cases.
"""

import json
import os
import subprocess
import sys
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
    p = subprocess.Popen(('./build/bin/vm/cpu', path), stdout=subprocess.PIPE)
    output, error = p.communicate()
    exit_code = p.wait()
    if exit_code not in (expected_exit_code if type(expected_exit_code) in [list, tuple] else (expected_exit_code,)):
        raise ViuaCPUError('{0} [{1}]: {2}'.format(path, exit_code, output.decode('utf-8').strip()))
    return (exit_code, output.decode('utf-8'))


def runTest(self, name, expected_output, expected_exit_code = 0, output_processing_function = None):
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.bin'.format(self.PATH[2:].replace('/', '_'), name))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        got_output = (output.strip() if output_processing_function is None else output_processing_function(output))
        self.assertEqual(expected_output, got_output)
        self.assertEqual(expected_exit_code, excode)

        disasm_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.dis.asm'.format(self.PATH[2:].replace('/', '_'), name))
        compiled_disasm_path = '{0}.bin'.format(disasm_path)
        disassemble(compiled_path, disasm_path)
        assemble(disasm_path, compiled_disasm_path)
        dis_excode, dis_output = run(compiled_disasm_path)
        self.assertEqual(got_output, (dis_output.strip() if output_processing_function is None else output_processing_function(dis_output)))
        self.assertEqual(excode, dis_excode)

def runTestNoDisassemblyRerun(self, name, expected_output, expected_exit_code = 0, output_processing_function = None):
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.bin'.format(self.PATH[2:].replace('/', '_'), name))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        got_output = (output.strip() if output_processing_function is None else output_processing_function(output))
        self.assertEqual(expected_output, got_output)
        self.assertEqual(expected_exit_code, excode)

def runTestSplitlines(self, name, expected_output, expected_exit_code = 0):
    runTest(self, name, expected_output, expected_exit_code, output_processing_function = lambda o: o.strip().splitlines())

def runTestReturnsIntegers(self, name, expected_output, expected_exit_code = 0):
    runTest(self, name, expected_output, expected_exit_code, output_processing_function = lambda o: [int(i) for i in o.strip().splitlines()])

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
        name = 'not.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.bin'.format(self.PATH[2:].replace('/', '_'), name))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual(['false', 'true'], output.strip().splitlines())
        self.assertEqual(0, excode)

        disasm_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.dis.asm'.format(self.PATH[2:].replace('/', '_'), name))
        compiled_disasm_path = '{0}.bin'.format(disasm_path)
        disassemble(compiled_path, disasm_path)
        assemble(disasm_path, compiled_disasm_path)
        dis_excode, dis_output = run(compiled_disasm_path)
        self.assertEqual(output.strip(), dis_output.strip())
        self.assertEqual(excode, dis_excode)

    def testAND(self):
        name = 'and.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.bin'.format(self.PATH[2:].replace('/', '_'), name))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual(['false', 'true', 'false'], output.strip().splitlines())
        self.assertEqual(0, excode)

        disasm_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.dis.asm'.format(self.PATH[2:].replace('/', '_'), name))
        compiled_disasm_path = '{0}.bin'.format(disasm_path)
        disassemble(compiled_path, disasm_path)
        assemble(disasm_path, compiled_disasm_path)
        dis_excode, dis_output = run(compiled_disasm_path)
        self.assertEqual(output.strip(), dis_output.strip())
        self.assertEqual(excode, dis_excode)

    def testOR(self):
        name = 'or.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.bin'.format(self.PATH[2:].replace('/', '_'), name))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual(['false', 'true', 'true'], output.strip().splitlines())
        self.assertEqual(0, excode)

        disasm_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.dis.asm'.format(self.PATH[2:].replace('/', '_'), name))
        compiled_disasm_path = '{0}.bin'.format(disasm_path)
        disassemble(compiled_path, disasm_path)
        assemble(disasm_path, compiled_disasm_path)
        dis_excode, dis_output = run(compiled_disasm_path)
        self.assertEqual(output.strip(), dis_output.strip())
        self.assertEqual(excode, dis_excode)


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
        runTestThrowsException(self, 'neverending0.asm', 'uncaught object: Exception = Exception: "stack size (8192) exceeded with call to \'one/0\'"')


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
        runTest(self, 'simple.asm', '42')

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
        compiled_lib_path = os.path.join(COMPILED_SAMPLES_PATH, (lib_name + '.wlib'))
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


class ExternalModulesTests(unittest.TestCase):
    """Tests for C/C++ module importing, and calling external functions.
    """
    PATH = './sample/asm/external'

    def testHelloWorldExample(self):
        runTestNoDisassemblyRerun(self, 'hello_world.asm', "Hello World!")

    def testReturningAValue(self):
        runTestNoDisassemblyRerun(self, 'sqrt.asm', 1.73, 0, lambda o: round(float(o.strip()), 2))


if __name__ == '__main__':
    unittest.main()
