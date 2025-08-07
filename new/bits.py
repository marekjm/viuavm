#!/usr/bin/env python3

import dataclasses
import sys


@dataclasses.dataclass
class Specification:
    width: int
    fields: tuple[tuple[int, str | None]]


def emit(specification):
    bit_indexes = ""
    horizontal_lines = "+"
    top_line = "┌"
    bot_line = "└"
    labelled_fields = "│"

    top_separator = "┼"
    mid_separator = "│"
    bot_separator = "┴"
    top_bar = "─"
    bot_bar = "─"

    current_bit = specification.width
    span_separator = " "

    for field in specification.fields:
        width, label = field

        highest_bit = current_bit - 1
        lowest_bit = highest_bit - width + 1

        if min(highest_bit, lowest_bit) < 0:
            raise Exception("too many fields, not enough bits")

        if highest_bit == lowest_bit:
            span_label = f" {highest_bit} "
        else:
            span_label = f"{highest_bit} {lowest_bit}"

        if label is None:
            name_label = " " * len(span_label)
        else:
            name_label = f" {label} "
            nl = len(name_label)
            sl = len(span_label)
            if nl == sl:
                pass
            elif nl > sl:
                if highest_bit == lowest_bit:
                    span_label = f"{highest_bit}".center(nl)
                else:
                    left_side = len(str(highest_bit))
                    right_side = len(str(lowest_bit))
                    filler = " " * (nl - left_side - right_side)
                    span_label = f"{highest_bit}{filler}{lowest_bit}"
            elif nl < sl:
                name_label = (label or "").center(sl)

        bit_indexes += span_separator + span_label
        labelled_fields += name_label + mid_separator
        top_line += (top_bar * len(name_label)) + top_separator
        bot_line += (bot_bar * len(name_label)) + bot_separator

        span_separator = mid_separator
        current_bit = lowest_bit

    top_line = top_line[:-1]
    top_line += "┐"

    bot_line = bot_line[:-1]
    bot_line += "┘"

    return (bit_indexes, top_line, labelled_fields.replace("`", " "), bot_line,)


GENERIC_INSTRUCTION = Specification(
    width = 64,
    fields = (
        (48, "``````````workload`````````",),
        (16, "opcode",),
    ),
)
print("\n".join(emit(GENERIC_INSTRUCTION)))

T = Specification(
    width = 64,
    fields = (
        (24, "`````````",),
        (8, "snd",),
        (8, "src",),
        (8, "dst",),
        (16, "opcode",),
    ),
)
print()
print("\n".join(emit(T)))

D = Specification(
    width = 64,
    fields = (
        (32, "```````````````",),
        (8, "src",),
        (8, "dst",),
        (16, "opcode",),
    ),
)
print()
print("\n".join(emit(D)))

S = Specification(
    width = 64,
    fields = (
        (32, "`````````````````````",),
        (8, "rgr",),
        (16, "opcode",),
    ),
)
print()
print("\n".join(emit(S)))

M = Specification(
    width = 64,
    fields = (
        (32, "````offset`````",),
        (8, "src",),
        (8, "dst",),
        (16, "opcode",),
    ),
)
print()
print("\n".join(emit(M)))

I = Specification(
    width = 64,
    fields = (
        (32, "```immediate```",),
        (8, "```",),
        (8, "dst",),
        (16, "opcode",),
    ),
)
print()
print("\n".join(emit(I)))

U = Specification(
    width = 64,
    fields = (
        (32, "```immediate```",),
        (8, "src",),
        (8, "dst",),
        (16, "opcode",),
    ),
)
print()
print("\n".join(emit(U)))


REGISTER_ACCESS = Specification(
    width = 8,
    fields = (
        (2, "set",),
        (6, "index",),
    ),
)
print()
print("\n".join(emit(REGISTER_ACCESS)))


OPCODE = Specification(
    width = 16,
    fields = (
        (3, "`fmt`",),
        (3, "`flg`",),
        (1, "u",),
        (9, "```operation````",),
    ),
)
print()
print("\n".join(emit(OPCODE)))
