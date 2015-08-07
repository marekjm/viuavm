#include <cstdlib>
#include <cstdint>
#include <iostream>
#include <algorithm>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <unordered_set>
#include <viua/bytecode/maps.h>
#include <viua/support/string.h>
#include <viua/support/env.h>
#include <viua/loader.h>
#include <viua/program.h>
#include <viua/cg/assembler/assembler.h>
#include <viua/front/asm.h>
using namespace std;

int gatherFunctions(invocables_t* invocables, const vector<string>& expanded_lines, const vector<string>& ilines) {
    ///////////////////////////////////////////
    // GATHER FUNCTION NAMES AND SIGNATURES
    //
    // SIGNATURES ARE USED WITH DYNAMIC LINKING
    // AS ASSEMBLER WOULD COMPLAIN ABOUT
    // CALLS TO UNDEFINED FUNCTIONS
    try {
        invocables->names = assembler::ce::getFunctionNames(expanded_lines);
    } catch (const string& e) {
        cout << "fatal: " << e << endl;
        return 1;
    }

    try {
        invocables->signatures = assembler::ce::getSignatures(expanded_lines);
    } catch (const string& e) {
        cout << "fatal: " << e << endl;
        return 1;
    }

    ///////////////////////////////
    // GATHER FUNCTIONS' CODE LINES
    try {
         invocables->bodies = assembler::ce::getInvokables("function", ilines);
    } catch (const string& e) {
        cout << "error: function gathering failed: " << e << endl;
        return 1;
    }

    return 0;
}

int gatherBlocks(invocables_t* invocables, const vector<string>& expanded_lines, const vector<string>& ilines) {
    /////////////////////
    // GATHER BLOCK NAMES
    try {
        invocables->names = assembler::ce::getBlockNames(expanded_lines);
    } catch (const string& e) {
        cout << "fatal: " << e << endl;
        return 1;
    }
    try {
        invocables->signatures = assembler::ce::getBlockSignatures(expanded_lines);
    } catch (const string& e) {
        cout << "fatal: " << e << endl;
        return 1;
    }

    ///////////////////////////////
    // GATHER BLOCK CODE LINES
    try {
         invocables->bodies = assembler::ce::getInvokables("block", ilines);
    } catch (const string& e) {
        cout << "error: block gathering failed: " << e << endl;
        return 1;
    }

    return 0;
}
