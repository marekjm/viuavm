#!/usr/bin/env python3

import os
import re
import shutil
import sys
import textwrap


DEBUG_LONGEN = False
def longen_line(line, width):
    chunks = line.split()
    length_of_chunks = len(''.join(chunks))
    spaces_to_fill = (width - length_of_chunks)
    no_of_splits = len(chunks) - 1
    spaces_per_split = (spaces_to_fill // (no_of_splits or 1))
    spaces_left = (spaces_to_fill - (spaces_per_split * no_of_splits))
    no_of_double_spaces = spaces_left

    if DEBUG_LONGEN:
        print('---- for line: {}'.format(repr(line)))
        print('length_of_chunks =', length_of_chunks)
        print('spaces_to_fill =', spaces_to_fill)
        print('no_of_splits =', no_of_splits)
        print('spaces_per_split =', spaces_per_split)
        print('spaces_left =', spaces_left)

    new_line = [chunks[0]]

    for each in chunks[1:]:
        if no_of_double_spaces:
            new_line.append('  ')
            no_of_double_spaces -= 1
        else:
            new_line.append(' ')
        new_line.append(each)

    new_line = ''.join(new_line)
    if DEBUG_LONGEN:
        new_line = '[{}] {}'.format(len(new_line), new_line)
    return new_line

def longen(lines, width):
    return [longen_line(each, width) for each in lines]


def stringify_encoding(encoding):
    stringified = []

    size_so_far = 0
    for each in encoding:
        if each == '@opcode':
            head = '{} ... {}'.format(size_so_far, size_so_far + 7)
            size_so_far += 8
            stringified.append(
                (head, 'OP',),
            )
        elif each == '@register':
            head = '{} ... {}'.format(size_so_far, size_so_far + 7)
            size_so_far += 8
            stringified.append(
                (head, 'AS',),
            )

            head = '{} ... {}'.format(size_so_far, size_so_far + 7)
            size_so_far += 8
            stringified.append(
                (head, 'RS',),
            )

    max_lengths = list(map(lambda pair: max(len(pair[0]), len(pair[1])), stringified))
    heads = []
    bodies = []
    for i, each in enumerate(stringified):
        head, body = each
        heads.append(head.center(max_lengths[i]))
        bodies.append(body.center(max_lengths[i]))

    return (
        size_so_far,
        '| {} |'.format(' | '.join(heads)),
        '| {} |'.format(' | '.join(bodies))
    )


def main(args):
    documented_opcodes = sorted(os.listdir('./opcodes'))

    first_opcode_being_documented = True

    for each in args:
        if each not in documented_opcodes:
            sys.stderr.write('no documentation for {} opcode\n'.format(repr(each)))
            return 1

    for each in (args or documented_opcodes):
        groups = []
        with open(os.path.join('.', 'opcodes', each, 'groups')) as ifstream:
            groups = ifstream.read().splitlines()
        if not groups:
            groups = [each]

        syntax = []
        with open(os.path.join('.', 'opcodes', each, 'syntax')) as ifstream:
            syntax = ifstream.read().splitlines()

        description = ''
        with open(os.path.join('.', 'opcodes', each, 'description')) as ifstream:
            description = ifstream.read().strip()

        # encoding = []
        # with open(os.path.join('.', 'opcodes', each, 'encoding')) as ifstream:
        #     encoding = ifstream.read().splitlines()

        exceptions = []
        try:
            for each_ex in os.listdir(os.path.join('.', 'opcodes', each, 'exceptions')):
                with open(os.path.join('.', 'opcodes', each, 'exceptions', each_ex)) as ifstream:
                    exceptions.append( (each_ex, ifstream.read().strip(),) )
        except FileNotFoundError:
            sys.stderr.write('no exceptions defined for "{}" instruction'.format(each))

        examples = []
        try:
            for each_ex in os.listdir(os.path.join('.', 'opcodes', each, 'examples')):
                with open(os.path.join('.', 'opcodes', each, 'examples', each_ex)) as ifstream:
                    examples.append( (each_ex, ifstream.read().strip(),) )
        except FileNotFoundError:
            sys.stderr.write('no examples defined for "{}" instruction'.format(each))

        remarks = ''
        with open(os.path.join('.', 'opcodes', each, 'remarks')) as ifstream:
            remarks = (ifstream.read().strip() or 'None.')

        see_also = []
        with open(os.path.join('.', 'opcodes', each, 'see_also')) as ifstream:
            see_also = ifstream.read().splitlines()


        if first_opcode_being_documented:
            first_opcode_being_documented = False
        else:
            print()
            print('-' * 42)
            print()


        print('{}'.format(each.upper()))
        print('    in group{}: {}'.format(
            ('' if len(groups) == 1 else 's'),
            ', '.join(groups)
        ))
        print()

        print('  SYNTAX')
        for i, syn in enumerate(syntax):
            print('    ({})    {}'.format(i, syn))
        print()

        print('  DESCRIPTION')
        print(textwrap.indent(
            text = '\n'.join(longen(textwrap.wrap(description, width=66), width=66)).strip(),
            prefix = '    ',
        ))
        print()

        print('  EXCEPTIONS')
        if exceptions:
            print()
            for each_ex in exceptions:
                print('    {}'.format(each_ex))
                print(textwrap.indent(
                    text = '\n'.join(longen(textwrap.wrap(remarks, width=64), width=64)).strip(),
                    prefix = '      ',
                ))
                print()
        else:
            print('    None.')
            print()

        print('  EXAMPLES')
        if examples:
            print()
            for each_ex in examples:
                print(textwrap.indent(
                    each_ex[1],
                    prefix = '    ',
                ))
        else:
            print('    None.')
        print()

        # print('  ENCODING')
        # instruction_size, encoding_header, encoding_body = stringify_encoding(encoding)
        # print('    on {} bits'.format(instruction_size))
        # print()
        # print('    MSB{}LSB'.format(' ' * (len(encoding_header) - 6)))
        # print('    {}'.format(encoding_header))
        # print('    {}'.format(encoding_body))
        # print()
        # print('    OP: opcode')
        # print('    AS: access specifier')
        # print('    RS: register set type')
        # print()

        print('  REMARKS')
        print(textwrap.indent(
            text = '\n'.join(longen(textwrap.wrap(remarks, width=66), width=66)).strip(),
            prefix = '    ',
        ))
        print()

        if see_also:
            print('  SEE ALSO')
            print('    {}'.format(', '.join(see_also)))

    return 0


exit(main(sys.argv[1:]))
