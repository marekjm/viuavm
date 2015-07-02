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
    asmargs = ('./bin/vm/asm',) + opts
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
    asmargs = ('./bin/vm/dis',)
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
    p = subprocess.Popen(('./bin/vm/cpu', path), stdout=subprocess.PIPE)
    output, error = p.communicate()
    exit_code = p.wait()
    if exit_code not in (expected_exit_code if type(expected_exit_code) in [list, tuple] else (expected_exit_code,)):
        raise ViuaCPUError('{0} [{1}]: {2}'.format(path, exit_code, output.decode('utf-8').strip()))
    return (exit_code, output.decode('utf-8'))


class IntegerInstructionsTests(unittest.TestCase):
    """Tests for integer instructions.
    """
    PATH = './sample/asm/int'

    def testIADD(self):
        name = 'add.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.bin'.format(self.PATH[2:].replace('/', '_'), name))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual('1', output.strip())
        self.assertEqual(0, excode)

        disasm_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.dis.asm'.format(self.PATH[2:].replace('/', '_'), name))
        compiled_disasm_path = '{0}.bin'.format(disasm_path)
        disassemble(compiled_path, disasm_path)
        assemble(disasm_path, compiled_disasm_path)
        dis_excode, dis_output = run(compiled_disasm_path)
        self.assertEqual(output.strip(), dis_output.strip())
        self.assertEqual(excode, dis_excode)

    def testISUB(self):
        name = 'sub.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.bin'.format(self.PATH[2:].replace('/', '_'), name))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual('1', output.strip())
        self.assertEqual(0, excode)

        disasm_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.dis.asm'.format(self.PATH[2:].replace('/', '_'), name))
        compiled_disasm_path = '{0}.bin'.format(disasm_path)
        disassemble(compiled_path, disasm_path)
        assemble(disasm_path, compiled_disasm_path)
        dis_excode, dis_output = run(compiled_disasm_path)
        self.assertEqual(output.strip(), dis_output.strip())
        self.assertEqual(excode, dis_excode)

    def testIMUL(self):
        name = 'mul.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.bin'.format(self.PATH[2:].replace('/', '_'), name))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual('1', output.strip())
        self.assertEqual(0, excode)

        disasm_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.dis.asm'.format(self.PATH[2:].replace('/', '_'), name))
        compiled_disasm_path = '{0}.bin'.format(disasm_path)
        disassemble(compiled_path, disasm_path)
        assemble(disasm_path, compiled_disasm_path)
        dis_excode, dis_output = run(compiled_disasm_path)
        self.assertEqual(output.strip(), dis_output.strip())
        self.assertEqual(excode, dis_excode)

    def testIDIV(self):
        name = 'div.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.bin'.format(self.PATH[2:].replace('/', '_'), name))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual('1', output.strip())
        self.assertEqual(0, excode)

        disasm_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.dis.asm'.format(self.PATH[2:].replace('/', '_'), name))
        compiled_disasm_path = '{0}.bin'.format(disasm_path)
        disassemble(compiled_path, disasm_path)
        assemble(disasm_path, compiled_disasm_path)
        dis_excode, dis_output = run(compiled_disasm_path)
        self.assertEqual(output.strip(), dis_output.strip())
        self.assertEqual(excode, dis_excode)

    def testIDEC(self):
        name = 'dec.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.bin'.format(self.PATH[2:].replace('/', '_'), name))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual('1', output.strip())
        self.assertEqual(0, excode)

        disasm_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.dis.asm'.format(self.PATH[2:].replace('/', '_'), name))
        compiled_disasm_path = '{0}.bin'.format(disasm_path)
        disassemble(compiled_path, disasm_path)
        assemble(disasm_path, compiled_disasm_path)
        dis_excode, dis_output = run(compiled_disasm_path)
        self.assertEqual(output.strip(), dis_output.strip())
        self.assertEqual(excode, dis_excode)

    def testIINC(self):
        name = 'inc.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.bin'.format(self.PATH[2:].replace('/', '_'), name))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual('1', output.strip())
        self.assertEqual(0, excode)

        disasm_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.dis.asm'.format(self.PATH[2:].replace('/', '_'), name))
        compiled_disasm_path = '{0}.bin'.format(disasm_path)
        disassemble(compiled_path, disasm_path)
        assemble(disasm_path, compiled_disasm_path)
        dis_excode, dis_output = run(compiled_disasm_path)
        self.assertEqual(output.strip(), dis_output.strip())
        self.assertEqual(excode, dis_excode)

    def testILT(self):
        name = 'lt.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.bin'.format(self.PATH[2:].replace('/', '_'), name))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual('true', output.strip())
        self.assertEqual(0, excode)

        disasm_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.dis.asm'.format(self.PATH[2:].replace('/', '_'), name))
        compiled_disasm_path = '{0}.bin'.format(disasm_path)
        disassemble(compiled_path, disasm_path)
        assemble(disasm_path, compiled_disasm_path)
        dis_excode, dis_output = run(compiled_disasm_path)
        self.assertEqual(output.strip(), dis_output.strip())
        self.assertEqual(excode, dis_excode)

    def testILTE(self):
        name = 'lte.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.bin'.format(self.PATH[2:].replace('/', '_'), name))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual('true', output.strip())
        self.assertEqual(0, excode)

        disasm_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.dis.asm'.format(self.PATH[2:].replace('/', '_'), name))
        compiled_disasm_path = '{0}.bin'.format(disasm_path)
        disassemble(compiled_path, disasm_path)
        assemble(disasm_path, compiled_disasm_path)
        dis_excode, dis_output = run(compiled_disasm_path)
        self.assertEqual(output.strip(), dis_output.strip())
        self.assertEqual(excode, dis_excode)

    def testIGT(self):
        name = 'gt.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.bin'.format(self.PATH[2:].replace('/', '_'), name))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual('true', output.strip())
        self.assertEqual(0, excode)

        disasm_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.dis.asm'.format(self.PATH[2:].replace('/', '_'), name))
        compiled_disasm_path = '{0}.bin'.format(disasm_path)
        disassemble(compiled_path, disasm_path)
        assemble(disasm_path, compiled_disasm_path)
        dis_excode, dis_output = run(compiled_disasm_path)
        self.assertEqual(output.strip(), dis_output.strip())
        self.assertEqual(excode, dis_excode)

    def testIGTE(self):
        name = 'gte.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.bin'.format(self.PATH[2:].replace('/', '_'), name))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual('true', output.strip())
        self.assertEqual(0, excode)

        disasm_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.dis.asm'.format(self.PATH[2:].replace('/', '_'), name))
        compiled_disasm_path = '{0}.bin'.format(disasm_path)
        disassemble(compiled_path, disasm_path)
        assemble(disasm_path, compiled_disasm_path)
        dis_excode, dis_output = run(compiled_disasm_path)
        self.assertEqual(output.strip(), dis_output.strip())
        self.assertEqual(excode, dis_excode)

    def testIEQ(self):
        name = 'eq.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.bin'.format(self.PATH[2:].replace('/', '_'), name))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual('true', output.strip())
        self.assertEqual(0, excode)

        disasm_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.dis.asm'.format(self.PATH[2:].replace('/', '_'), name))
        compiled_disasm_path = '{0}.bin'.format(disasm_path)
        disassemble(compiled_path, disasm_path)
        assemble(disasm_path, compiled_disasm_path)
        dis_excode, dis_output = run(compiled_disasm_path)
        self.assertEqual(output.strip(), dis_output.strip())
        self.assertEqual(excode, dis_excode)

    def testCalculatingModulo(self):
        name = 'modulo.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.bin'.format(self.PATH[2:].replace('/', '_'), name))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual('65', output.strip())
        self.assertEqual(0, excode)

        disasm_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.dis.asm'.format(self.PATH[2:].replace('/', '_'), name))
        compiled_disasm_path = '{0}.bin'.format(disasm_path)
        disassemble(compiled_path, disasm_path)
        assemble(disasm_path, compiled_disasm_path)
        dis_excode, dis_output = run(compiled_disasm_path)
        self.assertEqual(output.strip(), dis_output.strip())
        self.assertEqual(excode, dis_excode)

    def testIntegersInCondition(self):
        name = 'in_condition.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.bin'.format(self.PATH[2:].replace('/', '_'), name))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual('true', output.strip())
        self.assertEqual(0, excode)

        disasm_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.dis.asm'.format(self.PATH[2:].replace('/', '_'), name))
        compiled_disasm_path = '{0}.bin'.format(disasm_path)
        disassemble(compiled_path, disasm_path)
        assemble(disasm_path, compiled_disasm_path)
        dis_excode, dis_output = run(compiled_disasm_path)
        self.assertEqual(output.strip(), dis_output.strip())
        self.assertEqual(excode, dis_excode)

    def testBooleanAsInteger(self):
        name = 'boolean_as_int.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.bin'.format(self.PATH[2:].replace('/', '_'), name))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual('70', output.strip())
        self.assertEqual(0, excode)

        disasm_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.dis.asm'.format(self.PATH[2:].replace('/', '_'), name))
        compiled_disasm_path = '{0}.bin'.format(disasm_path)
        disassemble(compiled_path, disasm_path)
        assemble(disasm_path, compiled_disasm_path)
        dis_excode, dis_output = run(compiled_disasm_path)
        self.assertEqual(output.strip(), dis_output.strip())
        self.assertEqual(excode, dis_excode)


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
        name = 'add.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.bin'.format(self.PATH[2:].replace('/', '_'), name))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual('0.5', output.strip())
        self.assertEqual(0, excode)

        disasm_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.dis.asm'.format(self.PATH[2:].replace('/', '_'), name))
        compiled_disasm_path = '{0}.bin'.format(disasm_path)
        disassemble(compiled_path, disasm_path)
        assemble(disasm_path, compiled_disasm_path)
        dis_excode, dis_output = run(compiled_disasm_path)
        self.assertEqual(output.strip(), dis_output.strip())
        self.assertEqual(excode, dis_excode)

    def testFSUB(self):
        name = 'sub.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.bin'.format(self.PATH[2:].replace('/', '_'), name))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual('1.015', output.strip())
        self.assertEqual(0, excode)

        disasm_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.dis.asm'.format(self.PATH[2:].replace('/', '_'), name))
        compiled_disasm_path = '{0}.bin'.format(disasm_path)
        disassemble(compiled_path, disasm_path)
        assemble(disasm_path, compiled_disasm_path)
        dis_excode, dis_output = run(compiled_disasm_path)
        self.assertEqual(output.strip(), dis_output.strip())
        self.assertEqual(excode, dis_excode)

    def testFMUL(self):
        name = 'mul.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.bin'.format(self.PATH[2:].replace('/', '_'), name))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual('8.004', output.strip())
        self.assertEqual(0, excode)

        disasm_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.dis.asm'.format(self.PATH[2:].replace('/', '_'), name))
        compiled_disasm_path = '{0}.bin'.format(disasm_path)
        disassemble(compiled_path, disasm_path)
        assemble(disasm_path, compiled_disasm_path)
        dis_excode, dis_output = run(compiled_disasm_path)
        self.assertEqual(output.strip(), dis_output.strip())
        self.assertEqual(excode, dis_excode)

    def testFDIV(self):
        name = 'div.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.bin'.format(self.PATH[2:].replace('/', '_'), name))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual('1.57', output.strip())
        self.assertEqual(0, excode)

        disasm_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.dis.asm'.format(self.PATH[2:].replace('/', '_'), name))
        compiled_disasm_path = '{0}.bin'.format(disasm_path)
        disassemble(compiled_path, disasm_path)
        assemble(disasm_path, compiled_disasm_path)
        dis_excode, dis_output = run(compiled_disasm_path)
        self.assertEqual(output.strip(), dis_output.strip())
        self.assertEqual(excode, dis_excode)

    def testFLT(self):
        name = 'lt.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.bin'.format(self.PATH[2:].replace('/', '_'), name))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual('true', output.strip())
        self.assertEqual(0, excode)

        disasm_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.dis.asm'.format(self.PATH[2:].replace('/', '_'), name))
        compiled_disasm_path = '{0}.bin'.format(disasm_path)
        disassemble(compiled_path, disasm_path)
        assemble(disasm_path, compiled_disasm_path)
        dis_excode, dis_output = run(compiled_disasm_path)
        self.assertEqual(output.strip(), dis_output.strip())
        self.assertEqual(excode, dis_excode)

    def testFLTE(self):
        name = 'lte.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.bin'.format(self.PATH[2:].replace('/', '_'), name))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual('true', output.strip())
        self.assertEqual(0, excode)

        disasm_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.dis.asm'.format(self.PATH[2:].replace('/', '_'), name))
        compiled_disasm_path = '{0}.bin'.format(disasm_path)
        disassemble(compiled_path, disasm_path)
        assemble(disasm_path, compiled_disasm_path)
        dis_excode, dis_output = run(compiled_disasm_path)
        self.assertEqual(output.strip(), dis_output.strip())
        self.assertEqual(excode, dis_excode)

    def testFGT(self):
        name = 'gt.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.bin'.format(self.PATH[2:].replace('/', '_'), name))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual('true', output.strip())
        self.assertEqual(0, excode)

        disasm_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.dis.asm'.format(self.PATH[2:].replace('/', '_'), name))
        compiled_disasm_path = '{0}.bin'.format(disasm_path)
        disassemble(compiled_path, disasm_path)
        assemble(disasm_path, compiled_disasm_path)
        dis_excode, dis_output = run(compiled_disasm_path)
        self.assertEqual(output.strip(), dis_output.strip())
        self.assertEqual(excode, dis_excode)

    def testFGTE(self):
        name = 'gte.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.bin'.format(self.PATH[2:].replace('/', '_'), name))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual('true', output.strip())
        self.assertEqual(0, excode)

        disasm_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.dis.asm'.format(self.PATH[2:].replace('/', '_'), name))
        compiled_disasm_path = '{0}.bin'.format(disasm_path)
        disassemble(compiled_path, disasm_path)
        assemble(disasm_path, compiled_disasm_path)
        dis_excode, dis_output = run(compiled_disasm_path)
        self.assertEqual(output.strip(), dis_output.strip())
        self.assertEqual(excode, dis_excode)

    def testFEQ(self):
        name = 'eq.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.bin'.format(self.PATH[2:].replace('/', '_'), name))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual('true', output.strip())
        self.assertEqual(0, excode)

        disasm_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.dis.asm'.format(self.PATH[2:].replace('/', '_'), name))
        compiled_disasm_path = '{0}.bin'.format(disasm_path)
        disassemble(compiled_path, disasm_path)
        assemble(disasm_path, compiled_disasm_path)
        dis_excode, dis_output = run(compiled_disasm_path)
        self.assertEqual(output.strip(), dis_output.strip())
        self.assertEqual(excode, dis_excode)

    def testFloatsInCondition(self):
        name = 'in_condition.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.bin'.format(self.PATH[2:].replace('/', '_'), name))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual('true', output.strip())
        self.assertEqual(0, excode)

        disasm_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.dis.asm'.format(self.PATH[2:].replace('/', '_'), name))
        compiled_disasm_path = '{0}.bin'.format(disasm_path)
        disassemble(compiled_path, disasm_path)
        assemble(disasm_path, compiled_disasm_path)
        dis_excode, dis_output = run(compiled_disasm_path)
        self.assertEqual(output.strip(), dis_output.strip())
        self.assertEqual(excode, dis_excode)


