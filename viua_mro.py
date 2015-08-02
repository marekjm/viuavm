#!/usr/bin/env python3

"""Prototype implementation of machine's Method Resolution Order algorithm.
"""


def basesOf(klass, bases):
    bases_of_klass = (bases[klass] if klass in bases else [])
    i = 0
    while i < len(bases_of_klass):
        bases_of_klass.extend(basesOf(bases_of_klass[i], bases))
        i += 1
    return bases_of_klass


def linearize(klass, bases):
    linearized_bases = []
    bases_of_klass = basesOf(klass, bases)
    i = 0
    while i < len(bases_of_klass):
        element = bases_of_klass[i]
        if element in linearized_bases: linearized_bases.remove(element)
        linearized_bases.append(element)
        i += 1
    linearized_bases.insert(0, klass)
    return linearized_bases


bases_z = {
    'O': [],
    'A': ['O'],
    'B': ['O'],
    'C': ['O'],
    'D': ['O'],
    'E': ['O'],
    'K1': ['A', 'B', 'C'],
    'K2': ['D', 'B', 'E'],
    'K3': ['D', 'A'],
    'Z': ['K1', 'K2', 'K3'],
}
# print(basesOf('Z', bases_z))
# print(linearize('Z', bases_z))


bases_dylan = {
    'pane': ['object'],
    'scrolling-mixin': ['object'],
    'editing-mixin': ['object'],
    'scrollable-pane': ['pane', 'scrolling-mixin'],
    'editable-pane': ['pane', 'editing-mixin'],
    'editable-scrollable-pane': ['scrollable-pane', 'editable-pane'],
}
# print(basesOf('editable-scrollable-pane', bases_dylan))
# print(linearize('editable-scrollable-pane', bases_dylan))


bases_viua = {
    'SharedBase': [],
    'SharedDerived': [],

    'BaseA': ['SharedBase'],
    'DerivedA': ['BaseA'],
    'MoreDerivedA': ['DerivedA', 'SharedDerived'],

    'BaseB': ['SharedBase'],
    'DerivedB': ['BaseB'],
    'MoreDerivedB': ['DerivedB', 'SharedDerived'],

    'Combined': ['MoreDerivedA', 'MoreDerivedB']
}
# print(basesOf('Combined', bases_viua))
print(linearize('Combined', bases_viua))
