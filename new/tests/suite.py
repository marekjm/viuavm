#!/usr/bin/env python3

import datetime
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
        return f'{run_time:.2f}us'
    if run_time < 1e6:
        ms = (run_time / 1e3)
        return f'{ms:6.2f}ms'
    return '{:.4}s'.format(run_time / 1e6)

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
    return '{:.2f} {}'.format(hz, unit)

ENCODING = 'utf-8'
INTERPRETER = './build/tools/exec/vm'
ASSEMBLER = './build/tools/exec/asm'
DISASSEMBLER = './build/tools/exec/dis'

DIS_EXTENSION = '~'

SKIP_DISASSEMBLER_TESTS = False

EBREAK_LINE_BOXED = re.compile(r'\[(\d+)\.([lap])\] (\*?[a-zA-Z_][a-zA-Z_0-9]*) = (.*)')
EBREAK_LINE_PRIMITIVE = re.compile(r'\[(\d+)\.([lap])\] (is|iu|fl|db) (.*)')
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

def load_ebreak_line(ebreak, line):
    mb = EBREAK_LINE_BOXED.match(line)
    mp = EBREAK_LINE_PRIMITIVE.match(line)
    if not (mb or mp):
        return False

    m = (mb or mp)

    index = m.group(1)
    regset = m.group(2)
    type_of = m.group(3)
    value_of = m.group(4)

    ebreak['registers'][regset][int(index)] = (type_of, value_of,)

    return True

def run_and_capture(interpreter, executable, args = ()):
    (read_fd, write_fd,) = os.pipe()

    env = dict(os.environ)
    env['VIUA_VM_TRACE_FD'] = str(write_fd)
    proc = subprocess.Popen(
        args = (interpreter,) + (executable,) + args,
        stdout = subprocess.DEVNULL,
        stderr = subprocess.DEVNULL,
        pass_fds = (write_fd,),
        env = env,
    )
    os.close(write_fd)
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
    i = 0
    while i < len(lines):
        line = lines[i]
        if line == 'ebreak':
            i += 1

            ebreak = make_ebreak()

            while True:
                if not load_ebreak_line(ebreak, lines[i]):
                    break
                i += 1

            ebreaks.append(ebreak)

        i += 1

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

    return (result, (ebreaks[-1] if ebreaks else None), perf,)


CHECK_KINDS = (
    'stdout', # check standard output
    'stderr', # check standard error
    'ebreak', # check ebreak output
    'py',     # use Python script to check test result
)

def detect_check_kind(test_path):
    base_path = os.path.splitext(test_path)[0]

    for ck in CHECK_KINDS:
        if os.path.isfile(f'{base_path}.{ck}'):
            return ck

    raise No_check_file_for(test_path)

