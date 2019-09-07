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

#ifndef VIUA_SCHEDULER_IO_INTERACTIONS_H
#define VIUA_SCHEDULER_IO_INTERACTIONS_H

#include <atomic>
#include <optional>

namespace viua { namespace scheduler { namespace io {
enum class IO_kind : uint8_t {
    Input,
    Output,
};

struct IO_interaction {
    using id_type = std::tuple<uint64_t, uint64_t>;
    using fd_type = int;

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
    virtual auto fd() const -> std::optional<fd_type> { return std::nullopt; }
    virtual auto kind() const -> IO_kind = 0;

    auto cancel() -> void;
    auto cancelled() const -> bool;

    auto abort() -> void;
    auto aborted() const -> bool;

    auto complete() -> void;
    auto completed() const -> bool;

    IO_interaction(id_type const);
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

    auto fd() const -> std::optional<fd_type> { return file_descriptor; }
    auto kind() const -> IO_kind { return IO_kind::Input; }

    IO_read_interaction(id_type const, int const, size_t const);
};
struct IO_write_interaction : public IO_interaction {
    int const file_descriptor;
    std::string const buffer;

    auto interact() -> Interaction_result override;

    auto fd() const -> std::optional<fd_type> { return file_descriptor; }
    auto kind() const -> IO_kind { return IO_kind::Output; }

    IO_write_interaction(id_type const, int const, std::string);
};
}}}

#endif