class ByteInstructionsTests(unittest.TestCase):
    """Tests for byte instructions.
    """
    PATH = './sample/asm/byte'

    def testHelloWorld(self):
        name = 'helloworld.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.bin'.format(self.PATH[2:].replace('/', '_'), name))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual('Hello World!', output.strip())
        self.assertEqual(0, excode)

        disasm_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.dis.asm'.format(self.PATH[2:].replace('/', '_'), name))
        compiled_disasm_path = '{0}.bin'.format(disasm_path)
        disassemble(compiled_path, disasm_path)
        assemble(disasm_path, compiled_disasm_path)
        dis_excode, dis_output = run(compiled_disasm_path)
        self.assertEqual(output.strip(), dis_output.strip())
        self.assertEqual(excode, dis_excode)


class StringInstructionsTests(unittest.TestCase):
    """Tests for string instructions.
    """
    PATH = './sample/asm/string'

    def testHelloWorld(self):
        name = 'hello_world.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.bin'.format(self.PATH[2:].replace('/', '_'), name))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual('Hello World!', output.strip())
        self.assertEqual(0, excode)

        disasm_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.dis.asm'.format(self.PATH[2:].replace('/', '_'), name))
        compiled_disasm_path = '{0}.bin'.format(disasm_path)
        disassemble(compiled_path, disasm_path)
        assemble(disasm_path, compiled_disasm_path)
        dis_excode, dis_output = run(compiled_disasm_path)
        self.assertEqual(output.strip(), dis_output.strip())
        self.assertEqual(excode, dis_excode)


