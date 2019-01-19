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

#include <arpa/inet.h>  // for inet_pton(3), htons(3)
#include <string.h>     // for memset(3)
#include <sys/socket.h> // for socket(3)
                        //   , connect(3)
                        //   , listen(3)
                        //   , accept(3)
                        //   , shutdown(3)
                        //   , recv(3)
                        //   , setsockopt(3)
#include <unistd.h>     // for close(3), write(3), read(3)

#include <iostream>
#include <memory>
#include <string_view>

#include <viua/include/module.h>
#include <viua/types/exception.h>
#include <viua/types/integer.h>
#include <viua/types/string.h>


namespace viua {
namespace stdlib {
namespace posix {
namespace network {
static auto inet_ston(std::string const& s) -> uint32_t {
    auto address = uint32_t{0};
    auto const ret = inet_pton(AF_INET, s.c_str(), static_cast<void*>(&address));

    if (ret == 0) {
        throw std::make_unique<viua::types::Exception>("Misformatted_address");
    }
    if (ret == -1) {
        throw std::make_unique<viua::types::Exception>("EAFNOSUPPORT");
    }

    return address;
}

template<typename T>
auto memset(T& value, int const c) -> void {
    ::memset(&value, c, sizeof(std::remove_reference_t<T>));
}

static auto socket(Frame* frame,
                   viua::kernel::Register_set*,
                   viua::kernel::Register_set*,
                   viua::process::Process*,
                   viua::kernel::Kernel*) -> void {
    auto const sock = ::socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        auto const error_number = errno;
        auto const known_errors = std::map<decltype(error_number), std::string>{
            { EAFNOSUPPORT, "EAFNOSUPPORT", },
            { EMFILE, "EMFILE", },
            { ENFILE, "ENFILE", },
            { EPROTONOSUPPORT, "EPROTONOSUPPORT", },
            { EPROTOTYPE, "EPROTOTYPE", },
            { EACCES, "EACCES", },
            { ENOBUFS, "ENOBUFS", },
            { ENOMEM, "ENOMEM", },
        };
        if (not known_errors.count(error_number)) {
            throw std::make_unique<viua::types::Exception>("Unknown_errno");
        }
        throw std::make_unique<viua::types::Exception>(known_errors.at(error_number));
    }
    {
        auto const val = 1;
        auto const res = ::setsockopt(
              sock
            , SOL_SOCKET
            , SO_REUSEADDR
            , reinterpret_cast<void const*>(&val)
            , sizeof(val)
        );
        if (res == -1) {
            auto const error_number = errno;
            auto const known_errors = std::map<decltype(error_number), std::string>{
                { EBADF, "EAFNOSUPPORT", },
                { EDOM, "EDOM", },
                { EINVAL, "EINVAL", },
                { EISCONN, "EISCONN", },
                { ENOPROTOOPT, "ENOPROTOOPT", },
                { ENOTSOCK, "ENOTSOCK", },
                { ENOMEM, "ENOMEM", },
                { ENOBUFS, "ENOBUFS", },
            };
            if (not known_errors.count(error_number)) {
                throw std::make_unique<viua::types::Exception>("setsockopt(3): Unknown_errno: " + std::to_string(error_number));
            }
            throw std::make_unique<viua::types::Exception>(known_errors.at(error_number));
        }
    }

