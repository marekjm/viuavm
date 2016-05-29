#include <iostream>
#include <fstream>
#include <vector>
#include <string>
using namespace std;


int main(int argc, char *argv[]) {
    string input_filename, output_filename;

    if (argc < 2) {
        cerr << "error: no input file" << endl;
        return 1;
    }

    // extract input and output filenames from
    // commandline parameters
    input_filename = string(argv[1]);
    if (argc >= 3) {
        output_filename = string(argv[2]);
    } else {
        output_filename = (input_filename + ".compressed.log");
    }

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
