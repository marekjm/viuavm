#!/usr/bin/env python3

import datetime
import enum
import glob
import io
import os
import re
import subprocess
import sys

try:
    import colored
except ImportError:
    colored = None


def colorise(color, s):
    return '{}{}{}'.format(colored.fg(color), s, colored.attr('reset'))

# CASE_RUNTIME_COLOUR = 'light_gray'
CASE_RUNTIME_COLOUR = 'grey_42'
AVG_COLOUR = 'chartreuse_2a'
MED_COLOUR = 'chartreuse_3b'
MIN_COLOUR = 'dark_slate_gray_2'
MAX_COLOUR = 'red_1'

def format_run_time_us(run_time):
    if run_time < 1e3:
        return f'{run_time:7.2f}us'
    if run_time < 1e6:
        ms = (run_time / 1e3)
        return f'{ms:7.2f}ms'
    return '{:9.4}s'.format(run_time / 1e6)

def format_run_time(run_time):
    if not run_time.seconds:
        return format_run_time_us(run_time.microseconds)

    us = (run_time.seconds * 1e6) + run_time.microseconds
    return format_run_time_us(us)

def format_freq(hz):
    unit = 'Hz'
    if 1e3 <= hz < 1e6:
        unit = 'kHz'
        hz = (hz / 1e3)
    return '{:7.2f} {}'.format(hz, unit)

ENCODING = 'utf-8'
INTERPRETER = './build/tools/exec/vm'
ASSEMBLER = './build/tools/exec/asm'
DISASSEMBLER = './build/tools/exec/dis'

DIS_EXTENSION = '~'

SKIP_DISASSEMBLER_TESTS = False

EBREAK_LINE_BOXED = re.compile(r'\[(\d+)\.([lap])\] (\*?[a-zA-Z_][a-zA-Z_0-9]*) = (.*)')
EBREAK_LINE_PRIMITIVE = re.compile(r'\[(\d+)\.([lap])\] (is|iu|fl|db) (.*)')
EBREAK_LINE_SPECIAL = re.compile(r'\[(fptr|sbrk)\] (is|iu|fl|db) (.*)')
EBREAK_MEMORY_LINE = re.compile(r'([0-9a-f]{16})  ((?:[0-9a-f]{2} ){16}) \| (.{16})')
PERF_OPS_AND_RUNTIME = re.compile(r'\[vm:perf\] executed ops (\d+), run time (.+)')
PERF_APPROX_FREQ = re.compile(r'\[vm:perf\] approximate frequency (.+ [kMG]?Hz)')

class uint(int):
    def typename(self):
        return 'uint'

class atom:
    def __init__(self, x):
        self._value = '"{}"'.format(repr(x)[1:-1])

    def __str__(self):
        return self._value

    def __repr__(self):
        return repr(self._value)

    def __eq__(self, other):
        return (self._value == other)

    def typename(self):
        return 'atom'

class ref:
    def __init__(self, value):
        self._value = value

    def __str__(self):
        return f'*{type(self._value).__name__}{{{self._value}}}'

    def __repr__(self):
        return repr(self._value)

    def value(self):
        return self._value

    def typename(self):
        return f'*{self.value().typename()}'


class Test_error(Exception):
    pass

class Type_mismatch(Test_error):
    pass

class Value_mismatch(Test_error):
    pass


class Suite_error(Exception):
    pass

class No_check_file_for(Suite_error):
    pass

class Status(enum.Enum):
    Normal = 0
    Skip = 1

def make_local(idx):
    return ('l', idx,)

def make_argument(idx):
    return ('a', idx,)

def make_parameter(idx):
    return ('p', idx,)


def check_register_impl(ebreak, access, expected):
    regset, idx = access
    got_type, got_value = ebreak['registers'][regset][idx]

    want_type, want_value = expected

    if want_type != got_type:
        raise Type_mismatch((want_type, got_type,))
    if want_value != got_value:
        raise Value_mismatch((want_value, got_value,))

