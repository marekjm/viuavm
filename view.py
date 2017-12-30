#!/usr/bin/env python3

import os
import re
import shutil
import sys
import textwrap

try:
    import colored
except ImportError:
    colored = None


COLOR_OPCODE = 'white'
COLOR_SECTION = 'red'
COLOR_SYNTAX_SAMPLE_INDEX = 'cyan'
COLOR_SYNTAX_SAMPLE = 'green'

def colorise(text, color):
    if colored is None or os.environ.get('COLOR') == 'no':
        return str(text)
    return (colored.fg(color) + str(text) + colored.attr('reset'))


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


LINE_WIDTH = 80


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


def into_paragraphs(text):
    lines = text.split('\n')
    paragraphs = []
    para = []

    for each in lines:
        if not each:
            # If the last paragraph is also empty, push only one paragraph-break.
            if paragraphs and not paragraphs[-1] and not para:
                continue

            # Append the paragraph, and the paragraph-break after it.
            # Empty lines on their own introduce paragraph breaks.
            # Use \break on its own to introduce line-break without an empty line.
            paragraphs.append(para)
            paragraphs.append([])

            para = []
            continue
        if each == r'\break':
            paragraphs.append(para)
            para = []
            continue
        para.append(each)
    if para:
        paragraphs.append(para)

    return ['\n'.join(each) for each in paragraphs]


class InvalidReference(Exception):
    pass

def parse_and_expand(text, syntax):
    expanded_text = text
    reg = re.compile(r'\\syntax{(\d+)}')
    found_syntax_refs = re.findall(reg, text)
    for i in found_syntax_refs:
        if int(i) >= len(syntax):
            raise InvalidReference('invalid syntax reference: \\syntax{{{}}}\n'.format(i))
        expanded_text = expanded_text.replace((r'\syntax{' + i + '}'), syntax[int(i)])
    return expanded_text


def render_free_form_text(source, indent = 4):
    paragraphs = into_paragraphs(source)
    for each in paragraphs:
        print(textwrap.indent(
            text = '\n'.join(longen(textwrap.wrap(each, width=(LINE_WIDTH - indent)), width=(LINE_WIDTH -
                indent))).strip(),
            prefix = (' ' * indent),
        ))

def render_file(path):
    source = ''
    with open(path) as ifstream:
        source = ifstream.read().strip()
    return render_free_form_text(source)

def render_section(section):
    return render_file(os.path.join('.', 'sections', section))


