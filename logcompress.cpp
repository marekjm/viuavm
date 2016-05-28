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

    string line, previous, preprevious;
    string _;
    unsigned long repeated = 0;

    int initialisation = 3;

    while (getline(in, _)) {
        preprevious = previous;
        previous = line;
        line = _;

        if (initialisation) {
            // first three iterations are just to initialise the state
            --initialisation;
            continue;
        }

        if (line == previous and previous == preprevious) {
            // another repeated line
            ++repeated;
        } else if (line != previous and previous == preprevious) {
            cout << previous << endl;
            if (repeated) {
                cout << "\n# repeated " << repeated << " time(s)...\n" << endl;
                cout << previous << endl;
            }
            repeated = 0;
        } else if (line == previous and previous != preprevious) {
            cout << preprevious << endl;
        }

        /* if (initialisation && line == previous) { */
        /*     ++repeated; */
        /* } */

        /* if (line == previous) { */
        /*     ++repeated; */
        /* } else { */
        /*     cout << line << endl; */
        /*     if (repeated) { */
        /*         cout << "\n# repeated " << repeated << " time(s)...\n" << endl; */
        /*         cout << line << endl; */
        /*     } */
        /*     repeated = 0; */
        /* } */
    }

    if (line == previous and previous == preprevious) {
        // another repeated line
        ++repeated;
    } else if (line != previous and previous == preprevious) {
        cout << line << endl;
        if (repeated) {
            cout << "\n# repeated " << repeated << " time(s)...\n" << endl;
            cout << line << endl;
        }
    } else if (line == previous and previous != preprevious) {
        cout << preprevious << endl;
    }


    return 0;
}