def check_register(ebreak, access, expected):
    if type(expected) is int:
        return check_register_impl(
            ebreak,
            access,
            ('is', f'{expected:016x} {expected}',),
        )
    elif type(expected) is uint:
        return check_register_impl(
            ebreak,
            access,
            ('iu', f'{expected:016x} {expected}',),
        )
    elif type(expected) is atom:
        return check_register_impl(
            ebreak,
            access,
            ('atom', expected,),
        )
    elif type(expected) is ref:
        return check_register_impl(
            ebreak,
            access,
            (expected.typename(), str(expected.value()),),
        )
    else:
        t = type(expected).__name__
        raise TypeError(f'invalid value to check for: {t}{{{expected}}}')

def make_ebreak():
    return {
        'registers': {
            'l': {}, # local
            'a': {}, # arguments
            'p': {}, # parameters
        },
    }

EBREAK_BEGIN = re.compile(r'^begin ebreak in process (\[[a-f0-9:]+\])')
EBREAK_END = re.compile(r'^end ebreak in process (\[[a-f0-9:]+\])')
EBREAK_BACKTRACE_ENTRY = re.compile(
       r'^#\d+  (.*?[\.text\+0x[a-f0-9]{8}\]) '
    r'return to (.*?\[\.text\+0x[a-f0-9]{8}\]|null)')
EBREAK_CONTENTS_OF = re.compile(r'^of #(\d+|last)')
EBREAK_SELECT = re.compile(r'^ebreak (-?\d+) in proc(?:ess)? (\[[a-f0-9:]+\])')

EBREAK_LINE_BOXED = re.compile(r'\[(\d+)\.([lap])\] (\*?[a-zA-Z_][a-zA-Z_0-9]*) = (.*)')
EBREAK_LINE_PRIMITIVE = re.compile(r'\[(\d+)\.([lap])\] (is|iu|fl|db|ptr|atom|pid) (.*)')


class Missing_value(Exception):
    @staticmethod
    def to_string():
        return 'missing value'

class Unexpected_type(Exception):
    @staticmethod
    def to_string():
        return 'unexpected type'

class Unexpected_value(Exception):
    @staticmethod
    def to_string():
        return 'unexpected value'

class Bad_ebreak_script(Exception):
    pass