class VectorInstructionsTests(unittest.TestCase):
    """Tests for vector-related instructions.

    VEC instruction does not get its own test, but is used in every other vector test
    so it gets pretty good coverage.
    """
    PATH = './sample/asm/vector'

    def testVLEN(self):
        name = 'vlen.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.bin'.format(self.PATH[2:].replace('/', '_'), name))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual('8', output.strip())
        self.assertEqual(0, excode)

        disasm_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.dis.asm'.format(self.PATH[2:].replace('/', '_'), name))
        compiled_disasm_path = '{0}.bin'.format(disasm_path)
        disassemble(compiled_path, disasm_path)
        assemble(disasm_path, compiled_disasm_path)
        dis_excode, dis_output = run(compiled_disasm_path)
        self.assertEqual(output.strip(), dis_output.strip())
        self.assertEqual(excode, dis_excode)

    def testVINSERT(self):
        name = 'vinsert.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.bin'.format(self.PATH[2:].replace('/', '_'), name))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual(['Hurr', 'durr', 'Im\'a', 'sheep!'], output.strip().splitlines())
        self.assertEqual(0, excode)

        disasm_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.dis.asm'.format(self.PATH[2:].replace('/', '_'), name))
        compiled_disasm_path = '{0}.bin'.format(disasm_path)
        disassemble(compiled_path, disasm_path)
        assemble(disasm_path, compiled_disasm_path)
        dis_excode, dis_output = run(compiled_disasm_path)
        self.assertEqual(output.strip(), dis_output.strip())
        self.assertEqual(excode, dis_excode)

    def testVPUSH(self):
        name = 'vpush.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.bin'.format(self.PATH[2:].replace('/', '_'), name))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual(['0', '1', 'Hello World!'], output.strip().splitlines())
        self.assertEqual(0, excode)

        disasm_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.dis.asm'.format(self.PATH[2:].replace('/', '_'), name))
        compiled_disasm_path = '{0}.bin'.format(disasm_path)
        disassemble(compiled_path, disasm_path)
        assemble(disasm_path, compiled_disasm_path)
        dis_excode, dis_output = run(compiled_disasm_path)
        self.assertEqual(output.strip(), dis_output.strip())
        self.assertEqual(excode, dis_excode)

    def testVPOP(self):
        name = 'vpop.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.bin'.format(self.PATH[2:].replace('/', '_'), name))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual(['0', '1', '0', 'Hello World!'], output.strip().splitlines())
        self.assertEqual(0, excode)

        disasm_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.dis.asm'.format(self.PATH[2:].replace('/', '_'), name))
        compiled_disasm_path = '{0}.bin'.format(disasm_path)
        disassemble(compiled_path, disasm_path)
        assemble(disasm_path, compiled_disasm_path)
        dis_excode, dis_output = run(compiled_disasm_path)
        self.assertEqual(output.strip(), dis_output.strip())
        self.assertEqual(excode, dis_excode)

    def testVAT(self):
        name = 'vat.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.bin'.format(self.PATH[2:].replace('/', '_'), name))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual(['0', '1', '1', 'Hello World!'], output.strip().splitlines())
        self.assertEqual(0, excode)

        disasm_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.dis.asm'.format(self.PATH[2:].replace('/', '_'), name))
        compiled_disasm_path = '{0}.bin'.format(disasm_path)
        disassemble(compiled_path, disasm_path)
        assemble(disasm_path, compiled_disasm_path)
        dis_excode, dis_output = run(compiled_disasm_path)
        self.assertEqual(output.strip(), dis_output.strip())
        self.assertEqual(excode, dis_excode)


