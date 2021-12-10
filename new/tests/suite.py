#!/usr/bin/env python3

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


ENCODING = 'utf-8'
INTERPRETER = './build/tools/exec/vm'
ASSEMBLER = './build/tools/exec/asm'

EBREAK_LINE_BOXED = re.compile(r'\[(\d+)\.([lap])\] (\*?[a-zA-Z_][a-zA-Z_0-9]*) = (.*)')
EBREAK_LINE_PRIMITIVE = re.compile(r'\[(\d+)\.([lap])\] (is|iu|ft|db) (.*)')

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

    return (result, (ebreaks[-1] if ebreaks else None),)

result, ebreaks = run_and_capture(
    INTERPRETER,
    './a.out',
)

# check_register(ebreaks[0], make_local(4), uint(41))
# check_register(ebreaks[0], make_local(2), atom('answer'))
# check_register(ebreaks[0], make_local(5), ref(uint(41)))


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
        return (False, 'no check file',)

    test_executable = (os.path.splitext(test_program)[0] + '.bin')

    asm_return = subprocess.call(args = (
        ASSEMBLER,
        '-o',
        test_executable,
        test_program,
    ), stderr = subprocess.DEVNULL, stdout = subprocess.DEVNULL)
    if asm_return != 0:
        errors.write(f'case {case_name} failed to assemble\n')
        return (False, 'failed to assemble',)

    result, ebreak = run_and_capture(
        INTERPRETER,
        test_executable,
    )

    if result != 0:
        return (False, 'crashed',)

    if check_kind == 'ebreak':
        ebreak_dump = (os.path.splitext(test_program)[0] + '.ebreak')
        with open(ebreak_dump, 'r') as ifstream:
            ebreak_dump = ifstream.readlines()

        want_ebreak = make_ebreak()
        for line in ebreak_dump:
            if not load_ebreak_line(want_ebreak, line):
                errors.write(f'    invalid-want ebreak line: {line}')
                return False

        for r, content in want_ebreak['registers'].items():
            for index, cell in content.items():
                if index not in ebreak['registers'][r]:
                    leader = f'    register {index}.{r}'
                    errors.write(f'{leader} is void\n')
                    errors.write('{} expected {} = {}\n'.format(
                        (len(leader) * ' '),
                        *cell
                    ))
                    return False

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
                    return False

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
                    return False

    return True


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

    success_cases = 0
    pad_case_no = len(str(len(cases) + 1))
    pad_case_name = max(map(lambda _: len(_[0]), cases))

    for case_no, (case_name, test_program,) in enumerate(cases, start = 1):
        error_stream = io.StringIO()

        rc = lambda: test_case(case_name, test_program, error_stream)

        result, symptom = (False, None,)
        if type(result := rc()) is tuple:
            result, symptom = result

        if result:
            success_cases += 1

        print('  case {}. of {}: [{}]'.format(
            colorise('white', str(case_no).rjust(pad_case_no)),
            colorise('white', case_name.ljust(pad_case_name)),
            colorise(
                ('green' if result else 'red'),
                (' ok ' if result else 'fail'),
            ) + ((' => ' + colorise('light_red', symptom)) if symptom else ''),
        ))

        error_stream.seek(0)
        if error_report := error_stream.read(None):
            sys.stderr.write(error_report)
            sys.stderr.write('\n')

    print('run {} test case{} with {}% success rate'.format(
        colorise('white', (len(cases) or 'no')),
        ('s' if (len(cases) != 1) else ''),
        colorise(
            ('green' if (success_cases == len(cases)) else 'white'),
            '{:5.2f}'.format((success_cases / len(cases)) * 100),
        ),
    ))


if __name__ == '__main__':
    main(sys.argv)