def main(args):
    # See if the user requested documentation for a specific group of instructions.
    selected_group = None
    if len(args) == 1 and args[0][-1] == ':':
        selected_group = args[0][:-1]
        args = []


    # Load a list of documented opcodes, and
    # check if all requested instructions are documented.
    documented_opcodes = sorted(os.listdir('./opcodes'))
    for each in args:
        if each not in documented_opcodes:
            sys.stderr.write('no documentation for {} opcode\n'.format(repr(each)))
            return 1


    # Print introduction, but only if the user requested full documentation.
    # If the user requested docs only for a specific group of instructions, or
    # a list of instructions do not print the introduction.
    # It looks like the user knows what they want anyway.
    if (not args) and selected_group is None:
        print('VIUA VM OPCODES DOCUMENTATION'.center(LINE_WIDTH))
        print()

        introduction = ''
        with open('./introduction') as ifstream:
            introduction = ifstream.read().strip()
        if introduction:
            print('  INTRODUCTION')
            print()
            render_free_form_text(introduction)
            print()
            print('-' * LINE_WIDTH)
        print()


    # Render documentation for all requested instructions.
    # If no instructions were explicitly requested then print the full documentation.
    first_opcode_being_documented = True
    for each in (args or documented_opcodes):
        # Instructions are grouped into groups.
        # Every instruction is in at least 1 group.
        # If an instruction does not have any explicitly assigned groups then
        # a group is created for it.
        groups = []
        with open(os.path.join('.', 'opcodes', each, 'groups')) as ifstream:
            groups = ifstream.read().splitlines()
        if not groups:
            groups = [each]


        # If the user requested a group documentation and current instruction does
        # not belong to the requested group - skip it.
        if selected_group is not None and selected_group not in groups:
            continue


        # Every instruction should come with at least one syntax sample.
        syntax = []
        with open(os.path.join('.', 'opcodes', each, 'syntax')) as ifstream:
            syntax = ifstream.read().splitlines()


        # Every instruction should be described.
        # If it's not - how are we to know what does it do?
        description = ''
        with open(os.path.join('.', 'opcodes', each, 'description')) as ifstream:
            description = ifstream.read().strip()
        description = into_paragraphs(description)


        # encoding = []
        # with open(os.path.join('.', 'opcodes', each, 'encoding')) as ifstream:
        #     encoding = ifstream.read().splitlines()


        # An exception may (or may not) throw exceptions.
        # As an example, "checkedsmul" will throw an exception if the arithmetic operation
        # would overflow.
        #
        # Only explicitly listed exceptions are printed here.
        # "Default" exceptions (e.g. access to register out of range for selected register set,
        # read from an empty register) are not printed here.
        exceptions = []
        try:
            for each_ex in os.listdir(os.path.join('.', 'opcodes', each, 'exceptions')):
                with open(os.path.join('.', 'opcodes', each, 'exceptions', each_ex)) as ifstream:
                    exceptions.append( (each_ex, ifstream.read().strip(),) )
        except FileNotFoundError:
            sys.stderr.write('no exceptions defined for "{}" instruction\n'.format(each))


        # Print any examples provided for this instruction.
        examples = []
        try:
            for each_ex in os.listdir(os.path.join('.', 'opcodes', each, 'examples')):
                with open(os.path.join('.', 'opcodes', each, 'examples', each_ex)) as ifstream:
                    examples.append( (each_ex, ifstream.read().strip(),) )
        except FileNotFoundError:
            sys.stderr.write('no examples defined for "{}" instruction\n'.format(each))


        # Apart from a description, instruction may come with a list of "remarks".
        # These are additional notes, describin peculiarities of an instruction, its differences from
        # other instructions (and its relations with them).
        # Anything that does not fit the "description" field is put here.
        remarks = ''
        with open(os.path.join('.', 'opcodes', each, 'remarks')) as ifstream:
            remarks = (ifstream.read().strip() or 'None.')
        remarks = into_paragraphs(remarks)


        # Any other instructions that are related to the currently rendered instruction.
        see_also = []
        with open(os.path.join('.', 'opcodes', each, 'see_also')) as ifstream:
            see_also = ifstream.read().splitlines()


        # Instructions should be separated by a '------' line.
        # This will make the documentation more readable.
        if first_opcode_being_documented:
            first_opcode_being_documented = False
        else:
            print()
            print('-' * LINE_WIDTH)
            print()


        print('{}'.format(colorise(each.upper(), COLOR_OPCODE)))
        print('    in group{}: {}'.format(
            ('' if len(groups) == 1 else 's'),
            ', '.join(groups)
        ))
        print()


        print('  {}'.format(colorise('SYNTAX', COLOR_SECTION)))
        for i, syn in enumerate(syntax):
            print('    ({})    {}'.format(colorise(i, COLOR_SYNTAX_SAMPLE_INDEX), colorise(syn, COLOR_SYNTAX_SAMPLE)))
        print()


        print('  {}'.format(colorise('DESCRIPTION', COLOR_SECTION)))
        indent = 4
        for each_paragraph in description:
            print(parse_and_expand(textwrap.indent(
                text = '\n'.join(longen(textwrap.wrap(each_paragraph, width=(LINE_WIDTH - indent)),
                    width=(LINE_WIDTH - indent))).strip(),
                prefix = (' ' * indent),
            ),
            syntax = syntax,
            ))
        print()


        print('  {}'.format(colorise('EXCEPTIONS', COLOR_SECTION)))
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


        print('  {}'.format(colorise('EXAMPLES', COLOR_SECTION)))
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


        print('  {}'.format(colorise('REMARKS', COLOR_SECTION)))
        for each_paragraph in remarks:
            print(parse_and_expand(textwrap.indent(
                text = '\n'.join(longen(textwrap.wrap(each_paragraph, width=(LINE_WIDTH - indent)),
                    width=(LINE_WIDTH - indent))).strip(),
                prefix = (' ' * indent),
            ),
            syntax = syntax
            ))
        print()


        if see_also:
            print('  {}'.format(colorise('SEE ALSO', COLOR_SECTION)))
            print('    {}'.format(', '.join(see_also)))

    return 0


exit(main(sys.argv[1:]))