class CastingInstructionsTests(unittest.TestCase):
    """Tests for byte instructions.
    """
    PATH = './sample/asm/casts'

    def testITOF(self):
        name = 'itof.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.bin'.format(self.PATH[2:].replace('/', '_'), name))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual('4.0', output.strip())
        self.assertEqual(0, excode)

        disasm_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.dis.asm'.format(self.PATH[2:].replace('/', '_'), name))
        compiled_disasm_path = '{0}.bin'.format(disasm_path)
        disassemble(compiled_path, disasm_path)
        assemble(disasm_path, compiled_disasm_path)
        dis_excode, dis_output = run(compiled_disasm_path)
        self.assertEqual(output.strip(), dis_output.strip())
        self.assertEqual(excode, dis_excode)

    def testFTOI(self):
        name = 'ftoi.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.bin'.format(self.PATH[2:].replace('/', '_'), name))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual('3', output.strip())
        self.assertEqual(0, excode)

        disasm_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.dis.asm'.format(self.PATH[2:].replace('/', '_'), name))
        compiled_disasm_path = '{0}.bin'.format(disasm_path)
        disassemble(compiled_path, disasm_path)
        assemble(disasm_path, compiled_disasm_path)
        dis_excode, dis_output = run(compiled_disasm_path)
        self.assertEqual(output.strip(), dis_output.strip())
        self.assertEqual(excode, dis_excode)

    def testSTOI(self):
        name = 'stoi.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.bin'.format(self.PATH[2:].replace('/', '_'), name))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual('69', output.strip())
        self.assertEqual(0, excode)

        disasm_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.dis.asm'.format(self.PATH[2:].replace('/', '_'), name))
        compiled_disasm_path = '{0}.bin'.format(disasm_path)
        disassemble(compiled_path, disasm_path)
        assemble(disasm_path, compiled_disasm_path)
        dis_excode, dis_output = run(compiled_disasm_path)
        self.assertEqual(output.strip(), dis_output.strip())
        self.assertEqual(excode, dis_excode)


