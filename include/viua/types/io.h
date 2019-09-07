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

#ifndef VIUA_TYPES_IO_H
#define VIUA_TYPES_IO_H

#include <string>
#include <viua/kernel/frame.h>
#include <viua/kernel/registerset.h>
#include <viua/scheduler/io/interactions.h>
#include <viua/support/string.h>
#include <viua/types/integer.h>
#include <viua/types/value.h>
#include <viua/types/vector.h>
#include <viua/process.h>


namespace viua { namespace types {
class IO_request : public Value {
    using interaction_id_type = viua::scheduler::io::IO_interaction::id_type;
    interaction_id_type const interaction_id;
    viua::kernel::Kernel* const kernel;
  public:
    static std::string const type_name;

    std::string type() const override;
    std::string str() const override;
    std::string repr() const override;
    bool boolean() const override;

    auto id() const -> interaction_id_type {
        return interaction_id;
    }

    std::unique_ptr<Value> copy() const override;

    IO_request(viua::kernel::Kernel*, interaction_id_type const);
    ~IO_request();
};

class IO_port : public Value {
  public:
      using counter_type = uint64_t;
  protected:
      counter_type counter = 0;
  public:
    static std::string const type_name;

    std::string type() const override;
    std::string str() const override;
    std::string repr() const override;
    bool boolean() const override;

    std::unique_ptr<Value> copy() const override;

    virtual auto read(
          viua::kernel::Kernel&
        , std::unique_ptr<Value>
    ) -> std::unique_ptr<IO_request> = 0;
    virtual auto write(
          viua::kernel::Kernel&
        , std::unique_ptr<Value>
    ) -> std::unique_ptr<IO_request> = 0;

    IO_port();
    ~IO_port();
};

class IO_fd : public IO_port {
  public:
    enum class Ownership : bool {
        Owned,
        Borrowed,
    };

  private:
    /*
     * The simples possible I/O port represents the "file descriptor" known from
     * the usual POSIX interfaces. It supports two basic operations: reading and
     * writing.
     */
    int const file_descriptor;
    Ownership const ownership;
  public:
    static std::string const type_name;

    std::string type() const override;
    std::string str() const override;
    std::string repr() const override;
    bool boolean() const override;

    std::unique_ptr<Value> copy() const override;

    auto fd() const -> int;
    auto read(viua::kernel::Kernel&, std::unique_ptr<Value>) -> std::unique_ptr<IO_request> override;
    auto write(viua::kernel::Kernel&, std::unique_ptr<Value>) -> std::unique_ptr<IO_request> override;

    IO_fd(int const, Ownership const = Ownership::Owned);
    ~IO_fd();
};
}}  // namespace viua::types


#endif