    frame->set_local_register_set(std::make_unique<viua::kernel::Register_set>(1));
    frame->local_register_set->set(0, std::make_unique<viua::types::Integer>(sock));
}

static auto connect(Frame* frame,
                   viua::kernel::Register_set*,
                   viua::kernel::Register_set*,
                   viua::process::Process*,
                   viua::kernel::Kernel*) -> void {
    sockaddr_in addr;
    memset(addr, 0);

    addr.sin_family = AF_INET;
    addr.sin_port =
        htons(static_cast<uint16_t>(
            static_cast<viua::types::Integer*>(
                frame->arguments->get(2))->as_integer()));
    addr.sin_addr.s_addr = inet_ston(frame->arguments->get(1)->str());

    auto const sock = static_cast<viua::types::Integer*>(frame->arguments->get(0))->as_integer();
    if (::connect(static_cast<int>(sock), reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == -1) {
        auto const error_number = errno;
        auto const known_errors = std::map<decltype(error_number), std::string>{
            { EADDRNOTAVAIL, "EADDRNOTAVAIL", },
            { EAFNOSUPPORT, "EAFNOSUPPORT", },
            { EALREADY, "EALREADY", },
            { EBADF, "EBADF", },
            { ECONNREFUSED, "ECONNREFUSED", },
            { EINPROGRESS, "EINPROGRESS", },
            { EINTR, "EINTR", },
            { EISCONN, "EISCONN", },
            { ENETUNREACH, "ENETUNREACH", },
            { ENOTSOCK, "ENOTSOCK", },
            { EPROTOTYPE, "EPROTOTYPE", },
            { ETIMEDOUT, "ETIMEDOUT", },
            { EIO, "EIO", },
            { ELOOP, "ELOOP", },
            { ENAMETOOLONG, "ENAMETOOLONG", },
            { ENOENT, "ENOENT", },
            { ENOTDIR, "ENOTDIR", },
            { EACCES, "EACCES", },
            { EADDRINUSE, "EADDRINUSE", },
            { ECONNRESET, "ECONNRESET", },
            { EHOSTUNREACH, "EHOSTUNREACH", },
            { EINVAL, "EINVAL", },
            { ENETDOWN, "ENETDOWN", },
            { ENOBUFS, "ENOBUFS", },
            { EOPNOTSUPP, "EOPNOTSUPP", },
        };
        if (not known_errors.count(error_number)) {
            throw std::make_unique<viua::types::Exception>("Unknown_errno");
        }
        throw std::make_unique<viua::types::Exception>(known_errors.at(error_number));
    }
}

static auto bind(Frame* frame,
                   viua::kernel::Register_set*,
                   viua::kernel::Register_set*,
                   viua::process::Process*,
                   viua::kernel::Kernel*) -> void {
    sockaddr_in addr;
    memset(addr, 0);

    addr.sin_family = AF_INET;
    addr.sin_port =
        htons(static_cast<uint16_t>(
            static_cast<viua::types::Integer*>(
                frame->arguments->get(2))->as_integer()));
    addr.sin_addr.s_addr = inet_ston(frame->arguments->get(1)->str());

    auto const sock = static_cast<viua::types::Integer*>(frame->arguments->get(0))->as_integer();
    if (::bind(static_cast<int>(sock), reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == -1) {
        auto const error_number = errno;
        auto const known_errors = std::map<decltype(error_number), std::string>{
            { EADDRNOTAVAIL, "EADDRNOTAVAIL", },
            { EAFNOSUPPORT, "EAFNOSUPPORT", },
            { EALREADY, "EALREADY", },
            { EBADF, "EBADF", },
            { ECONNREFUSED, "ECONNREFUSED", },
            { EINPROGRESS, "EINPROGRESS", },
            { EINTR, "EINTR", },
            { EISCONN, "EISCONN", },
            { ENETUNREACH, "ENETUNREACH", },
            { ENOTSOCK, "ENOTSOCK", },
            { EPROTOTYPE, "EPROTOTYPE", },
            { ETIMEDOUT, "ETIMEDOUT", },
            { EIO, "EIO", },
            { ELOOP, "ELOOP", },
            { ENAMETOOLONG, "ENAMETOOLONG", },
            { ENOENT, "ENOENT", },
            { ENOTDIR, "ENOTDIR", },
            { EACCES, "EACCES", },
            { EADDRINUSE, "EADDRINUSE", },
            { ECONNRESET, "ECONNRESET", },
            { EHOSTUNREACH, "EHOSTUNREACH", },
            { EINVAL, "EINVAL", },
            { ENETDOWN, "ENETDOWN", },
            { ENOBUFS, "ENOBUFS", },
            { EOPNOTSUPP, "EOPNOTSUPP", },
        };
        if (not known_errors.count(error_number)) {
            throw std::make_unique<viua::types::Exception>("Unknown_errno");
        }
        throw std::make_unique<viua::types::Exception>(known_errors.at(error_number));
    }
}

static auto listen(Frame* frame,
                   viua::kernel::Register_set*,
                   viua::kernel::Register_set*,
                   viua::process::Process*,
                   viua::kernel::Kernel*) -> void {
    auto const sock =
        static_cast<viua::types::Integer*>(
            frame->arguments->get(0))->as_integer();
    auto const backlog =
        static_cast<viua::types::Integer*>(
            frame->arguments->get(1))->as_integer();
    if (::listen(static_cast<int>(sock), static_cast<int>(backlog)) == -1) {
        auto const error_number = errno;
        auto const known_errors = std::map<decltype(error_number), std::string>{
            { EADDRNOTAVAIL, "EADDRNOTAVAIL", },
            { EAFNOSUPPORT, "EAFNOSUPPORT", },
            { EALREADY, "EALREADY", },
            { EBADF, "EBADF", },
            { ECONNREFUSED, "ECONNREFUSED", },
            { EINPROGRESS, "EINPROGRESS", },
            { EINTR, "EINTR", },
            { EISCONN, "EISCONN", },
            { ENETUNREACH, "ENETUNREACH", },
            { ENOTSOCK, "ENOTSOCK", },
            { EPROTOTYPE, "EPROTOTYPE", },
            { ETIMEDOUT, "ETIMEDOUT", },
            { EIO, "EIO", },
            { ELOOP, "ELOOP", },
            { ENAMETOOLONG, "ENAMETOOLONG", },
            { ENOENT, "ENOENT", },
            { ENOTDIR, "ENOTDIR", },
            { EACCES, "EACCES", },
            { EADDRINUSE, "EADDRINUSE", },
            { ECONNRESET, "ECONNRESET", },
            { EHOSTUNREACH, "EHOSTUNREACH", },
            { EINVAL, "EINVAL", },
            { ENETDOWN, "ENETDOWN", },
            { ENOBUFS, "ENOBUFS", },
            { EOPNOTSUPP, "EOPNOTSUPP", },
        };
        if (not known_errors.count(error_number)) {
            throw std::make_unique<viua::types::Exception>("Unknown_errno");
        }
        throw std::make_unique<viua::types::Exception>(known_errors.at(error_number));
    }
}

static auto accept(Frame* frame,
                   viua::kernel::Register_set*,
                   viua::kernel::Register_set*,
                   viua::process::Process*,
                   viua::kernel::Kernel*) -> void {
    auto const sock =
        static_cast<viua::types::Integer*>(
            frame->arguments->get(0))->as_integer();
    auto const incoming = ::accept(static_cast<int>(sock), nullptr, nullptr);
    if (incoming == -1) {
        auto const error_number = errno;
        auto const known_errors = std::map<decltype(error_number), std::string>{
            { EADDRNOTAVAIL, "EADDRNOTAVAIL", },
            { EAFNOSUPPORT, "EAFNOSUPPORT", },
            { EALREADY, "EALREADY", },
            { EBADF, "EBADF", },
            { ECONNREFUSED, "ECONNREFUSED", },
            { EINPROGRESS, "EINPROGRESS", },
            { EINTR, "EINTR", },
            { EISCONN, "EISCONN", },
            { ENETUNREACH, "ENETUNREACH", },
            { ENOTSOCK, "ENOTSOCK", },
            { EPROTOTYPE, "EPROTOTYPE", },
            { ETIMEDOUT, "ETIMEDOUT", },
            { EIO, "EIO", },
            { ELOOP, "ELOOP", },
            { ENAMETOOLONG, "ENAMETOOLONG", },
            { ENOENT, "ENOENT", },
            { ENOTDIR, "ENOTDIR", },
            { EACCES, "EACCES", },
            { EADDRINUSE, "EADDRINUSE", },
            { ECONNRESET, "ECONNRESET", },
            { EHOSTUNREACH, "EHOSTUNREACH", },
            { EINVAL, "EINVAL", },
            { ENETDOWN, "ENETDOWN", },
            { ENOBUFS, "ENOBUFS", },
            { EOPNOTSUPP, "EOPNOTSUPP", },
        };
        if (not known_errors.count(error_number)) {
            throw std::make_unique<viua::types::Exception>("accept(3): Unknown_errno: " + std::to_string(error_number));
        }
        throw std::make_unique<viua::types::Exception>(known_errors.at(error_number));
    }
    {
        timeval timeout_length;
        memset(timeout_length, 0);
        timeout_length.tv_usec = 500000;  // 500ms
        auto const res = ::setsockopt(
              incoming
            , SOL_SOCKET
            , SO_RCVTIMEO
            , reinterpret_cast<void const*>(&timeout_length)
            , sizeof(timeout_length)
        );
        if (res == -1) {
            auto const error_number = errno;
            auto const known_errors = std::map<decltype(error_number), std::string>{
                { EBADF, "EAFNOSUPPORT", },
                { EDOM, "EDOM", },
                { EINVAL, "EINVAL", },
                { EISCONN, "EISCONN", },
                { ENOPROTOOPT, "ENOPROTOOPT", },
                { ENOTSOCK, "ENOTSOCK", },
                { ENOMEM, "ENOMEM", },
                { ENOBUFS, "ENOBUFS", },
            };
            if (not known_errors.count(error_number)) {
                throw std::make_unique<viua::types::Exception>("setsockopt(3): Unknown_errno: " + std::to_string(error_number));
            }
            throw std::make_unique<viua::types::Exception>(known_errors.at(error_number));
        }
    }

    frame->set_local_register_set(std::make_unique<viua::kernel::Register_set>(1));
    frame->local_register_set->set(0, std::make_unique<viua::types::Integer>(incoming));
}

static auto write(Frame* frame,
                   viua::kernel::Register_set*,
                   viua::kernel::Register_set*,
                   viua::process::Process*,
                   viua::kernel::Kernel*) -> void {
    auto const sock = static_cast<int>(
        static_cast<viua::types::Integer*>(
            frame->arguments->get(0))->as_integer());

    auto const buffer = frame->arguments->get(1)->str();
    auto const written = ::write(sock, buffer.c_str(), buffer.size());

    if (written == -1) {
        auto const error_number = errno;
        auto const known_errors = std::map<decltype(error_number), std::string>{
            { EADDRNOTAVAIL, "EADDRNOTAVAIL", },
            { EAFNOSUPPORT, "EAFNOSUPPORT", },
            { EALREADY, "EALREADY", },
            { EBADF, "EBADF", },
            { ECONNREFUSED, "ECONNREFUSED", },
            { EINPROGRESS, "EINPROGRESS", },
            { EINTR, "EINTR", },
            { EISCONN, "EISCONN", },
            { ENETUNREACH, "ENETUNREACH", },
            { ENOTSOCK, "ENOTSOCK", },
            { EPROTOTYPE, "EPROTOTYPE", },
            { ETIMEDOUT, "ETIMEDOUT", },
            { EIO, "EIO", },
            { ELOOP, "ELOOP", },
            { ENAMETOOLONG, "ENAMETOOLONG", },
            { ENOENT, "ENOENT", },
            { ENOTDIR, "ENOTDIR", },
            { EACCES, "EACCES", },
            { EADDRINUSE, "EADDRINUSE", },
            { ECONNRESET, "ECONNRESET", },
            { EHOSTUNREACH, "EHOSTUNREACH", },
            { EINVAL, "EINVAL", },
            { ENETDOWN, "ENETDOWN", },
            { ENOBUFS, "ENOBUFS", },
            { EOPNOTSUPP, "EOPNOTSUPP", },
        };
        if (not known_errors.count(error_number)) {
            throw std::make_unique<viua::types::Exception>("Unknown_errno");
        }
        throw std::make_unique<viua::types::Exception>(known_errors.at(error_number));
    }

    frame->set_local_register_set(std::make_unique<viua::kernel::Register_set>(1));
    frame->local_register_set->set(0, std::make_unique<viua::types::Integer>(written));
}

static auto read(Frame* frame,
                   viua::kernel::Register_set*,
                   viua::kernel::Register_set*,
                   viua::process::Process*,
                   viua::kernel::Kernel*) -> void {
    auto const sock = static_cast<int>(
        static_cast<viua::types::Integer*>(
            frame->arguments->get(0))->as_integer());

    auto buffer = std::array<char, 1024>{};
    auto const n_bytes = ::read(sock, buffer.data(), buffer.size());

    if (n_bytes == 0) {
        throw std::make_unique<viua::types::Exception>("Eof", "end of file reached");
    }

    if (n_bytes == -1) {
        auto const error_number = errno;
        auto const known_errors = std::map<decltype(error_number), std::string>{
            { EADDRNOTAVAIL, "EADDRNOTAVAIL", },
            { EAFNOSUPPORT, "EAFNOSUPPORT", },
            { EALREADY, "EALREADY", },
            { EBADF, "EBADF", },
            { ECONNREFUSED, "ECONNREFUSED", },
            { EINPROGRESS, "EINPROGRESS", },
            { EINTR, "EINTR", },
            { EISCONN, "EISCONN", },
            { ENETUNREACH, "ENETUNREACH", },
            { ENOTSOCK, "ENOTSOCK", },
            { EPROTOTYPE, "EPROTOTYPE", },
            { ETIMEDOUT, "ETIMEDOUT", },
            { EIO, "EIO", },
            { ELOOP, "ELOOP", },
            { ENAMETOOLONG, "ENAMETOOLONG", },
            { ENOENT, "ENOENT", },
            { ENOTDIR, "ENOTDIR", },
            { EACCES, "EACCES", },
            { EADDRINUSE, "EADDRINUSE", },
            { ECONNRESET, "ECONNRESET", },
            { EHOSTUNREACH, "EHOSTUNREACH", },
            { EINVAL, "EINVAL", },
            { ENETDOWN, "ENETDOWN", },
            { ENOBUFS, "ENOBUFS", },
            { EOPNOTSUPP, "EOPNOTSUPP", },
        };
        if (not known_errors.count(error_number)) {
            throw std::make_unique<viua::types::Exception>("Unknown_errno");
        }
        throw std::make_unique<viua::types::Exception>(known_errors.at(error_number));
    }

    frame->set_local_register_set(std::make_unique<viua::kernel::Register_set>(1));
    frame->local_register_set->set(0, std::make_unique<viua::types::String>(
        std::string{buffer.data(), static_cast<std::string::size_type>(n_bytes)}
    ));
}

static auto recv(Frame* frame,
                 viua::kernel::Register_set*,
                 viua::kernel::Register_set*,
                 viua::process::Process*,
                 viua::kernel::Kernel*) -> void {
    auto const sock = static_cast<int>(
        static_cast<viua::types::Integer*>(
            frame->arguments->get(0))->as_integer());
    auto const buffer_length = static_cast<size_t>(
        static_cast<viua::types::Integer*>(
            frame->arguments->get(0))->as_integer());

    auto buffer = std::vector<char>(buffer_length, '\0');
    auto const n_bytes = ::recv(sock, buffer.data(), buffer.size(), 0);

    if (n_bytes == 0) {
        throw std::make_unique<viua::types::Exception>("Eof", "end of file reached");
    }
    if (n_bytes == -1) {
        auto const error_number = errno;
        if (error_number == EAGAIN) {
            throw std::make_unique<viua::types::Exception>("Eagain", "try again");
        }
        if (error_number == EWOULDBLOCK) {
            throw std::make_unique<viua::types::Exception>("Ewouldblock", "would block");
        }

        auto const known_errors = std::map<decltype(error_number), std::string>{
            { EADDRNOTAVAIL, "EADDRNOTAVAIL", },
            { EAFNOSUPPORT, "EAFNOSUPPORT", },
            { EALREADY, "EALREADY", },
            { EBADF, "EBADF", },
            { ECONNREFUSED, "ECONNREFUSED", },
            { EINPROGRESS, "EINPROGRESS", },
            { EINTR, "EINTR", },
            { EISCONN, "EISCONN", },
            { ENETUNREACH, "ENETUNREACH", },
            { ENOTSOCK, "ENOTSOCK", },
            { EPROTOTYPE, "EPROTOTYPE", },
            { ETIMEDOUT, "ETIMEDOUT", },
            { EIO, "EIO", },
            { ELOOP, "ELOOP", },
            { ENAMETOOLONG, "ENAMETOOLONG", },
            { ENOENT, "ENOENT", },
            { ENOTDIR, "ENOTDIR", },
            { EACCES, "EACCES", },
            { EADDRINUSE, "EADDRINUSE", },
            { ECONNRESET, "ECONNRESET", },
            { EHOSTUNREACH, "EHOSTUNREACH", },
            { EINVAL, "EINVAL", },
            { ENETDOWN, "ENETDOWN", },
            { ENOBUFS, "ENOBUFS", },
            { EOPNOTSUPP, "EOPNOTSUPP", },
        };
        if (not known_errors.count(error_number)) {
            throw std::make_unique<viua::types::Exception>("recv(3): Unknown_errno: " + std::to_string(error_number));
        }
        throw std::make_unique<viua::types::Exception>(known_errors.at(error_number));
    }

    frame->set_local_register_set(std::make_unique<viua::kernel::Register_set>(1));
    frame->local_register_set->set(0, std::make_unique<viua::types::String>(
        std::string{buffer.data(), static_cast<std::string::size_type>(n_bytes)}
    ));
}

static auto shutdown(Frame* frame,
                   viua::kernel::Register_set*,
                   viua::kernel::Register_set*,
                   viua::process::Process*,
                   viua::kernel::Kernel*) -> void {
    auto const sock = static_cast<viua::types::Integer*>(frame->arguments->get(0))->as_integer();
    // FIXME allow shutting down just SHUT_WR or SHUT_RD
    if (::shutdown(static_cast<int>(sock), SHUT_RDWR) == -1) {
        auto const error_number = errno;
        auto const known_errors = std::map<decltype(error_number), std::string>{
            { EBADF, "EBADF", },
            { EINVAL, "EINVAL", },
            { ENOTCONN, "ENOTCONN", },
            { ENOTSOCK, "ENOTSOCK", },
            { ENOBUFS, "ENOBUFS", },
        };
        if (not known_errors.count(error_number)) {
            throw std::make_unique<viua::types::Exception>("Unknown_errno");
        }
        throw std::make_unique<viua::types::Exception>(known_errors.at(error_number));
    }
}

static auto close(Frame* frame,
                   viua::kernel::Register_set*,
                   viua::kernel::Register_set*,
                   viua::process::Process*,
                   viua::kernel::Kernel*) -> void {
    auto const sock = static_cast<viua::types::Integer*>(frame->arguments->get(0))->as_integer();
    if (::close(static_cast<int>(sock)) == -1) {
        auto const error_number = errno;
        auto const known_errors = std::map<decltype(error_number), std::string>{
            { EBADF, "EBADF", },
            { EINTR, "EINTR", },
            { EIO, "EIO", },
        };
        if (not known_errors.count(error_number)) {
            throw std::make_unique<viua::types::Exception>("Unknown_errno");
        }
        throw std::make_unique<viua::types::Exception>(known_errors.at(error_number));
    }
}
}}}}

const Foreign_function_spec functions[] = {
    {"std::posix::network::socket/0", &viua::stdlib::posix::network::socket},
    {"std::posix::network::connect/3", &viua::stdlib::posix::network::connect},
    {"std::posix::network::bind/3", &viua::stdlib::posix::network::bind},
    {"std::posix::network::listen/2", &viua::stdlib::posix::network::listen},
    {"std::posix::network::accept/1", &viua::stdlib::posix::network::accept},
    {"std::posix::network::write/2", &viua::stdlib::posix::network::write},
    {"std::posix::network::read/1", &viua::stdlib::posix::network::read},
    {"std::posix::network::recv/2", &viua::stdlib::posix::network::recv},
    {"std::posix::network::shutdown/1", &viua::stdlib::posix::network::shutdown},
    {"std::posix::network::close/1", &viua::stdlib::posix::network::close},
    {nullptr, nullptr},
};

extern "C" const Foreign_function_spec* exports() {
    return functions;
}