class RegisterManipulationInstructionsTests(unittest.TestCase):
    """Tests for register-manipulation instructions.
    """
    PATH = './sample/asm/regmod'

    def testCOPY(self):
        name = 'copy.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.bin'.format(self.PATH[2:].replace('/', '_'), name))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual('1', output.strip())
        self.assertEqual(0, excode)

        disasm_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.dis.asm'.format(self.PATH[2:].replace('/', '_'), name))
        compiled_disasm_path = '{0}.bin'.format(disasm_path)
        disassemble(compiled_path, disasm_path)
        assemble(disasm_path, compiled_disasm_path)
        dis_excode, dis_output = run(compiled_disasm_path)
        self.assertEqual(output.strip(), dis_output.strip())
        self.assertEqual(excode, dis_excode)

    def testMOVE(self):
        name = 'move.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.bin'.format(self.PATH[2:].replace('/', '_'), name))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual('1', output.strip())
        self.assertEqual(0, excode)

        disasm_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.dis.asm'.format(self.PATH[2:].replace('/', '_'), name))
        compiled_disasm_path = '{0}.bin'.format(disasm_path)
        disassemble(compiled_path, disasm_path)
        assemble(disasm_path, compiled_disasm_path)
        dis_excode, dis_output = run(compiled_disasm_path)
        self.assertEqual(output.strip(), dis_output.strip())
        self.assertEqual(excode, dis_excode)

    def testSWAP(self):
        name = 'swap.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.bin'.format(self.PATH[2:].replace('/', '_'), name))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual([1, 0], [int(i) for i in output.strip().splitlines()])
        self.assertEqual(0, excode)

        disasm_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.dis.asm'.format(self.PATH[2:].replace('/', '_'), name))
        compiled_disasm_path = '{0}.bin'.format(disasm_path)
        disassemble(compiled_path, disasm_path)
        assemble(disasm_path, compiled_disasm_path)
        dis_excode, dis_output = run(compiled_disasm_path)
        self.assertEqual(output.strip(), dis_output.strip())
        self.assertEqual(excode, dis_excode)

    def testISNULL(self):
        name = 'isnull.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.bin'.format(self.PATH[2:].replace('/', '_'), name))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual('true', output.strip())
        self.assertEqual(0, excode)

        disasm_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.dis.asm'.format(self.PATH[2:].replace('/', '_'), name))
        compiled_disasm_path = '{0}.bin'.format(disasm_path)
        disassemble(compiled_path, disasm_path)
        assemble(disasm_path, compiled_disasm_path)
        dis_excode, dis_output = run(compiled_disasm_path)
        self.assertEqual(output.strip(), dis_output.strip())
        self.assertEqual(excode, dis_excode)

    def testFREE(self):
        name = 'free.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.bin'.format(self.PATH[2:].replace('/', '_'), name))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual('true', output.strip())
        self.assertEqual(0, excode)

        disasm_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.dis.asm'.format(self.PATH[2:].replace('/', '_'), name))
        compiled_disasm_path = '{0}.bin'.format(disasm_path)
        disassemble(compiled_path, disasm_path)
        assemble(disasm_path, compiled_disasm_path)
        dis_excode, dis_output = run(compiled_disasm_path)
        self.assertEqual(output.strip(), dis_output.strip())
        self.assertEqual(excode, dis_excode)

    def testEMPTY(self):
        name = 'empty.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.bin'.format(self.PATH[2:].replace('/', '_'), name))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual('true', output.strip())
        self.assertEqual(0, excode)

        disasm_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.dis.asm'.format(self.PATH[2:].replace('/', '_'), name))
        compiled_disasm_path = '{0}.bin'.format(disasm_path)
        disassemble(compiled_path, disasm_path)
        assemble(disasm_path, compiled_disasm_path)
        dis_excode, dis_output = run(compiled_disasm_path)
        self.assertEqual(output.strip(), dis_output.strip())
        self.assertEqual(excode, dis_excode)