def test_case(case_name, test_program, errors):
    check_kind = None
    try:
        check_kind = detect_check_kind(test_program)
    except No_check_file_for:
        return (False, 'no check file', None, None,)

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
        return (False, 'failed to assemble', count_runtime(), None,)

    result, ebreak, perf = run_and_capture(
        INTERPRETER,
        test_executable,
    )

    if result != 0:
        return (False, 'crashed', count_runtime(), None,)

    if check_kind == 'ebreak':
        ebreak_dump = (os.path.splitext(test_program)[0] + '.ebreak')
        with open(ebreak_dump, 'r') as ifstream:
            ebreak_dump = ifstream.readlines()

        want_ebreak = make_ebreak()

        for line in ebreak_dump:
            if not load_ebreak_line(want_ebreak, line):
                errors.write(f'    invalid-want ebreak line: {line}')
                return (False, None, count_runtime(), None,)
        if not ebreak_dump:
            return (False, 'empty ebreak file', count_runtime(), None,)

        if ebreak is None:
            return (False, 'no ebreak dump', count_runtime(), None,)

        for r, content in want_ebreak['registers'].items():
            for index, cell in content.items():
                if index not in ebreak['registers'][r]:
                    leader = f'    register {index}.{r}'
                    errors.write(f'{leader} is void\n')
                    errors.write('{} expected {} = {}\n'.format(
                        (len(leader) * ' '),
                        *cell
                    ))
                    return (False, None, count_runtime(), None,)

                got = ebreak['registers'][r][index]
                got_type, got_value = got

                want_type, want_value = cell

                if want_type != got_type:
                    leader = f'    register {index}.{r}'
                    errors.write('{} contains {} = {}\n'.format(
                        leader,
                        colorise('red', got_type.ljust(max(len(want_type), len(got_type)))),
                        got_value,
                    ))
                    errors.write('{} expected {} = {}\n'.format(
                        (len(leader) * ' '),
                        colorise('green', want_type.ljust(max(len(want_type), len(got_type)))),
                        want_value,
                    ))
                    errors.write('{}          {}\n'.format(
                        (len(leader) * ' '),
                        colorise('red', (max(len(want_type), len(got_type)) * '^')),
                    ))
                    return (False, 'unexpected type', count_runtime(), None,)

                if want_value != got_value:
                    leader = f'    register {index}.{r}'
                    errors.write('{} contains {} = {}\n'.format(
                        leader,
                        got_type.ljust(max(len(want_type), len(got_type))),
                        colorise('red', got_value),
                    ))
                    errors.write('{} expected {} = {}\n'.format(
                        (len(leader) * ' '),
                        want_type.ljust(max(len(want_type), len(got_type))),
                        colorise('green', want_value),
                    ))
                    return (False, 'unexpected value', count_runtime(), None,)

    if SKIP_DISASSEMBLER_TESTS:
        return (True, None, count_runtime(), perf,)

    test_disassembled_program = test_program + DIS_EXTENSION
    dis_return = subprocess.call(args = (
        DISASSEMBLER,
        '-o',
        test_disassembled_program,
        test_executable,
    ), stderr = subprocess.DEVNULL, stdout = subprocess.DEVNULL)
    if dis_return != 0:
        return (False, 'failed to disassemble', count_runtime(), None,)

    asm_return = subprocess.call(args = (
        ASSEMBLER,
        '-o',
        test_executable,
        test_disassembled_program,
    ), stderr = subprocess.DEVNULL, stdout = subprocess.DEVNULL)
    if asm_return != 0:
        return (False, 'failed to reassemble', count_runtime(), None,)

    result, ebreak, _ = run_and_capture(
        INTERPRETER,
        test_executable,
    )

    if result != 0:
        return (False, 'crashed after reassembly', count_runtime(), None,)

    if check_kind == 'ebreak':
        ebreak_dump = (os.path.splitext(test_program)[0] + '.ebreak')
        with open(ebreak_dump, 'r') as ifstream:
            ebreak_dump = ifstream.readlines()

        want_ebreak = make_ebreak()

        for line in ebreak_dump:
            if not load_ebreak_line(want_ebreak, line):
                errors.write(f'    invalid-want ebreak line: {line}')
                return (False, None, count_runtime(), None,)
        if not ebreak_dump:
            return (False, 'empty ebreak file', count_runtime(), None,)

        for r, content in want_ebreak['registers'].items():
            for index, cell in content.items():
                if index not in ebreak['registers'][r]:
                    leader = f'    register {index}.{r}'
                    errors.write(f'{leader} is void\n')
                    errors.write('{} expected {} = {}\n'.format(
                        (len(leader) * ' '),
                        *cell
                    ))
                    return (False, None, count_runtime(),)

                got = ebreak['registers'][r][index]
                got_type, got_value = got

                want_type, want_value = cell

                if want_type != got_type:
                    leader = f'    register {index}.{r}'
                    errors.write('{} contains {} = {}\n'.format(
                        leader,
                        colorise('red', got_type.ljust(max(len(want_type), len(got_type)))),
                        got_value,
                    ))
                    errors.write('{} expected {} = {}\n'.format(
                        (len(leader) * ' '),
                        colorise('green', want_type.ljust(max(len(want_type), len(got_type)))),
                        want_value,
                    ))
                    errors.write('{}          {}\n'.format(
                        (len(leader) * ' '),
                        colorise('red', (max(len(want_type), len(got_type)) * '^')),
                    ))
                    return (False, 'unexpected type (after reassembly)', count_runtime(), None,)

                if want_value != got_value:
                    leader = f'    register {index}.{r}'
                    errors.write('{} contains {} = {}\n'.format(
                        leader,
                        got_type.ljust(max(len(want_type), len(got_type))),
                        colorise('red', got_value),
                    ))
                    errors.write('{} expected {} = {}\n'.format(
                        (len(leader) * ' '),
                        want_type.ljust(max(len(want_type), len(got_type))),
                        colorise('green', want_value),
                    ))
                    return (False, 'unexpected value (after reassembly)', count_runtime(), None,)

    return (True, None, count_runtime(), perf)


