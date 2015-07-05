#include <string>
#include <vector>
#include <tuple>
#include <map>
#include "../../support/string.h"
#include "../../program.h"
#include "assembler.h"
using namespace std;


vector<string> assembler::ce::getilines(const vector<string>& lines) {
    /*  Clears code from empty lines and comments.
     */
    vector<string> ilines;
    string line;

    for (unsigned i = 0; i < lines.size(); ++i) {
        line = str::lstrip(lines[i]);
        if (!line.size() or line[0] == ';') continue;
        ilines.push_back(line);
    }

    return ilines;
}

map<string, int> assembler::ce::getmarks(const vector<string>& lines) {
    /** This function will pass over all instructions and
     * gather "marks", i.e. `.mark: <name>` directives which may be used by
     * `jump` and `branch` instructions.
     *
     * When referring to a mark in code, you should use: `jump :<name>`.
     *
     * The colon before name of the marker is placed here to make it possible to use numeric markers
     * which would otherwise be treated as instruction indexes.
     */
    map<string, int> marks;
    string line, mark;
    int instruction = 0;  // we need separate instruction counter because number of lines is not exactly number of instructions
    for (unsigned i = 0; i < lines.size(); ++i) {
        line = lines[i];
        if (str::startswith(line, ".name:") or str::startswith(line, ".link:")) {
            // names and links can be safely skipped as they are not CPU instructions
            continue;
        }
        if (str::startswith(line, ".function:")) {
            // instructions in functions are counted separately so they are
            // not included here
            while (!str::startswith(lines[i], ".end")) { ++i; }
            continue;
        }
        if (!str::startswith(line, ".mark:")) {
            // if all previous checks were false, then this line must be either .mark: directive or
            // an instruction
            // if this check is true - then it is an instruction
            ++instruction;
            continue;
        }

        // get mark name
        line = str::lstrip(str::sub(line, 6));
        mark = str::chunk(line);

        // create mark for current instruction
        marks[mark] = instruction;
    }
    return marks;
}

map<string, int> assembler::ce::getnames(const vector<string>& lines) {
    /** This function will pass over all instructions and
     *  gather "names", i.e. `.name: <register> <name>` instructions which may be used by
     *  as substitutes for register indexes to more easily remember what is stored where.
     *
     *  Example name instruction: `.name: 1 base`.
     *  This allows to access first register with name `base` instead of its index.
     *
     *  Example (which also uses marks) name reference could be: `branch if_equals_0 :finish`.
     */
    map<string, int> names;
    string line, reg, name;
    for (unsigned i = 0; i < lines.size(); ++i) {
        line = lines[i];
        if (!str::startswith(line, ".name:")) {
            continue;
        }

        line = str::lstrip(str::sub(line, 6));
        reg = str::chunk(line);
        line = str::lstrip(str::sub(line, reg.size()));
        name = str::chunk(line);

        try {
            names[name] = stoi(reg);
        } catch (const std::invalid_argument& e) {
            throw "invalid register index in .name instruction";
        }
    }
    return names;
}

vector<string> assembler::ce::getlinks(const vector<string>& lines) {
    /** This function will pass over all instructions and
     * gather .link: assembler instructions.
     */
    vector<string> links;
    string line;
    for (unsigned i = 0; i < lines.size(); ++i) {
        line = lines[i];
        if (str::startswith(line, ".link:")) {
            line = str::chunk(str::lstrip(str::sub(line, str::chunk(line).size())));
            links.push_back(line);
        }
    }
    return links;
}

vector<string> assembler::ce::getFunctionNames(const vector<string>& lines) {
    vector<string> names;

    string line, holdline;
    for (unsigned i = 0; i < lines.size(); ++i) {
        holdline = line = lines[i];
        if (!str::startswith(line, ".function:")) { continue; }

        if (str::startswith(line, ".function:")) {
            for (int j = i+1; lines[j] != ".end"; ++j, ++i) {}
        }

        line = str::lstrip(str::sub(line, str::chunk(line).size()));
        names.push_back(str::chunk(line));
    }

    return names;
}
vector<string> assembler::ce::getSignatures(const vector<string>& lines) {
    vector<string> names;

    string line, holdline;
    for (unsigned i = 0; i < lines.size(); ++i) {
        holdline = line = lines[i];
        if (!str::startswith(line, ".signature:")) { continue; }
        line = str::lstrip(str::sub(line, str::chunk(line).size()));
        names.push_back(str::chunk(line));
    }

    return names;
}
vector<string> assembler::ce::getBlockNames(const vector<string>& lines) {
    vector<string> names;

    string line, holdline;
    for (unsigned i = 0; i < lines.size(); ++i) {
        holdline = line = lines[i];
        if (not str::startswith(line, ".block:")) { continue; }

        if (str::startswith(line, ".block:")) {
            for (int j = i+1; lines[j] != ".end"; ++j, ++i) {}
        }

        line = str::lstrip(str::sub(line, str::chunk(line).size()));
        string name = str::chunk(line);
        line = str::lstrip(str::sub(line, name.size()));
        string ret_sign = str::chunk(line);

        names.push_back(name);
    }

    return names;
}
map<string, vector<string> > assembler::ce::getInvokables(const string& type, const vector<string>& lines) {
    map<string, vector<string> > invokables;

    string opening;
    if (type == "function") {
        opening = ".function:";
    } else if (type == "block") {
        opening = ".block:";
    }

    string line, holdline;
    for (unsigned i = 0; i < lines.size(); ++i) {
        holdline = line = lines[i];
        if (!str::startswith(line, opening)) { continue; }

        vector<string> flines;
        for (int j = i+1; lines[j] != ".end"; ++j, ++i) {
            if (str::startswith(lines[j], opening)) {
                throw ("another " + type + " opened before assembler reached .end after '" + str::chunk(str::sub(holdline, str::chunk(holdline).size())) + "' " + type);
            }
            flines.push_back(lines[j]);
        }

        line = str::lstrip(str::sub(line, opening.size()));
        string name = str::chunk(line);
        line = str::lstrip(str::sub(line, name.size()));

        invokables[name] = flines;
    }

    return invokables;
}