class SampleProgramsTests(unittest.TestCase):
    """Tests for various sample programs.

    These tests (as well as samples) use several types of instructions and/or
    assembler directives and as such are more stressing for both assembler and CPU.
    """
    PATH = './sample/asm'

    def testCalculatingIntegerPowerOf(self):
        name = 'power_of.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.bin'.format(self.PATH[2:].replace('/', '_'), name))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual('64', output.strip())
        self.assertEqual(0, excode)

        disasm_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.dis.asm'.format(self.PATH[2:].replace('/', '_'), name))
        compiled_disasm_path = '{0}.bin'.format(disasm_path)
        disassemble(compiled_path, disasm_path)
        assemble(disasm_path, compiled_disasm_path)
        dis_excode, dis_output = run(compiled_disasm_path)
        self.assertEqual(output.strip(), dis_output.strip())
        self.assertEqual(excode, dis_excode)

    def testCalculatingAbsoluteValueOfAnInteger(self):
        name = 'abs.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.bin'.format(self.PATH[2:].replace('/', '_'), name))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual('17', output.strip())
        self.assertEqual(0, excode)

        disasm_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.dis.asm'.format(self.PATH[2:].replace('/', '_'), name))
        compiled_disasm_path = '{0}.bin'.format(disasm_path)
        disassemble(compiled_path, disasm_path)
        assemble(disasm_path, compiled_disasm_path)
        dis_excode, dis_output = run(compiled_disasm_path)
        self.assertEqual(output.strip(), dis_output.strip())
        self.assertEqual(excode, dis_excode)

    def testLooping(self):
        name = 'looping.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.bin'.format(self.PATH[2:].replace('/', '_'), name))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual([i for i in range(0, 11)], [int(i) for i in output.strip().splitlines()])
        self.assertEqual(0, excode)

        disasm_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.dis.asm'.format(self.PATH[2:].replace('/', '_'), name))
        compiled_disasm_path = '{0}.bin'.format(disasm_path)
        disassemble(compiled_path, disasm_path)
        assemble(disasm_path, compiled_disasm_path)
        dis_excode, dis_output = run(compiled_disasm_path)
        self.assertEqual(output.strip(), dis_output.strip())
        self.assertEqual(excode, dis_excode)

    def testReferences(self):
        name = 'refs.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.bin'.format(self.PATH[2:].replace('/', '_'), name))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual([2, 16], [int(i) for i in output.strip().splitlines()])
        self.assertEqual(0, excode)

        disasm_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.dis.asm'.format(self.PATH[2:].replace('/', '_'), name))
        compiled_disasm_path = '{0}.bin'.format(disasm_path)
        disassemble(compiled_path, disasm_path)
        assemble(disasm_path, compiled_disasm_path)
        dis_excode, dis_output = run(compiled_disasm_path)
        self.assertEqual(output.strip(), dis_output.strip())
        self.assertEqual(excode, dis_excode)

    def testRegisterReferencesInIntegerOperands(self):
        name = 'registerref.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.bin'.format(self.PATH[2:].replace('/', '_'), name))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual([16, 1, 1, 16], [int(i) for i in output.strip().splitlines()])
        self.assertEqual(0, excode)

        disasm_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.dis.asm'.format(self.PATH[2:].replace('/', '_'), name))
        compiled_disasm_path = '{0}.bin'.format(disasm_path)
        disassemble(compiled_path, disasm_path)
        assemble(disasm_path, compiled_disasm_path)
        dis_excode, dis_output = run(compiled_disasm_path)
        self.assertEqual(output.strip(), dis_output.strip())
        self.assertEqual(excode, dis_excode)

    def testCalculatingFactorial(self):
        """The code that is tested by this unit is not the best implementation of factorial calculation.
        However, it tests passing parameters by value and by reference;
        so we got that going for us what is nice.
        """
        name = 'factorial.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.bin'.format(self.PATH[2:].replace('/', '_'), name))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual('40320', output.strip())
        self.assertEqual(0, excode)

        disasm_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.dis.asm'.format(self.PATH[2:].replace('/', '_'), name))
        compiled_disasm_path = '{0}.bin'.format(disasm_path)
        disassemble(compiled_path, disasm_path)
        assemble(disasm_path, compiled_disasm_path)
        dis_excode, dis_output = run(compiled_disasm_path)
        self.assertEqual(output.strip(), dis_output.strip())
        self.assertEqual(excode, dis_excode)

    def testIterativeFibonacciNumbers(self):
        """45. Fibonacci number calculated iteratively.
        """
        name = 'iterfib.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.bin'.format(self.PATH[2:].replace('/', '_'), name))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual(int(output.strip()), 1134903170)
        self.assertEqual(0, excode)

        disasm_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.dis.asm'.format(self.PATH[2:].replace('/', '_'), name))
        compiled_disasm_path = '{0}.bin'.format(disasm_path)
        disassemble(compiled_path, disasm_path)
        assemble(disasm_path, compiled_disasm_path)
        dis_excode, dis_output = run(compiled_disasm_path)
        self.assertEqual(output.strip(), dis_output.strip())
        self.assertEqual(excode, dis_excode)


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
        name = 'nested_calls.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.bin'.format(self.PATH[2:].replace('/', '_'), name))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual([2015, 1995, 69, 42], [int(i) for i in output.strip().splitlines()])
        self.assertEqual(0, excode)

        disasm_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.dis.asm'.format(self.PATH[2:].replace('/', '_'), name))
        compiled_disasm_path = '{0}.bin'.format(disasm_path)
        disassemble(compiled_path, disasm_path)
        assemble(disasm_path, compiled_disasm_path)
        dis_excode, dis_output = run(compiled_disasm_path)
        self.assertEqual(output.strip(), dis_output.strip())
        self.assertEqual(excode, dis_excode)

    def testRecursiveCallFunctionSupport(self):
        name = 'recursive.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.bin'.format(self.PATH[2:].replace('/', '_'), name))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual([i for i in range(9, -1, -1)], [int(i) for i in output.strip().splitlines()])
        self.assertEqual(0, excode)

        disasm_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.dis.asm'.format(self.PATH[2:].replace('/', '_'), name))
        compiled_disasm_path = '{0}.bin'.format(disasm_path)
        disassemble(compiled_path, disasm_path)
        assemble(disasm_path, compiled_disasm_path)
        dis_excode, dis_output = run(compiled_disasm_path)
        self.assertEqual(output.strip(), dis_output.strip())
        self.assertEqual(excode, dis_excode)

    def testLocalRegistersInFunctions(self):
        name = 'local_registers.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.bin'.format(self.PATH[2:].replace('/', '_'), name))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual([42, 69], [int(i) for i in output.strip().splitlines()])
        self.assertEqual(0, excode)

        disasm_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.dis.asm'.format(self.PATH[2:].replace('/', '_'), name))
        compiled_disasm_path = '{0}.bin'.format(disasm_path)
        disassemble(compiled_path, disasm_path)
        assemble(disasm_path, compiled_disasm_path)
        dis_excode, dis_output = run(compiled_disasm_path)
        self.assertEqual(output.strip(), dis_output.strip())
        self.assertEqual(excode, dis_excode)

    def testReturningReferences(self):
        name = 'return_by_reference.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.bin'.format(self.PATH[2:].replace('/', '_'), name))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual(42, int(output.strip()))
        self.assertEqual(0, excode)

        disasm_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.dis.asm'.format(self.PATH[2:].replace('/', '_'), name))
        compiled_disasm_path = '{0}.bin'.format(disasm_path)
        disassemble(compiled_path, disasm_path)
        assemble(disasm_path, compiled_disasm_path)
        dis_excode, dis_output = run(compiled_disasm_path)
        self.assertEqual(output.strip(), dis_output.strip())
        self.assertEqual(excode, dis_excode)

    def testStaticRegisters(self):
        name = 'static_registers.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.bin'.format(self.PATH[2:].replace('/', '_'), name))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual([0, 1, 2, 3, 4, 5, 6, 7, 8, 9], [int(i) for i in output.strip().splitlines()])
        self.assertEqual(0, excode)

        disasm_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.dis.asm'.format(self.PATH[2:].replace('/', '_'), name))
        compiled_disasm_path = '{0}.bin'.format(disasm_path)
        disassemble(compiled_path, disasm_path)
        assemble(disasm_path, compiled_disasm_path)
        dis_excode, dis_output = run(compiled_disasm_path)
        self.assertEqual(output.strip(), dis_output.strip())
        self.assertEqual(excode, dis_excode)


