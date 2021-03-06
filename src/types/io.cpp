/*
 *  Copyright (C) 2019, 2020 Marek Marecki
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
#include <viua/kernel/kernel.h>
#include <viua/types/integer.h>
#include <viua/types/io.h>
#include <viua/types/string.h>

namespace viua::scheduler::io {
auto IO_interaction::id() const -> id_type
{
    return assigned_id;
}
auto IO_interaction::cancel() -> void
{
    is_cancelled.store(true, std::memory_order_release);
}
auto IO_interaction::cancelled() const -> bool
{
    return is_cancelled.load(std::memory_order_acquire);
}
auto IO_interaction::abort() -> void
{
    is_aborted.store(true, std::memory_order_release);
}
auto IO_interaction::aborted() const -> bool
{
    return is_aborted.load(std::memory_order_acquire);
}
auto IO_interaction::complete() -> void
{
    is_complete.store(true, std::memory_order_release);
}
auto IO_interaction::completed() const -> bool
{
    return is_complete.load(std::memory_order_acquire);
}

IO_interaction::IO_interaction(id_type const x) : assigned_id{x}
{}
IO_interaction::~IO_interaction()
{}

IO_interaction::Interaction_result::Interaction_result(State const st,
                                                       Status const su,
                                                       result_type r)
        : state{st}, status{su}, result{std::move(r)}
{}

IO_read_interaction::IO_read_interaction(id_type const x,
                                         int const fd,
                                         size_t const limit)
        : IO_interaction(x), file_descriptor{fd}, buffer(limit, '\0')
{}
auto IO_read_interaction::interact() -> Interaction_result
{
    if (cancelled()) {
        return Interaction_result{
            IO_interaction::State::Complete,
            IO_interaction::Status::Cancelled,
            std::make_unique<viua::types::Exception>(
                viua::types::Exception::Tag{"IO_cancel"}, "I/O cancelled")};
    }

    auto const n = ::read(file_descriptor, buffer.data(), buffer.size());

    if (n == -1) {
        auto const saved_errno = errno;
        return Interaction_result{
            IO_interaction::State::Complete,
            IO_interaction::Status::Error,
            std::make_unique<viua::types::Integer>(saved_errno)};
    }
    if (n == 0) {
        return Interaction_result{IO_interaction::State::Complete,
                                  IO_interaction::Status::Success,
                                  std::make_unique<viua::types::String>("")};
    }
    buffer.resize(static_cast<decltype(buffer)::size_type>(n));
    return Interaction_result{
        IO_interaction::State::Complete,
        IO_interaction::Status::Success,
        std::make_unique<viua::types::String>(std::move(buffer))};
}

IO_write_interaction::IO_write_interaction(id_type const x,
                                           int const fd,
                                           std::string buf)
        : IO_interaction(x), file_descriptor{fd}, buffer{std::move(buf)}
{}
auto IO_write_interaction::interact() -> Interaction_result
{
    if (cancelled()) {
        return Interaction_result{
            IO_interaction::State::Complete,
            IO_interaction::Status::Cancelled,
            std::make_unique<viua::types::Exception>(
                viua::types::Exception::Tag{"IO_cancel"}, "I/O cancelled")};
    }

    auto const n = ::write(file_descriptor, buffer.data(), buffer.size());

    if (n == -1) {
        auto const saved_errno = errno;
        return Interaction_result{
            IO_interaction::State::Complete,
            IO_interaction::Status::Error,
            std::make_unique<viua::types::Integer>(saved_errno)};
    }
    return Interaction_result{IO_interaction::State::Complete,
                              IO_interaction::Status::Success,
                              std::make_unique<viua::types::String>(
                                  buffer.substr(static_cast<size_t>(n)))};
}

IO_close_interaction::IO_close_interaction(id_type const x, int const fd)
        : IO_interaction(x), file_descriptor{fd}
{}
auto IO_close_interaction::interact() -> Interaction_result
{
    if (cancelled()) {
        return Interaction_result{
            IO_interaction::State::Complete,
            IO_interaction::Status::Cancelled,
            std::make_unique<viua::types::Exception>(
                viua::types::Exception::Tag{"IO_cancel"}, "I/O cancelled")};
    }

    auto const n = ::close(file_descriptor);

    if (n == -1) {
        auto const saved_errno = errno;
        return Interaction_result{
            IO_interaction::State::Complete,
            IO_interaction::Status::Error,
            std::make_unique<viua::types::Integer>(saved_errno)};
    }
    return Interaction_result{IO_interaction::State::Complete,
                              IO_interaction::Status::Success,
                              std::make_unique<viua::types::Boolean>(true)};
}

auto IO_empty_interaction::interact() -> Interaction_result
{
    if (cancelled()) {
        return Interaction_result{
            IO_interaction::State::Complete,
            IO_interaction::Status::Cancelled,
            std::make_unique<viua::types::Exception>(
                viua::types::Exception::Tag{"IO_cancel"}, "I/O cancelled")};
    }

    return Interaction_result{IO_interaction::State::Complete,
                              IO_interaction::Status::Success,
                              std::make_unique<viua::types::Boolean>(true)};
}
}  // namespace viua::scheduler::io

namespace viua::types {

std::string IO_port::type() const
{
    return type_name;
}
std::string IO_port::str() const
{
    return "";
}
std::string IO_port::repr() const
{
    return "";
}
bool IO_port::boolean() const
{
    return false;
}

std::unique_ptr<Value> IO_port::copy() const
{
    return std::unique_ptr<IO_port>();
}

IO_port::IO_port()
{}
IO_port::~IO_port()
{}


std::string IO_fd::type() const
{
    return type_name;
}
std::string IO_fd::str() const
{
    return "IO_fd: " + std::to_string(file_descriptor);
}
std::string IO_fd::repr() const
{
    return std::to_string(file_descriptor);
}
bool IO_fd::boolean() const
{
    return true;
}

std::unique_ptr<Value> IO_fd::copy() const
{
    throw std::make_unique<viua::types::Exception>(
        viua::types::Exception::Tag{"Not_copyable"});
}

auto IO_fd::fd() const -> int
{
    return file_descriptor;
}
auto IO_fd::read(viua::kernel::Kernel& k, std::unique_ptr<Value> x)
    -> std::unique_ptr<IO_request>
{
    using viua::scheduler::io::IO_interaction;
    using viua::scheduler::io::IO_read_interaction;

    auto const interaction_id =
        IO_interaction::id_type{file_descriptor, counter++};
    auto const limit =
        static_cast<viua::types::Integer*>(x.get())->as_integer();
    k.schedule_io(std::make_unique<IO_read_interaction>(
        interaction_id, file_descriptor, static_cast<size_t>(limit)));
    return std::make_unique<IO_request>(&k, interaction_id);
}
auto IO_fd::write(viua::kernel::Kernel& k, std::unique_ptr<Value> x)
    -> std::unique_ptr<IO_request>
{
    using viua::scheduler::io::IO_interaction;
    using viua::scheduler::io::IO_write_interaction;

    auto const interaction_id =
        IO_interaction::id_type{file_descriptor, counter++};
    auto buffer = x->str();
    k.schedule_io(std::make_unique<IO_write_interaction>(
        interaction_id, file_descriptor, std::move(buffer)));
    return std::make_unique<IO_request>(&k, interaction_id);
}
auto IO_fd::close(viua::kernel::Kernel& k) -> std::unique_ptr<IO_request>
{
    using viua::scheduler::io::IO_close_interaction;
    using viua::scheduler::io::IO_empty_interaction;
    using viua::scheduler::io::IO_interaction;

    auto const interaction_id =
        IO_interaction::id_type{file_descriptor, counter++};

    if (ownership == Ownership::Borrowed) {
        k.schedule_io(std::make_unique<IO_empty_interaction>(interaction_id));
        return std::make_unique<IO_request>(&k, interaction_id);
    }

    k.schedule_io(std::make_unique<IO_close_interaction>(interaction_id,
                                                         file_descriptor));
    return std::make_unique<IO_request>(&k, interaction_id);
}
auto IO_fd::close() -> void
{
    if (ownership == Ownership::Owned) {
        ::close(file_descriptor);
    }
}

IO_fd::IO_fd(int const x, Ownership const o) : file_descriptor{x}, ownership{o}
{}
IO_fd::~IO_fd()
{
    close();
}


std::string IO_request::type() const
{
    return type_name;
}
std::string IO_request::str() const
{
    auto const [a, b] = interaction_id;
    return "<I/O request: " + std::to_string(a) + "." + std::to_string(b) + ">";
}
std::string IO_request::repr() const
{
    return "<I/O request>";
}
bool IO_request::boolean() const
{
    return true;
}

std::unique_ptr<Value> IO_request::copy() const
{
    throw std::make_unique<viua::types::Exception>(
        viua::types::Exception::Tag{"IO_request"}, "not copyable");
}

IO_request::IO_request(viua::kernel::Kernel* k, interaction_id_type const x)
        : interaction_id{x}, kernel{k}
{}
IO_request::~IO_request()
{
    kernel->cancel_io(id());
}
}  // namespace viua::types
