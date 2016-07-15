/*
 *  Copyright (C) 2016 Marek Marecki
 *
 *  This file is part of Viua VM.
 *
 *  Viua VM is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Viua VM is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Viua VM.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
using namespace std;


/** Log shortener tool; to be used when logs are verbose and plenty.
 *
 *  This tools takes a filename on input and prints to standard output.
 *  It shortens logs by cutting out series of identical lines and
 *  replacing them with a repetition report.
 *  For example, this text:
 *
 *      foo
 *      foo
 *      foo
 *      foo
 *      foo
 *      foo
 *      foo
 *      bar
 *
 *  ...will be shortened to:
 *
 *      foo
 *
 *      # repeated 5 time(s), continuing from line 7....
 *
 *      foo
 *      bar
 *
 *  This is not a general purpose tool, but one tailored to the format
 *  of Viua debugging logs.
 */


int main(int argc, char *argv[]) {
    string input_filename;

    if (argc < 2) {
        cerr << "error: no input file" << endl;
        return 1;
    }

    // extract input and output filenames from
    // commandline parameters
    input_filename = string(argv[1]);

    ifstream in(input_filename);

    // The `preprevious` is just for checking, never for printing while
    // inside the loop.
    // After the loop, `preprevious` can be used for printing during
    // the finishing section.
    string line, previous, preprevious;
    string _;
    unsigned long repeated = 0;

    int initialisation = 3;
    unsigned long line_counter = 0;

    while (getline(in, _)) {
        preprevious = previous;
        previous = line;
        line = _;

        if (initialisation) {
            // first three iterations are just to initialise the state
            --initialisation;
            ++line_counter;
            continue;
        }

        if (line == previous and previous == preprevious) {
            // another repeated line
            ++repeated;
        } else if (line != previous and previous == preprevious) {
            cout << previous << endl;
            if (repeated > 1) {
                cout << "\n# repeated " << repeated+1 << " time(s), continuing from line " << line_counter << "...\n" << endl;
            } else if (repeated == 1) {
                cout << previous << endl;
            } else {
                // repeated zero times
            }
            cout << previous << endl;
            repeated = 0;
        } else if (line == previous and previous != preprevious) {
            // do nothing
        } else if (line != previous and previous != preprevious) {
            cout << previous << endl;
            repeated = 0;
        }

        ++line_counter;
    }

    if (line == previous and previous == preprevious) {
        cout << line << endl;
        if (repeated > 1) {
            cout << "\n# repeated " << repeated+1 << " time(s)...\n" << endl;
            cout << line << endl;
        } else if (repeated == 1) {
            cout << line << endl;
            cout << line << endl;
        } else {
            // repeated zero times
        }
    } else if (line != previous and previous == preprevious) {
        cout << line << endl;
        if (repeated) {
            cout << "\n# repeated " << repeated+1 << " time(s)...\n" << endl;
            cout << line << endl;
        }
    } else if (line == previous and previous != preprevious) {
        cout << preprevious << endl;
    } else if (line != previous and previous != preprevious) {
        cout << previous << endl;
    }


    return 0;
}
