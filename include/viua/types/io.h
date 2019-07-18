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
#include <viua/support/string.h>
#include <viua/types/integer.h>
#include <viua/types/value.h>
#include <viua/types/vector.h>
#include <viua/process.h>


namespace viua { namespace types {
struct IO_interaction {
    using id_type = std::tuple<uint64_t, uint64_t>;

    enum class State : uint8_t {
        Queued,
        In_flight,
        Complete,
    };
    enum class Status : uint8_t {
        Success,
        Error,
        Cancelled,
    };

  private:
    id_type const assigned_id;
    viua::process::Process* const from_process;

    std::atomic_bool is_complete = false;
    std::atomic_bool is_cancelled = false;
    std::atomic_bool is_aborted = false;

  public:
    /*
     * This function should try to perform the I/O interaction
     * represented by this object.
     *
     * RETURN VALUE
     *
     * If the function returns a non-null pointer it is the return value
     * of the interaction and the interaction is finished; otherwise,
     * the interaction is not finished and must the scheduler must try
     * to run it again.
     *
     * The interaction is responsible for any buffering necessary to
     * produce a return value. For example, if the port that produced
     * the interaction returns 1000-byte blocks of data and the
     * interaction only managed to read 800 bytes it must buffer them
     * and not return until manages to read 200 bytes more.
     *
     * BLOCKING
     *
     * The interact function MUST NOT block.
     *
     * CANCELLING
     *
     * Interactions react differently to being cancelled. Some may
     * return partial results, some just flat out crash and return an
     * exception.
     *
     * POSTPROCESSING
     *
     * Keep any postprocessing to the absolute minimum. Returning raw
     * data and offering a function to process it outside of I/O
     * scheduler is a much better solution. This means, though, that
     * sometimes the I/O may be split into several functions. Spawn a
     * special-purpose actor if it too cumbersome to manage in-line.
     */
    struct Interaction_result {
        using result_type = std::unique_ptr<viua::types::Value>;

        State const state = State::In_flight;
        std::optional<Status> const status;
        std::unique_ptr<viua::types::Value> result;

        Interaction_result() = default;
        Interaction_result(State const, Status const, result_type);
    };
    virtual auto interact() -> Interaction_result = 0;

    auto id() const -> id_type;

    auto cancel() -> void;
    auto cancelled() const -> bool;

    auto abort() -> void;
    auto aborted() const -> bool;

    auto complete() -> void;
    auto completed() const -> bool;

    IO_interaction(viua::process::Process* const, id_type const);
    virtual ~IO_interaction();
};
struct IO_fake_interaction : public IO_interaction {
    auto interact() -> Interaction_result override;

    using IO_interaction::IO_interaction;
};
struct IO_read_interaction : public IO_interaction {
    int const file_descriptor;
    std::string buffer;

    auto interact() -> Interaction_result override;

    IO_read_interaction(viua::process::Process* const, id_type const, int const, size_t const);
};

class IO_request : public Value {
    IO_interaction::id_type const interaction_id;
  public:
    static std::string const type_name;

    std::string type() const override;
    std::string str() const override;
    std::string repr() const override;
    bool boolean() const override;

    auto id() const -> IO_interaction::id_type {
        return interaction_id;
    }

    std::unique_ptr<Value> copy() const override;

    IO_request(IO_interaction::id_type const);
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
          viua::process::Process&
        , std::unique_ptr<Value>
    ) -> std::unique_ptr<IO_request> = 0;

    IO_port();
    ~IO_port();
};

class IO_fd : public IO_port {
    /*
     * The simples possible I/O port represents the "file descriptor" known from
     * the usual POSIX interfaces. It supports two basic operations: reading and
     * writing.
     */
    int const file_descriptor;
  public:
    static std::string const type_name;

    std::string type() const override;
    std::string str() const override;
    std::string repr() const override;
    bool boolean() const override;

    std::unique_ptr<Value> copy() const override;

    auto read(viua::process::Process&, std::unique_ptr<Value>) -> std::unique_ptr<IO_request> override;
    auto write(std::string) -> int;

    IO_fd(int const);
    ~IO_fd();
};
}}  // namespace viua::types


#endif
