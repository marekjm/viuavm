/*
 *  Copyright (C) 2015, 2016 Marek Marecki
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
#include <sstream>
#include <viua/types/type.h>
#include <viua/types/pointer.h>
#include <viua/types/string.h>
#include <viua/types/vector.h>
#include <viua/types/exception.h>
#include <viua/kernel/frame.h>
#include <viua/kernel/registerset.h>
#include <viua/include/module.h>
using namespace std;


class Ifstream: public viua::types::Type {
        ifstream in;
        const string filename;

    public:
        string type() const override {
            return "Ifstream";
        }
        string str() const override {
            return type();
        }
        string repr() const override {
            return str();
        }
        bool boolean() const override {
            return in.is_open();
        }

        virtual std::vector<std::string> bases() const {
            return std::vector<std::string>{"viua::types::Type"};
        }
        virtual std::vector<std::string> inheritancechain() const {
            return std::vector<std::string>{"viua::types::Type"};
        }

        string getline() {
            string line;
            std::getline(in, line);
            return line;
        }

        virtual viua::types::Type* copy() const {
        }

        Ifstream(const string& path): filename(path) {
            in.open(filename);
        }
        virtual ~Ifstream() {
            if (in.is_open()) {
                in.close();
            }
        }
};


void io_stdin_getline(Frame* frame, RegisterSet*, RegisterSet*, viua::process::Process*, viua::kernel::Kernel*) {
    string line;
    getline(cin, line);
    frame->regset->set(0, new viua::types::String(line));
}


void io_file_read(Frame* frame, RegisterSet*, RegisterSet*, viua::process::Process*, viua::kernel::Kernel*) {
    string path = frame->args->get(0)->str();
    ifstream in(path);

    ostringstream oss;
    string line;
    while (getline(in, line)) {
        oss << line << '\n';
    }

    frame->regset->set(0, new viua::types::String(oss.str()));
}

void io_ifstream_open(Frame *frame, RegisterSet*, RegisterSet*, viua::process::Process*, viua::kernel::Kernel*) {
    frame->regset->set(0, new Ifstream(frame->args->get(0)->str()));
}

void io_ifstream_getline(Frame *frame, RegisterSet*, RegisterSet*, viua::process::Process*, viua::kernel::Kernel*) {
    Ifstream *in = dynamic_cast<Ifstream*>(static_cast<viua::types::Pointer*>(frame->args->get(0))->to());
    frame->regset->set(0, new viua::types::String(in->getline()));
}

const ForeignFunctionSpec functions[] = {
    { "std::io::stdin::getline/0", &io_stdin_getline },
    { "std::io::file::read/1", &io_file_read },
    { "std::io::ifstream::open/1", &io_ifstream_open },
    { "std::io::ifstream::getline/1", &io_ifstream_getline },
    { NULL, NULL },
};

extern "C" const ForeignFunctionSpec* exports() {
    return functions;
}