class HigherOrderFunctionTests(unittest.TestCase):
    """Tests for higher-order function support.
    """
    PATH = './sample/asm/functions/higher_order'

    def testApply(self):
        name = 'apply.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, (name + '.bin'))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual('25', output.strip())
        self.assertEqual(0, excode)

    def testInvoke(self):
        name = 'invoke.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, (name + '.bin'))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual(['42', '42'], output.splitlines())
        self.assertEqual(0, excode)

    def testMap(self):
        name = 'map.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, (name + '.bin'))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual([[1, 2, 3, 4, 5], [1, 4, 9, 16, 25]], [json.loads(i) for i in output.splitlines()])
        self.assertEqual(0, excode)

    def testFilter(self):
        name = 'filter.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, (name + '.bin'))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual([[1, 2, 3, 4, 5], [2, 4]], [json.loads(i) for i in output.splitlines()])
        self.assertEqual(0, excode)

    def testFilterByClosure(self):
        name = 'filter_closure.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, (name + '.bin'))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual([[1, 2, 3, 4, 5], [2, 4]], [json.loads(i) for i in output.splitlines()])
        self.assertEqual(0, excode)


class ClosureTests(unittest.TestCase):
    """Tests for closures.
    """
    PATH = './sample/asm/functions/closures'

    def testSimpleClosure(self):
        name = 'simple.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.bin'.format(self.PATH[2:].replace('/', '_'), name))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual('42', output.strip())
        self.assertEqual(0, excode)

        disasm_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.dis.asm'.format(self.PATH[2:].replace('/', '_'), name))
        compiled_disasm_path = '{0}.bin'.format(disasm_path)
        disassemble(compiled_path, disasm_path)
        assemble(disasm_path, compiled_disasm_path)
        dis_excode, dis_output = run(compiled_disasm_path)
        self.assertEqual(output.strip(), dis_output.strip())
        self.assertEqual(excode, dis_excode)

    def testVariableSharingBetweenTwoClosures(self):
        name = 'shared_variables.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.bin'.format(self.PATH[2:].replace('/', '_'), name))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual([42, 69], [int(i) for i in output.strip().splitlines()])
        self.assertEqual(0, excode)

        disasm_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.dis.asm'.format(self.PATH[2:].replace('/', '_'), name))
        compiled_disasm_path = '{0}.bin'.format(disasm_path)
        disassemble(compiled_path, disasm_path)
        assemble(disasm_path, compiled_disasm_path)
        dis_excode, dis_output = run(compiled_disasm_path)
        self.assertEqual(output.strip(), dis_output.strip())
        self.assertEqual(excode, dis_excode)


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
        name = 'absolute_jump.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.bin'.format(self.PATH[2:].replace('/', '_'), name))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual("Hey babe, I'm absolute.", output.strip())
        self.assertEqual(0, excode)

        disasm_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.dis.asm'.format(self.PATH[2:].replace('/', '_'), name))
        compiled_disasm_path = '{0}.bin'.format(disasm_path)
        disassemble(compiled_path, disasm_path)
        assemble(disasm_path, compiled_disasm_path)
        dis_excode, dis_output = run(compiled_disasm_path)
        self.assertEqual(output.strip(), dis_output.strip())
        self.assertEqual(excode, dis_excode)

    def testAbsoluteBranch(self):
        name = 'absolute_branch.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.bin'.format(self.PATH[2:].replace('/', '_'), name))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual("Hey babe, I'm absolute.", output.strip())
        self.assertEqual(0, excode)

        disasm_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.dis.asm'.format(self.PATH[2:].replace('/', '_'), name))
        compiled_disasm_path = '{0}.bin'.format(disasm_path)
        disassemble(compiled_path, disasm_path)
        assemble(disasm_path, compiled_disasm_path)
        dis_excode, dis_output = run(compiled_disasm_path)
        self.assertEqual(output.strip(), dis_output.strip())
        self.assertEqual(excode, dis_excode)