def walk_ebreak_test(errors, want_ebreak, live_ebreak):
    pid = '[{}]'.format(os.environ['VIUA_VM_PID_SEED'])
    frame = -1
    ebreak_index = 0

    for i, line in enumerate(want_ebreak):
        if select := EBREAK_SELECT.match(line):
            if (ebreak_index := select.group(1)) == 'last':
                ebreak_index = -1
            ebreak_index = int(ebreak_index)

            pid = select.group(2)

            if len(live_ebreak[pid]) <= ebreak_index:
                raise Bad_ebreak_script(i + 1)

            frame -1

            continue

        if frame_no := EBREAK_CONTENTS_OF.match(line):
            if (frame := frame_no.group(1)) == 'last':
                frame = -1
            frame = int(frame)
            continue

        if (mem := EBREAK_MEMORY_LINE.match(line)):
            addr, want_bin, want_ascii = mem.groups()

            live_bin = live_ebreak[pid][ebreak_index]['memory'][addr][0]
            live_ascii = live_ebreak[pid][ebreak_index]['memory'][addr][1]
            if want_bin != live_bin:
                leader = f'    memory line {addr}'
                errors.write('{} actual   {} | {}\n'.format(
                    leader,
                    colorise('red', live_bin),
                    colorise('red', live_ascii),
                ))
                errors.write('{} expected {} | {}\n'.format(
                    (len(leader) * ' '),
                    colorise('green', want_bin),
                    colorise('green', want_ascii),
                ))
                raise Unexpected_value()

            continue

        b = EBREAK_LINE_BOXED.match(line)
        p = EBREAK_LINE_PRIMITIVE.match(line)
        if not (b or p):
            continue # FIXME suspicious - maybe an error?

        m = (b or p)
        index = int(m.group(1))
        register_set = m.group(2)
        want_type = m.group(3)
        want_value = m.group(4)

        ebreak = live_ebreak[pid][ebreak_index]['backtrace']
        live_frame = ebreak[frame]

        if index not in live_frame['registers'][register_set]:
            leader = f'    register {index}.{register_set}'
            errors.write(f'{leader} is void\n')
            errors.write('{} expected {} = {}\n'.format(
                (len(leader) * ' '),
                want_type,
                want_value,
            ))
            raise Missing_value()

        live_cell = live_frame['registers'][register_set][index]
        live_type, live_value = live_cell

        if want_type != live_type:
            leader = f'    register {index}.{register_set}'
            errors.write('{} contains {} = {}\n'.format(
                leader,
                colorise('red', live_type.ljust(max(len(want_type), len(live_type)))),
                live_value,
            ))
            errors.write('{} expected {} = {}\n'.format(
                (len(leader) * ' '),
                colorise('green', want_type.ljust(max(len(want_type), len(live_type)))),
                want_value,
            ))
            errors.write('{}          {}\n'.format(
                (len(leader) * ' '),
                colorise('red', (max(len(want_type), len(live_type)) * '^')),
            ))
            raise Unexpected_type()

        if want_value != live_value:
            leader = f'    register {index}.{register_set}'
            errors.write('{} contains {} = {}\n'.format(
                leader,
                live_type.ljust(max(len(want_type), len(live_type))),
                colorise('red', live_value),
            ))
            errors.write('{} expected {} = {}\n'.format(
                (len(leader) * ' '),
                want_type.ljust(max(len(want_type), len(live_type))),
                colorise('green', want_value),
            ))
            raise Unexpected_value()


class Missing_frame_content_open(Exception):
    pass
class Invalid_ebreak_line(Exception):
    pass
def consume_register_contents(ebreak_lines):
    contents = {}

    if ebreak_lines and not EBREAK_CONTENTS_OF.match(ebreak_lines[0]):
        raise Missing_frame_content_open()

    while ebreak_lines and (frame := EBREAK_CONTENTS_OF.match(ebreak_lines[0])):
        frame = frame.group(1)
        frame = (-1 if frame == 'last' else int(frame))
        ebreak_lines.pop(0)

        registers = {
            'l': {},
            'a': {},
            'p': {},
        }

        while ebreak_lines:
            b = EBREAK_LINE_BOXED.match(ebreak_lines[0])
            p = EBREAK_LINE_PRIMITIVE.match(ebreak_lines[0])

            if (m := (b or p)):
                index = int(m.group(1))
                register_set = m.group(2)
                object_type = m.group(3)
                object_value = m.group(4)
                registers[register_set][index] = (object_type, object_value,)

                ebreak_lines.pop(0)
                continue
            elif (m := EBREAK_LINE_SPECIAL.match(ebreak_lines[0])):
                special_register = m.group(1)
                object_type = m.group(2)
                object_value = m.group(3)
                registers[special_register] = (object_type, object_value,)

                ebreak_lines.pop(0)
                continue

            break

        contents[frame] = registers

    return contents

def consume_memory_contents(ebreak_lines):
    contents = {}

    while ebreak_lines and (line := EBREAK_MEMORY_LINE.match(ebreak_lines[0])):
        ebreak_lines.pop(0)
        contents[line.group(1)] = (line.group(2), line.group(3),)

    return contents