def main(args):
    CASES_DIR = os.environ.get('VIUAVM_TEST_CASES_DIR', './tests/asm')

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
    pad_case_no = len(str(len(cases) + 1))
    pad_case_name = max(map(lambda _: len(_[0]), cases))

    run_times = []
    perf_stats = []

    for case_no, (case_name, test_program,) in enumerate(cases, start = 1):
        error_stream = io.StringIO()

        rc = lambda: test_case(case_name, test_program, error_stream)

        result, symptom, run_time = (False, None, None,)
        if type(result := rc()) is tuple:
            result, symptom, run_time, perf = result
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
            success_cases += 1

        print('  case {}. of {}: [{}] {}  {}'.format(
            colorise('white', str(case_no).rjust(pad_case_no)),
            colorise('white', case_name.ljust(pad_case_name)),
            colorise(
                ('green' if result else 'red'),
                (' ok ' if result else 'fail'),
            ) + ((' => ' + colorise('light_red', symptom)) if symptom else ''),
            (colorise(CASE_RUNTIME_COLOUR, format_run_time(run_time))
                if run_time else ''),
            (('perf: {} ops in {} at {}'.format(
                colorise(CASE_RUNTIME_COLOUR, '{:3}'.format(perf['ops'])),
                colorise(CASE_RUNTIME_COLOUR, perf['run_time'].rjust(6)),
                colorise(CASE_RUNTIME_COLOUR, perf['freq'].rjust(10)),
            )) if result else ''),
        ))

        error_stream.seek(0)
        if error_report := error_stream.read(None):
            sys.stderr.write(error_report)
            sys.stderr.write('\n')

    print('run {} test case{} with {}% success rate'.format(
        colorise('white', (len(cases) or 'no')),
        ('s' if (len(cases) != 1) else ''),
        colorise(
            ('green' if (success_cases == len(cases)) else 'red'),
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
    print('\nperf counter     {}    / {}     ({} ~ {}):'.format(
        colorise(AVG_COLOUR, 'average'),
        colorise(MED_COLOUR, 'median'),
        colorise(MIN_COLOUR, 'min'),
        colorise(MAX_COLOUR, 'max'),
    ))
    print('  ops executed:  {}     / {}     ({} ~ {})'.format(
        colorise(AVG_COLOUR, '{:.2f}'.format(avg(perf_ops)).rjust(6)),
        colorise(MED_COLOUR, '{:.2f}'.format(med(perf_ops)).rjust(6)),
        colorise('white', min(perf_ops)) if perf_ops else '--',
        colorise('white', max(perf_ops)) if perf_ops else '--',
    ))
    print('  VM run time:   {}   / {}   ({} ~ {})'.format(
        colorise(AVG_COLOUR, format_run_time_us(avg(perf_time)).rjust(8)),
        colorise(MED_COLOUR, format_run_time_us(med(perf_time)).rjust(8)),
        colorise('white', format_run_time_us(min(perf_time))) if perf_time else '--',
        colorise('white', format_run_time_us(max(perf_time))) if perf_time else '--',
    ))
    print('  VM CPU freq:   {} / {} ({} ~ {})'.format(
        colorise(AVG_COLOUR, format_freq(avg(perf_freq))),
        colorise(MED_COLOUR, format_freq(med(perf_freq))),
        colorise(MIN_COLOUR, format_freq(min(perf_freq))) if perf_time else '--',
        colorise(MAX_COLOUR, format_freq(max(perf_freq))) if perf_time else '--',
    ))


if __name__ == '__main__':
    main(sys.argv)