class TryCatchBlockTests(unittest.TestCase):
    """Tests for user code thrown exceptions.
    """
    PATH = './sample/asm/blocks'

    def testBasicNoThrowNoCatchBlock(self):
        name = 'basic.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.bin'.format(self.PATH[2:].replace('/', '_'), name))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual("42", output.strip())
        self.assertEqual(0, excode)

        disasm_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.dis.asm'.format(self.PATH[2:].replace('/', '_'), name))
        compiled_disasm_path = '{0}.bin'.format(disasm_path)
        disassemble(compiled_path, disasm_path)
        assemble(disasm_path, compiled_disasm_path)
        dis_excode, dis_output = run(compiled_disasm_path)
        self.assertEqual(output.strip(), dis_output.strip())
        self.assertEqual(excode, dis_excode)

    def testCatchingBuiltinType(self):
        name = 'catching_builtin_type.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.bin'.format(self.PATH[2:].replace('/', '_'), name))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual("42", output.strip())
        self.assertEqual(0, excode)

        disasm_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.dis.asm'.format(self.PATH[2:].replace('/', '_'), name))
        compiled_disasm_path = '{0}.bin'.format(disasm_path)
        disassemble(compiled_path, disasm_path)
        assemble(disasm_path, compiled_disasm_path)
        dis_excode, dis_output = run(compiled_disasm_path)
        self.assertEqual(output.strip(), dis_output.strip())
        self.assertEqual(excode, dis_excode)


class CatchingMachineThrownExceptionTests(unittest.TestCase):
    """Tests for catching machine-thrown exceptions.
    """
    PATH = './sample/asm/exceptions'

    def testCatchingMachineThrownException(self):
        name = 'nullregister_access.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.bin'.format(self.PATH[2:].replace('/', '_'), name))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual("exception encountered: (get) read from null register: 1", output.strip())
        self.assertEqual(0, excode)

        disasm_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.dis.asm'.format(self.PATH[2:].replace('/', '_'), name))
        compiled_disasm_path = '{0}.bin'.format(disasm_path)
        disassemble(compiled_path, disasm_path)
        assemble(disasm_path, compiled_disasm_path)
        dis_excode, dis_output = run(compiled_disasm_path)
        self.assertEqual(output.strip(), dis_output.strip())
        self.assertEqual(excode, dis_excode)


class AssemblerErrorTests(unittest.TestCase):
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
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.bin'.format(self.PATH[2:].replace('/', '_'), name))
        assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path, 1)
        self.assertEqual(lines, output.strip().splitlines())
        self.assertEqual(1, excode)

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
        name = 'hello_world.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.bin'.format(self.PATH[2:].replace('/', '_'), name))
        os.system('{0} -std=c++11 -fPIC -shared -o World.so ./sample/asm/external/World.cpp'.format((os.getenv('CXX') or 'g++')))
        output, error, exit_code = assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual("Hello World!", output.strip())
        self.assertEqual(0, exit_code)

        disasm_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.dis.asm'.format(self.PATH[2:].replace('/', '_'), name))
        compiled_disasm_path = '{0}.bin'.format(disasm_path)
        disassemble(compiled_path, disasm_path)
        assemble(disasm_path, compiled_disasm_path)
        dis_excode, dis_output = run(compiled_disasm_path)
        self.assertEqual(output.strip(), dis_output.strip())
        self.assertEqual(excode, dis_excode)

    def testReturningAValue(self):
        name = 'sqrt.asm'
        assembly_path = os.path.join(self.PATH, name)
        compiled_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.bin'.format(self.PATH[2:].replace('/', '_'), name))
        os.system('{0} -std=c++11 -fPIC -c -o registerset.o ./src/cpu/registerset.cpp'.format((os.getenv('CXX') or 'g++')))
        os.system('{0} -std=c++11 -fPIC -c -o exception.o ./src/types/exception.cpp'.format((os.getenv('CXX') or 'g++')))
        os.system('{0} -std=c++11 -fPIC -shared -o math.so ./sample/asm/external/math.cpp registerset.o exception.o'.format((os.getenv('CXX') or 'g++')))
        output, error, exit_code = assemble(assembly_path, compiled_path)
        excode, output = run(compiled_path)
        self.assertEqual(1.73, round(float(output.strip()), 2))
        self.assertEqual(0, exit_code)

        disasm_path = os.path.join(COMPILED_SAMPLES_PATH, '{0}_{1}.dis.asm'.format(self.PATH[2:].replace('/', '_'), name))
        compiled_disasm_path = '{0}.bin'.format(disasm_path)
        disassemble(compiled_path, disasm_path)
        assemble(disasm_path, compiled_disasm_path)
        dis_excode, dis_output = run(compiled_disasm_path)
        self.assertEqual(output.strip(), dis_output.strip())
        self.assertEqual(excode, dis_excode)


if __name__ == '__main__':
    unittest.main()