def consume_live_ebreak_lines(ebreak_lines):
    ebreak = {
        'pid': None,
        'backtrace': [],
        'memory': {},
    }

    if (pid := EBREAK_BEGIN.match(ebreak_lines[0])):
        ebreak['pid'] = pid.group(1)
        ebreak_lines.pop(0)
    else:
        raise Exception('missing ebreak begin')

    if ebreak_lines[0] == 'backtrace:':
        ebreak_lines.pop(0)

        while (be := EBREAK_BACKTRACE_ENTRY.match(ebreak_lines[0])):
            ret = be.group(2)
            ebreak['backtrace'].append({
                'fn': be.group(1),
                'return': (None if ret == 'null' else ret),
            })
            ebreak_lines.pop(0)
    else:
        raise Exception('invalid ebreak snapshot (expected backtrace)')

    if ebreak_lines[0] == 'register contents:':
        ebreak_lines.pop(0)

        for frame, registers in consume_register_contents(ebreak_lines).items():
            ebreak['backtrace'][frame]['registers'] = registers
    else:
        raise Exception('invalid ebreak snapshot (expected register contents)')

    if ebreak_lines[0] == 'memory:':
        ebreak_lines.pop(0)

        ebreak['memory'] = consume_memory_contents(ebreak_lines)
    else:
        raise Exception('invalid ebreak snapshot (expected memory contents)')

    if (pid := EBREAK_END.match(ebreak_lines[0])):
        ebreak_lines.pop(0)
    else:
        raise Exception('missing ebreak end')

    return ebreak

def run_and_capture(interpreter, executable, args = ()):
    (read_fd, write_fd,) = os.pipe()

    env = dict(os.environ)
    env['VIUA_VM_TRACE_FD'] = str(write_fd)
    proc = subprocess.Popen(
        args = (interpreter,) + (executable,) + args,
        stdout = subprocess.DEVNULL,
        stderr = subprocess.PIPE,
        pass_fds = (write_fd,),
        env = env,
        text = True,
    )
    os.close(write_fd)
    (stdout, stderr) = proc.communicate()
    result = proc.wait()

    buffer = b''
    BUF_SIZE = 4096
    while True:
        chunk = os.read(read_fd, BUF_SIZE)
        if not chunk:
            break
        buffer += chunk

    buffer = buffer.decode(ENCODING)

    lines = list(map(str.strip, buffer.splitlines()))

    ebreaks = []
    abort = None
    i = 0
    while i < len(lines):
        if EBREAK_BEGIN.match(lines[i]):
            ebreak_lines = []
            while i < len(lines):
                ebreak_lines.append(lines[i])
                i += 1

                if EBREAK_END.match(lines[i - 1]):
                    break

            ebreaks.append(consume_live_ebreak_lines(ebreak_lines))

        i += 1

    for line in stderr.splitlines():
        ABORTED = "Aborted: "
        ABORTED_IP = "Aborted IP: "
        ABORTED_IN = "Aborted instruction: "

        if line.startswith(ABORTED):
            abort = {}
            abort["message"] = line[len(ABORTED):].strip()
        if line.startswith(ABORTED_IP):
            abort["ip"] = line[len(ABORTED_IP):].strip()
        if line.startswith(ABORTED_IN):
            abort["instruction"] = line[len(ABORTED_IN):].strip()

    perf = {
        'ops': 0,
        'run_time': None,
        'freq': None,
    }

    for each in lines[::-1]:
        if m := PERF_OPS_AND_RUNTIME.match(each):
            perf['ops'] = int(m.group(1))
            perf['run_time'] = m.group(2)
        elif m := PERF_APPROX_FREQ.match(each):
            perf['freq'] = m.group(1)
        else:
            break

    if ebreaks:
        es = {}
        for each in ebreaks:
            pid = each['pid']
            if pid not in es:
                es[pid] = []

            del each['pid']
            es[pid].append(each)

        ebreaks = es

    return (result, (ebreaks if ebreaks else None), abort, perf,)


CHECK_KINDS = (
    'stdout', # check standard output
    'stderr', # check standard error
    'ebreak', # check ebreak output
    'py',     # use Python script to check test result
    'abort',  # check if aborted at the expected point
)

