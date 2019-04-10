/*
 *  Copyright (C) 2015, 2016, 2017, 2018 Marek Marecki
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

#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <viua/include/module.h>
#include <viua/types/exception.h>
#include <viua/types/pointer.h>
#include <viua/types/string.h>
using namespace std;


class Ifstream : public viua::types::Value {
    mutable ifstream in;
    std::string const filename;

  public:
    auto type() const -> std::string override {
        return "Ifstream";
    }
    auto str() const -> std::string override {
        return type();
    }
    auto repr() const -> std::string override {
        return str();
    }
    auto boolean() const -> bool override {
        return in.is_open();
    }

    auto getline() const -> std::string {
        if (in.eof()) {
            throw make_unique<viua::types::Exception>("EOF");
        }

        auto line = std::string{};
        ::std::getline(in, line);
        return line;
    }

    auto copy() const -> std::unique_ptr<viua::types::Value> override {
        throw make_unique<viua::types::Exception>("Ifstream is not copyable");
    }

    Ifstream(std::string const& path) : filename(path) {
        in.open(filename);
    }
    virtual ~Ifstream() {
        if (in.is_open()) {
            in.close();
        }
    }
};


static auto io_stdin_getline(Frame* frame,
                             viua::kernel::Register_set*,
                             viua::kernel::Register_set*,
                             viua::process::Process*,
                             viua::kernel::Kernel*) -> void {
    auto line = std::string{};
    getline(cin, line);

    frame->set_local_register_set(
        std::make_unique<viua::kernel::Register_set>(1));
    frame->local_register_set->set(0,
                                   std::make_unique<viua::types::String>(line));
}

static auto io_stdout_write(Frame* frame,
                            viua::kernel::Register_set*,
                            viua::kernel::Register_set*,
                            viua::process::Process*,
                            viua::kernel::Kernel*) -> void {
    cout << frame->arguments->at(0)->str();
}

static auto io_stderr_write(Frame* frame,
                            viua::kernel::Register_set*,
                            viua::kernel::Register_set*,
                            viua::process::Process*,
                            viua::kernel::Kernel*) -> void {
    cerr << frame->arguments->at(0)->str();
}


static auto io_file_read(Frame* frame,
                         viua::kernel::Register_set*,
                         viua::kernel::Register_set*,
                         viua::process::Process*,
                         viua::kernel::Kernel*) -> void {
    auto const path = frame->arguments->get(0)->str();
    auto in         = ifstream{path};

    auto oss  = ostringstream{};
    auto line = std::string{};
    while (getline(in, line)) {
        oss << line << '\n';
    }

    frame->local_register_set->set(0,
                                   make_unique<viua::types::String>(oss.str()));
}

static auto io_file_write(Frame* frame,
                          viua::kernel::Register_set*,
                          viua::kernel::Register_set*,
                          viua::process::Process*,
                          viua::kernel::Kernel*) -> void {
    auto out = ofstream{frame->arguments->get(0)->str()};
    out << frame->arguments->get(1)->str();
    out.close();
}

static auto io_ifstream_open(Frame* frame,
                             viua::kernel::Register_set*,
                             viua::kernel::Register_set*,
                             viua::process::Process*,
                             viua::kernel::Kernel*) -> void {
    frame->local_register_set->set(
        0, make_unique<Ifstream>(frame->arguments->get(0)->str()));
}

static auto io_ifstream_getline(Frame* frame,
                                viua::kernel::Register_set*,
                                viua::kernel::Register_set*,
                                viua::process::Process* p,
                                viua::kernel::Kernel*) -> void {
    auto const in = dynamic_cast<Ifstream*>(
        static_cast<viua::types::Pointer*>(frame->arguments->get(0))->to(p));
    frame->local_register_set->set(
        0, make_unique<viua::types::String>(in->getline()));
}

namespace io {
class Fstream : public viua::types::Value {
    std::string const file_name;
    mutable std::fstream the_stream;

  public:
    auto type() const -> std::string override {
        return "Fstream";
    }
    auto str() const -> std::string override {
        return type();
    }
    auto repr() const -> std::string override {
        return str();
    }
    auto boolean() const -> bool override {
        return the_stream.is_open();
    }
    auto copy() const -> std::unique_ptr<viua::types::Value> override {
        throw make_unique<viua::types::Exception>("Fstream is not copyable");
    }

    auto peek() -> std::string;
    auto getline(std::string::value_type const = '\n') -> std::string;

    Fstream(std::string const& path):
        file_name(path)
        , the_stream{file_name}
    {}
    virtual ~Fstream() {
        if (the_stream.is_open()) {
            the_stream.close();
        }
    }
};

auto Fstream::peek() -> std::string {
    return std::string{1, static_cast<std::string::value_type>(the_stream.peek())};
}

auto Fstream::getline(std::string::value_type const delim) -> std::string {
    auto s = std::string{};
    std::getline(the_stream, s, delim);
    return s;
}

static auto fstream_open(Frame* frame,
                         viua::kernel::Register_set*,
                         viua::kernel::Register_set*,
                         viua::process::Process*,
                         viua::kernel::Kernel*) -> void {
    auto const path = frame->arguments->get(0)->str();
    frame->set_local_register_set(
        std::make_unique<viua::kernel::Register_set>(1));
    frame->local_register_set->set(0,
                                   std::make_unique<Fstream>(path));
}

static auto fstream_peek(Frame* frame,
                         viua::kernel::Register_set*,
                         viua::kernel::Register_set*,
                         viua::process::Process* p,
                         viua::kernel::Kernel*) -> void {
    auto const fstream = dynamic_cast<Fstream*>(
        static_cast<viua::types::Pointer*>(frame->arguments->get(0))->to(p));
    frame->set_local_register_set(
        std::make_unique<viua::kernel::Register_set>(1));
    frame->local_register_set->set(
        0, std::make_unique<viua::types::String>(fstream->peek()));
}

static auto fstream_getline_default(Frame* frame,
                         viua::kernel::Register_set*,
                         viua::kernel::Register_set*,
                         viua::process::Process* p,
                         viua::kernel::Kernel*) -> void {
    auto const fstream = dynamic_cast<Fstream*>(
        static_cast<viua::types::Pointer*>(frame->arguments->get(0))->to(p));
    frame->set_local_register_set(
        std::make_unique<viua::kernel::Register_set>(1));
    frame->local_register_set->set(
        0, std::make_unique<viua::types::String>(fstream->getline()));
}

static auto fstream_getline_delim(Frame* frame,
                         viua::kernel::Register_set*,
                         viua::kernel::Register_set*,
                         viua::process::Process* p,
                         viua::kernel::Kernel*) -> void {
    auto const fstream = dynamic_cast<Fstream*>(
        static_cast<viua::types::Pointer*>(frame->arguments->get(0))->to(p));
    auto const delim = frame->arguments->get(1)->str().at(0);
    frame->set_local_register_set(
        std::make_unique<viua::kernel::Register_set>(1));
    frame->local_register_set->set(
        0, std::make_unique<viua::types::String>(fstream->getline(delim)));
}
}

const Foreign_function_spec functions[] = {
    {"std::io::stdin::getline/0", &io_stdin_getline},
    {"std::io::stdout::write/1", &io_stdout_write},
    {"std::io::stderr::write/1", &io_stderr_write},
    {"std::io::file::read/1", &io_file_read},
    {"std::io::file::write/1", &io_file_write},
    {"std::io::ifstream::open/1", &io_ifstream_open},
    {"std::io::ifstream::getline/1", &io_ifstream_getline},

    /*
     * New I/O library.
     */
    {"std::io::fstream::open/1", &io::fstream_open},
    {"std::io::fstream::peek/1", &io::fstream_peek},
    {"std::io::fstream::getline/1", &io::fstream_getline_default},
    {"std::io::fstream::getline/2", &io::fstream_getline_delim},

    {nullptr, nullptr},
};

extern "C" const Foreign_function_spec* exports() {
    return functions;
}
