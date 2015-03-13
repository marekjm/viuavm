#include <iostream>
#include <algorithm>
#include <vector>
#include <string>
#include "src/support/string.h"
using namespace std;


class State {
    public:
        vector<State*> children;
        string value;

        State& feed(vector<string>);
        vector<string> resolve();
};


int main() {
    int return_value = 0;

    string cmd("");
    vector<string> chunks;

    bool quit = false;

    while (not quit) {
        cout << ">>> ";
        getline(cin, cmd);

        if (str::lstrip(cmd) == "") {
            continue;
        }

        chunks = str::chunks(cmd);

        int i = 0;
        if (chunks[i] == "set") {
            ++i;
            if (i >= chunks.size()) {
                cout << "error: invalid command" << endl;
                continue;
            }
            if (chunks[i] == "breakpoint") {
                ++i;
                if (i >= chunks.size()) {
                    cout << "error: invalid command" << endl;
                    continue;
                }
                if (chunks[i] == "at") {
                    ++i;
                    if (i >= chunks.size()) {
                        cout << "error: invalid command" << endl;
                        continue;
                    }
                    if (str::isnum(chunks[i])) {
                        cout << "breakpoint set at byte " << chunks[i] << endl;
                    }
                }
            } else if (chunks[i] == "return") {
                ++i;
                if (i >= chunks.size()) {
                    cout << "error: invalid command" << endl;
                    continue;
                }
                if (str::isnum(chunks[i])) {
                    return_value = stoi(chunks[i]);
                }
            } else {
                cout << "error: invalid command" << endl;
                continue;
            }
        } else if (chunks[i] == "quit") {
            quit = true;
        } else {
            cout << "error: invalid command" << endl;
        }
    }

    return return_value;
}