def detect_check_kind(test_path):
    base_path = os.path.splitext(test_path)[0]

    for ck in CHECK_KINDS:
        if os.path.isfile(f'{base_path}.{ck}'):
            return ck

    raise No_check_file_for(test_path)

def test_case(case_name, test_program, errors):
    if os.path.isfile(skip_file := (os.path.splitext(test_program)[0] +
                                     '.skip')):
        skip_reason : str = None
        with open(skip_file, "r") as stream:
            skip_reason = (stream.read().strip() or "skipped")
        return (Status.Skip, False, skip_reason, None, None,)

    check_kind = None
    try:
        check_kind = detect_check_kind(test_program)
    except No_check_file_for:
        return (Status.Normal, False, 'no check file', None, None,)

    test_executable = (os.path.splitext(test_program)[0] + '.elf')

    start_timepoint = datetime.datetime.now()
    count_runtime = lambda: (datetime.datetime.now() - start_timepoint)

    asm_return = subprocess.call(args = (
        ASSEMBLER,
        '-o',
        test_executable,
        test_program,
    ), stderr = subprocess.DEVNULL, stdout = subprocess.DEVNULL)
    if asm_return != 0:
        return (Status.Normal, False, 'failed to assemble', count_runtime(), None,)

    try:
        result, ebreak, abort_report, perf = run_and_capture(
            INTERPRETER,
            test_executable,
        )
    except Exception as e:
        result = e

    if type(result) is Exception:
        return (Status.Normal, False, 'crashed with Python exception', count_runtime(), None,)
    elif result == -6 and check_kind == "abort":
        pass
    elif result != 0:
        return (Status.Normal, False, f'crashed with non-zero return: {result}', count_runtime(), None,)

    if check_kind == 'ebreak':
        ebreak_dump = (os.path.splitext(test_program)[0] + '.ebreak')
        with open(ebreak_dump, 'r') as ifstream:
            ebreak_dump = ifstream.read().splitlines()

        if not ebreak_dump:
            return (Status.Normal, False, 'empty ebreak file', count_runtime(), None,)

        if ebreak is None:
            return (Status.Normal, False, 'program did not emit ebreak', count_runtime(), None,)

        try:
            walk_ebreak_test(errors, ebreak_dump, ebreak)
        except (Missing_value, Unexpected_type, Unexpected_value,) as e:
            return (Status.Normal, False, e.to_string(), count_runtime(), None,)
        except Bad_ebreak_script as e:
            return (Status.Normal, False, f'bad ebreak script, error on line {e.args[0]}', count_runtime(), None,)
    elif check_kind == 'abort':
        abort_test = (os.path.splitext(test_program)[0] + ".abort")
        with open(abort_test, 'r') as ifstream:
            abort_test = ifstream.read().splitlines()

        if not abort_test:
            return (Status.Normal, False, 'empty abort file', count_runtime(), None,)

        if abort_report is None:
            return (Status.Normal, False, 'program did not abort', count_runtime(), None,)

        if (want_value := abort_test[0]) != (live_value := abort_report["ip"]):
            leader = f'    aborted IP'
            errors.write('{} is {}\n'.format(
                leader,
                live_value,
                colorise('red', live_value),
            ))
            errors.write('{} expected {}\n'.format(
                (len(leader) * ' '),
                want_value,
                colorise('green', want_value),
            ))
            raise Unexpected_value()
        if (want_value := abort_test[1]) != (live_value := abort_report["instruction"]):
            leader = f'    aborted instruction'
            errors.write('{} is {}\n'.format(
                leader,
                live_value,
                colorise('red', live_value),
            ))
            errors.write('{} expected {}\n'.format(
                (len(leader) * ' '),
                want_value,
                colorise('green', want_value),
            ))
            raise Unexpected_value()
        if (want_value := abort_test[2]) != (live_value := abort_report["message"]):
            leader = f'    abort message'
            errors.write('{} is {}\n'.format(
                leader,
                live_value,
                colorise('red', live_value),
            ))
            errors.write('{} expected {}\n'.format(
                (len(leader) * ' '),
                want_value,
                colorise('green', want_value),
            ))
            raise Unexpected_value()


    if SKIP_DISASSEMBLER_TESTS:
        return (Status.Normal, True, None, count_runtime(), perf,)

    test_disassembled_program = test_program + DIS_EXTENSION
    dis_return = subprocess.call(args = (
        DISASSEMBLER,
        '-o',
        test_disassembled_program,
        test_executable,
    ), stderr = subprocess.DEVNULL, stdout = subprocess.DEVNULL)
    if dis_return != 0:
        return (Status.Normal, False, 'failed to disassemble', count_runtime(), None,)

    asm_return = subprocess.call(args = (
        ASSEMBLER,
        '-o',
        test_executable,
        test_disassembled_program,
    ), stderr = subprocess.DEVNULL, stdout = subprocess.DEVNULL)
    if asm_return != 0:
        return (Status.Normal, False, 'failed to reassemble', count_runtime(), None,)

    result, ebreak, abort_report, _ = run_and_capture(
        INTERPRETER,
        test_executable,
    )

    if result == -6 and check_kind == "abort":
        pass
    elif result != 0:
        return (Status.Normal, False, 'crashed after reassembly', count_runtime(), None,)

    if check_kind == 'ebreak':
        ebreak_dump = (os.path.splitext(test_program)[0] + '.ebreak')
        with open(ebreak_dump, 'r') as ifstream:
            ebreak_dump = ifstream.read().splitlines()

        if not ebreak_dump:
            return (Status.Normal, False, 'empty ebreak file', count_runtime(), None,)

        if ebreak is None:
            return (Status.Normal, False, 'program did not emit ebreak', count_runtime(), None,)

        try:
            walk_ebreak_test(errors, ebreak_dump, ebreak)
        except (Missing_value, Unexpected_type, Unexpected_value,) as e:
            return (Status.Normal, False, e.to_string(), count_runtime(), None,)
        except Bad_ebreak_script as e:
            return (Status.Normal, False, f'bad ebreak script, error on line {e.args[0]}', count_runtime(), None,)
    elif check_kind == 'abort':
        abort_test = (os.path.splitext(test_program)[0] + ".abort")
        with open(abort_test, 'r') as ifstream:
            abort_test = ifstream.read().splitlines()

        if not abort_test:
            return (Status.Normal, False, 'empty abort file', count_runtime(), None,)

        if abort_report is None:
            return (Status.Normal, False, 'program did not abort', count_runtime(), None,)

        if (want_value := abort_test[0]) != (live_value := abort_report["ip"]):
            leader = f'    aborted IP'
            errors.write('{} is {}\n'.format(
                leader,
                live_value,
                colorise('red', live_value),
            ))
            errors.write('{} expected {}\n'.format(
                (len(leader) * ' '),
                want_value,
                colorise('green', want_value),
            ))
            raise Unexpected_value()
        if (want_value := abort_test[1]) != (live_value := abort_report["instruction"]):
            leader = f'    aborted instruction'
            errors.write('{} is {}\n'.format(
                leader,
                live_value,
                colorise('red', live_value),
            ))
            errors.write('{} expected {}\n'.format(
                (len(leader) * ' '),
                want_value,
                colorise('green', want_value),
            ))
            raise Unexpected_value()
        if (want_value := abort_test[2]) != (live_value := abort_report["message"]):
            leader = f'    abort message'
            errors.write('{} is {}\n'.format(
                leader,
                live_value,
                colorise('red', live_value),
            ))
            errors.write('{} expected {}\n'.format(
                (len(leader) * ' '),
                want_value,
                colorise('green', want_value),
            ))
            raise Unexpected_value()

    if check_kind == "abort":
        perf = None

    return (Status.Normal, True, None, count_runtime(), perf)


