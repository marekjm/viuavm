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


static auto io_stdin_getline(Frame* frame,
                             viua::kernel::Register_set*,
                             viua::kernel::Register_set*,
                             viua::process::Process*,
                             viua::kernel::Kernel*) -> void {
    auto line = std::string{};
    std::getline(std::cin, line);

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
    std::cout << frame->arguments->at(0)->str();
}

static auto io_stderr_write(Frame* frame,
                            viua::kernel::Register_set*,
                            viua::kernel::Register_set*,
                            viua::process::Process*,
                            viua::kernel::Kernel*) -> void {
    std::cerr << frame->arguments->at(0)->str();
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
        throw std::make_unique<viua::types::Exception>("Fstream is not copyable");
    }

    auto peek() -> std::string;
    auto getline(std::string::value_type const = '\n') -> std::string;
    auto read(size_t const) -> std::string;
    auto read() -> std::string;
    auto write(std::string const) -> void;

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

auto Fstream::read(size_t const size) -> std::string {
    auto s = std::vector<std::string::value_type>(size);
    the_stream.read(s.data(), static_cast<std::streamsize>(size));
    return std::string{s.data(), static_cast<std::string::size_type>(the_stream.gcount())};
}

auto Fstream::read() -> std::string {
    auto const current_position = the_stream.tellg();
    the_stream.seekg(0, std::ios_base::end);
    auto const size = the_stream.tellg();
    the_stream.seekg(current_position);

    auto s = std::vector<std::string::value_type>(static_cast<size_t>(size));
    the_stream.read(s.data(), static_cast<std::streamsize>(size));
    return std::string{s.data(), static_cast<std::string::size_type>(the_stream.gcount())};
}

auto Fstream::write(std::string const data) -> void {
    the_stream.clear();
    the_stream.seekp(0);
    the_stream.write(data.data(), static_cast<std::streamsize>(data.size()));
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

static auto fstream_read(Frame* frame,
                         viua::kernel::Register_set*,
                         viua::kernel::Register_set*,
                         viua::process::Process* p,
                         viua::kernel::Kernel*) -> void {
    auto const fstream = dynamic_cast<Fstream*>(
        static_cast<viua::types::Pointer*>(frame->arguments->get(0))->to(p));
    auto const size =
        static_cast<viua::types::Integer*>(frame->arguments->get(1))->as_unsigned();
    frame->set_local_register_set(
        std::make_unique<viua::kernel::Register_set>(1));
    frame->local_register_set->set(
        0, std::make_unique<viua::types::String>(fstream->read(size)));
}

static auto fstream_read_whole(Frame* frame,
                         viua::kernel::Register_set*,
                         viua::kernel::Register_set*,
                         viua::process::Process* p,
                         viua::kernel::Kernel*) -> void {
    auto const fstream = dynamic_cast<Fstream*>(
        static_cast<viua::types::Pointer*>(frame->arguments->get(0))->to(p));
    frame->set_local_register_set(
        std::make_unique<viua::kernel::Register_set>(1));
    frame->local_register_set->set(
        0, std::make_unique<viua::types::String>(fstream->read()));
}

static auto fstream_write(Frame* frame,
                         viua::kernel::Register_set*,
                         viua::kernel::Register_set*,
                         viua::process::Process* p,
                         viua::kernel::Kernel*) -> void {
    auto const fstream = dynamic_cast<Fstream*>(
        static_cast<viua::types::Pointer*>(frame->arguments->get(0))->to(p));
    auto const data = frame->arguments->get(1)->str();
    fstream->write(data);
}
}

const Foreign_function_spec functions[] = {
    {"std::io::stdin::getline/0", &io_stdin_getline},
    {"std::io::stdout::write/1", &io_stdout_write},
    {"std::io::stderr::write/1", &io_stderr_write},

    {"std::io::fstream::open/1", &io::fstream_open},
    {"std::io::fstream::peek/1", &io::fstream_peek},
    {"std::io::fstream::getline/1", &io::fstream_getline_default},
    {"std::io::fstream::getline/2", &io::fstream_getline_delim},
    {"std::io::fstream::read/1", &io::fstream_read_whole},
    {"std::io::fstream::read/2", &io::fstream_read},
    {"std::io::fstream::write/2", &io::fstream_write},

    {nullptr, nullptr},
};

extern "C" const Foreign_function_spec* exports() {
    return functions;
}
