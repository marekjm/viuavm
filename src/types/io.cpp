/*
 *  Copyright (C) 2019 Marek Marecki
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

#include <sys/select.h>
#include <unistd.h>
#include <iostream>
#include <memory>
#include <string>
#include <viua/exceptions.h>
#include <viua/types/integer.h>
#include <viua/types/io.h>
#include <viua/types/string.h>

namespace viua::types {
auto IO_interaction::id() const -> id_type {
    return assigned_id;
}
auto IO_interaction::cancel() -> void {
    is_cancelled.store(true, std::memory_order_release);
}
auto IO_interaction::cancelled() const -> bool {
    return is_cancelled.load(std::memory_order_acquire);
}
auto IO_interaction::abort() -> void {
    is_aborted.store(true, std::memory_order_release);
}
auto IO_interaction::aborted() const -> bool {
    return is_aborted.load(std::memory_order_acquire);
}
auto IO_interaction::complete() -> void {
    is_complete.store(true, std::memory_order_release);
}
auto IO_interaction::completed() const -> bool {
    return is_complete.load(std::memory_order_acquire);
}

IO_interaction::IO_interaction(viua::process::Process* const proc, id_type const x)
    : assigned_id{x}
    , from_process{proc}
{}
IO_interaction::~IO_interaction() {}

IO_interaction::Interaction_result::Interaction_result(
    State const st
    , Status const su
    , result_type r)
    : state{st}
    , status{su}
    , result{std::move(r)}
{}

auto IO_fake_interaction::interact() -> Interaction_result {
    std::cerr << "IO_fake_interaction interacting!\n";
    return Interaction_result{
        IO_interaction::State::Complete,
        IO_interaction::Status::Success,
        std::make_unique<viua::types::String>("Hello, World!")
    };
}

IO_read_interaction::IO_read_interaction(
    viua::process::Process* const proc
    , id_type const x
    , int const fd
    , size_t const limit)
    : IO_interaction(proc, x)
    , file_descriptor{fd}
    , buffer(limit, '\0')
{}
auto IO_read_interaction::interact() -> Interaction_result {
    if (cancelled()) {
        return Interaction_result{
            IO_interaction::State::Complete,
            IO_interaction::Status::Cancelled,
            std::make_unique<viua::types::Exception>("IO_cancel", "I/O cancelled")
        };
    }

    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(file_descriptor, &readfds);

    timeval timeout;
    memset(&timeout, 0, sizeof(timeout));
    timeout.tv_sec = 0;
    timeout.tv_usec = 1;

    auto const s = select(file_descriptor + 1, &readfds, nullptr, nullptr, &timeout);
    if (s == -1) {
        auto const saved_errno = errno;
        return Interaction_result{
            IO_interaction::State::Complete,
            IO_interaction::Status::Error,
            std::make_unique<viua::types::Integer>(saved_errno)
        };
    }
    if (s == 0) {
        return Interaction_result{};
    }

    auto const n = ::read(file_descriptor, buffer.data(), buffer.size());

    if (n == -1) {
        auto const saved_errno = errno;
        return Interaction_result{
            IO_interaction::State::Complete,
            IO_interaction::Status::Error,
            std::make_unique<viua::types::Integer>(saved_errno)
        };
    }
    if (n == 0) {
        return Interaction_result{
            IO_interaction::State::Complete,
            IO_interaction::Status::Success,
            std::make_unique<viua::types::String>("")
        };
    }
    buffer.resize(static_cast<decltype(buffer)::size_type>(n));
    return Interaction_result{
        IO_interaction::State::Complete,
        IO_interaction::Status::Success,
        std::make_unique<viua::types::String>(std::move(buffer))
    };
}

std::string const viua::types::IO_port::type_name = "IO_port";

std::string IO_port::type() const {
    return "IO_port";
}
std::string IO_port::str() const {
    return "";
}
std::string IO_port::repr() const {
    return "";
}
bool IO_port::boolean() const {
    return false;
}

std::unique_ptr<Value> IO_port::copy() const {
    return std::unique_ptr<IO_port>();
}

IO_port::IO_port() {}
IO_port::~IO_port() {}


std::string const viua::types::IO_fd::type_name = "IO_fd";

std::string IO_fd::type() const {
    return "IO_fd";
}
std::string IO_fd::str() const {
    return "";
}
std::string IO_fd::repr() const {
    return "";
}
bool IO_fd::boolean() const {
    return true;
}

std::unique_ptr<Value> IO_fd::copy() const {
    return std::make_unique<IO_fd>(file_descriptor);
}

auto IO_fd::read(viua::process::Process& proc, std::unique_ptr<Value> x) -> std::unique_ptr<IO_request> {
    auto const interaction_id = IO_interaction::id_type{ file_descriptor, counter++ };
    auto const limit = static_cast<viua::types::Integer*>(x.get())->as_integer();
    proc.schedule_io(std::make_unique<IO_read_interaction>(
        &proc
        , interaction_id
        , file_descriptor
        , static_cast<size_t>(limit)
    ));
    return std::make_unique<IO_request>(&proc, interaction_id);
}

IO_fd::IO_fd(int const x)
    : file_descriptor{x}
{}
IO_fd::~IO_fd() {}


std::string const viua::types::IO_request::type_name = "IO_request";

std::string IO_request::type() const {
    return "IO_request";
}
std::string IO_request::str() const {
    auto const [ a, b ] = interaction_id;
    return "<I/O request: " + std::to_string(a) + "." + std::to_string(b) + ">";
}
std::string IO_request::repr() const {
    return "<I/O request>";
}
bool IO_request::boolean() const {
    return true;
}

std::unique_ptr<Value> IO_request::copy() const {
    throw std::make_unique<viua::types::Exception>(
          "IO_request"
        , "not copyable"
    );
}

IO_request::IO_request(viua::process::Process* proc, IO_interaction::id_type const x)
    : interaction_id{x}
    , from_process{proc}
{}
IO_request::~IO_request() {
    from_process->cancel_io(id());
}
}