def main(args):
    CASES_DIR = os.environ.get('VIUA_VM_TEST_CASES_DIR', './tests/asm')
    raw_cases = glob.glob(f'{CASES_DIR}/*.asm')
    cases = [
        (
            os.path.split(os.path.splitext(each)[0])[1],
            each,
        )
        for each
        in sorted(raw_cases)
    ]

    print('looking for test programs in: {} (found {} test program{})'.format(
        CASES_DIR,
        (len(cases) or 'no'),
        ('s' if len(cases) != 1 else ''),
    ))

    # Set a known PID seed. This makes test checks much easier to write, as the
    # PID values can be statically determined. Without setting VIUA_VM_PID_SEED
    # the PIDs would be semi-random ie, would start from a randomly generated
    # address in the fe80::/10 subnet.
    #
    # Why fe80::42? Because the fe80::/10 subnet denotes a link-local IPv6
    # address (which is a suitable choice for process IDs, as we don't want them
    # to be globally valid addresses), and 42 is the answer to the question of
    # life, universe, and everything.
    os.environ['VIUA_VM_PID_SEED'] = os.environ.get('VIUA_VM_PID_SEED', 'fe80::42')

    success_cases = 0
    skip_cases = 0
    pad_case_no = len(str(len(cases) + 1))
    pad_case_name = max(map(lambda _: len(_[0]), cases))

    run_times = []
    perf_stats = []

    for case_no, (case_name, test_program,) in enumerate(cases, start = 1):
        error_stream = io.StringIO()

        rc = lambda: test_case(case_name, test_program, error_stream)

        result, symptom, run_time = (False, None, None,)
        tag_color : str = None
        tag : str = None

        if type(result := rc()) is tuple:
            status, result, symptom, run_time, perf = result
            if status == Status.Normal:
                if run_time:
                    run_times.append(run_time)
                if perf:
                    def make_vm_time(s):
                        if s.endswith('us'):
                            return int(s[:-2])
                        elif s.endswith('ms'):
                            return int(float(s[:-2]) * 1000)
                        else:
                            raise
                    vm_time = make_vm_time(perf['run_time'])

                    def make_hz(s):
                        n, hz = s.split()
                        return int(float(n) * {
                            'kHz': 1e3,
                            'MHz': 1e6,
                        }[hz])
                    freq = make_hz(perf['freq'])

                    perf_stats.append((perf['ops'], vm_time, freq,))

                if result:
                    tag = ' ok '
                    tag_color = 'green'
                    success_cases += 1
                else:
                    tag = 'fail'
                    tag_color = 'red'
            elif status == Status.Skip:
                tag = 'skip'
                tag_color = 'yellow'
                skip_cases += 1

        print('  case {}. of {}: [{}] {}  {}'.format(
            colorise('white', str(case_no).rjust(pad_case_no)),
            colorise('white', case_name.ljust(pad_case_name)),
            colorise(tag_color, tag) + ((' => ' + colorise('light_red', symptom)) if symptom else ''),
            (colorise(CASE_RUNTIME_COLOUR, format_run_time(run_time))
                if run_time else ''),
            (('perf: {} ops in {} at {}'.format(
                colorise(CASE_RUNTIME_COLOUR, '{:3}'.format(perf['ops'])),
                colorise(CASE_RUNTIME_COLOUR, perf['run_time'].rjust(6)),
                colorise(CASE_RUNTIME_COLOUR, perf['freq'].rjust(10)),
            )) if (result and perf) else ''),
        ))

        error_stream.seek(0)
        if error_report := error_stream.read(None):
            sys.stderr.write(error_report)
            sys.stderr.write('\n')

    run_color : str = None
    if success_cases == len(cases):
        run_color = "green"
    elif (success_cases + skip_cases) == len(cases):
        run_color = "yellow"
    else:
        run_color = "red"

    print('run {} test case{} with {}% success rate'.format(
        colorise('white', (len(cases) or 'no')),
        ('s' if (len(cases) != 1) else ''),
        colorise(
            run_color,
            '{:5.2f}'.format((success_cases / len(cases)) * 100),
        ),
    ))

    total_run_time = sum(run_times[1:], start=run_times[0])
    print('\ntotal run time was {} ({} ~ {} per case)'.format(
        colorise('white', format_run_time(total_run_time)),
        colorise(MIN_COLOUR, format_run_time(min(run_times)).strip()),
        colorise(MAX_COLOUR, format_run_time(max(run_times)).strip()),
    ))
    run_times = sorted(run_times)
    middle = (len(run_times) // 2)
    print('median run time was {}'.format(
        colorise(MED_COLOUR, format_run_time((
            run_times[middle]
            if (len(run_times) % 2) else
            ((run_times[middle] + run_times[middle + 1]) / 2)
        )).strip()),
    ))

    perf_ops = sorted(map(lambda x: x[0], perf_stats))
    perf_time = sorted(map(lambda x: x[1], perf_stats))
    perf_freq = sorted(map(lambda x: x[2], perf_stats))
    avg = lambda seq: (sum(seq) / len(seq)) if seq else 0
    med_impl = lambda seq: (lambda m: (
            seq[m] if (len(seq) % 2) else (seq[m] + seq[m])
        ))(len(seq) // 2)
    med = lambda seq: med_impl(seq) if seq else 0

    perf_ops_avg = avg(perf_ops)
    perf_ops_med = med(perf_ops)
    perf_ops_min = min(perf_ops)
    perf_ops_max = max(perf_ops)

    perf_time_avg = avg(perf_time)
    perf_time_med = med(perf_time)
    perf_time_min = min(perf_time)
    perf_time_max = max(perf_time)

    perf_freq_avg = avg(perf_freq)
    perf_freq_med = med(perf_freq)
    perf_freq_min = min(perf_freq)
    perf_freq_max = max(perf_freq)

    print('\nperf counter     {}    /  {}     ({} ~ {}):'.format(
        colorise(AVG_COLOUR, 'average'),
        colorise(MED_COLOUR, 'median'),
        colorise(MIN_COLOUR, 'min'.ljust(11)),
        colorise(MAX_COLOUR, 'max'),
    ))
    print('  ops executed: {}     / {}     ({} ~ {})'.format(
        colorise(AVG_COLOUR, '{:7.2f}'.format(avg(perf_ops))),
        colorise(MED_COLOUR, '{:7.2f}'.format(med(perf_ops))),
        colorise('white', f"{min(perf_ops):7.2f}".ljust(11)) if perf_ops else '--',
        colorise('white', f"{max(perf_ops):7.2f}") if perf_ops else '--',
    ))
    print('  VM run time:  {}   / {}   ({} ~ {})'.format(
        colorise(AVG_COLOUR, format_run_time_us(avg(perf_time))),
        colorise(MED_COLOUR, format_run_time_us(med(perf_time))),
        colorise('white', format_run_time_us(min(perf_time)).ljust(11)) if perf_time else '--',
        colorise('white', format_run_time_us(max(perf_time))) if perf_time else '--',
    ))
    print('  VM CPU freq:  {} / {} ({} ~ {})'.format(
        colorise(AVG_COLOUR, format_freq(avg(perf_freq))),
        colorise(MED_COLOUR, format_freq(med(perf_freq))),
        colorise(MIN_COLOUR, format_freq(min(perf_freq)).ljust(11)) if perf_time else '--',
        colorise(MAX_COLOUR, format_freq(max(perf_freq))) if perf_time else '--',
    ))


if __name__ == '__main__':
    main(sys.argv)
