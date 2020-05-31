/*
 *  Copyright (C) 2015, 2016, 2020 Marek Marecki
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

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <array>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include <viua/include/module.h>
#include <viua/kernel/frame.h>
#include <viua/kernel/registerset.h>
#include <viua/types/boolean.h>
#include <viua/types/exception.h>
#include <viua/types/integer.h>
#include <viua/types/string.h>
#include <viua/types/struct.h>
#include <viua/types/value.h>
#include <viua/types/vector.h>


static void os_system(Frame* frame,
                      viua::kernel::Register_set*,
                      viua::kernel::Register_set*,
                      viua::process::Process*,
                      viua::kernel::Kernel*)
{
    if (frame->arguments->at(0) == nullptr) {
        throw std::make_unique<viua::types::Exception>(
            "expected command to launch (string) as parameter 0");
    }
    auto const command = frame->arguments->get(0)->str();
    auto const ret     = system(command.c_str());
    frame->set_local_register_set(
        std::make_unique<viua::kernel::Register_set>(1));
    frame->local_register_set->set(0,
                                   std::make_unique<viua::types::Integer>(ret));
}

static void os_exec_pipe_stdout(Frame* frame,
                                viua::kernel::Register_set*,
                                viua::kernel::Register_set*,
                                viua::process::Process*,
                                viua::kernel::Kernel*)
{
    if (frame->arguments->at(0) == nullptr) {
        throw std::make_unique<viua::types::Exception>(
            "expected command to launch (vector of string) as parameter 0");
    }

    // "/usr/bin/stat", "--printf", "%a/%A %s byte(s), %F, %b blocks of %B",
    // "capture.cpp"
    auto args = std::vector<std::string>{};
    auto argv = std::vector<char*>{};

    auto const v = static_cast<viua::types::Vector*>(frame->arguments->at(0));
    for (auto const& each : v->value()) {
        args.push_back(each->str());
    }
    for (auto const& each : args) {
        argv.push_back(const_cast<char*>(each.c_str()));
    }
    argv.push_back(nullptr);

    std::array<int, 2> piped_stdout;
    pipe(piped_stdout.data());  // FIXME check errors

    auto const read_end  = piped_stdout[0];
    auto const write_end = piped_stdout[1];

    auto ret    = int{0};
    auto buffer = std::string{};

    auto const child_pid = fork();
    if (child_pid == 0) {
        close(read_end);
        dup2(write_end, 1);
        auto const n = execve(argv.at(0), argv.data(), nullptr);
        if (n == -1) {
            exit(1);
        }
    } else {
        close(write_end);

        auto buf = std::string{};
        buf.resize(1024);

        auto n = ssize_t{0};
        while ((n = read(read_end, buf.data(), buf.size())) > 0) {
            buf.resize(static_cast<size_t>(n));
            buffer += buf;
        }

        waitpid(child_pid, &ret, 0);

        close(read_end);
    }

    frame->set_local_register_set(
        std::make_unique<viua::kernel::Register_set>(2));
    frame->local_register_set->set(
        0, viua::types::String::make(std::move(buffer)));
}

static void os_lsdir(Frame* frame,
                     viua::kernel::Register_set*,
                     viua::kernel::Register_set*,
                     viua::process::Process*,
                     viua::kernel::Kernel*)
{
    auto const path = frame->arguments->at(0)->str();

    auto entries = std::make_unique<viua::types::Vector>();

    for (auto const& each : std::filesystem::directory_iterator{path}) {
        auto entry = std::make_unique<viua::types::Struct>();
        entry->insert("path",
                      std::make_unique<viua::types::String>(each.path()));
        entry->insert("is_directory",
                      viua::types::Boolean::make(
                          std::filesystem::is_directory(each.status())));
        entry->insert("is_regular_file",
                      viua::types::Boolean::make(
                          std::filesystem::is_regular_file(each.status())));
        entries->push(std::move(entry));
    }

    frame->set_local_register_set(
        std::make_unique<viua::kernel::Register_set>(1));
    frame->local_register_set->set(0, std::move(entries));
}

static void os_fs_path_lexically_normal(Frame* frame,
                                        viua::kernel::Register_set*,
                                        viua::kernel::Register_set*,
                                        viua::process::Process*,
                                        viua::kernel::Kernel*)
{
    auto const path = frame->arguments->at(0)->str();

    frame->set_local_register_set(
        std::make_unique<viua::kernel::Register_set>(1));
    using viua::types::String;
    frame->local_register_set->set(
        0, String::make(std::filesystem::path{path}.lexically_normal()));
}

static void os_fs_path_lexically_relative(Frame* frame,
                                          viua::kernel::Register_set*,
                                          viua::kernel::Register_set*,
                                          viua::process::Process*,
                                          viua::kernel::Kernel*)
{
    auto const path = frame->arguments->at(0)->str();
    auto const base = frame->arguments->at(1)->str();

    frame->set_local_register_set(
        std::make_unique<viua::kernel::Register_set>(1));
    using viua::types::String;
    frame->local_register_set->set(
        0, String::make(std::filesystem::path{path}.lexically_relative(base)));
}


const Foreign_function_spec functions[] = {
    {"std::os::system/1", &os_system},
    {"std::os::exec_pipe_stdout/1", os_exec_pipe_stdout},
    {"std::os::lsdir/1", &os_lsdir},
    {"std::os::fs::path::lexically_normal/1", &os_fs_path_lexically_normal},
    {"std::os::fs::path::lexically_relative/2", &os_fs_path_lexically_relative},
    {nullptr, nullptr},
};

extern "C" const Foreign_function_spec* exports()
{
    return functions;
}
