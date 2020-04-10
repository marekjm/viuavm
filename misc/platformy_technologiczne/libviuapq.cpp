/*
 *  Copyright (C) 2020 Marek Marecki
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

#include "libpq-fe.h"

#include <cstdlib>
#include <functional>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>
#include <viua/include/module.h>
#include <viua/kernel/frame.h>
#include <viua/kernel/registerset.h>
#include <viua/types/boolean.h>
#include <viua/types/exception.h>
#include <viua/types/integer.h>
#include <viua/types/pointer.h>
#include <viua/types/string.h>
#include <viua/types/struct.h>
#include <viua/types/value.h>
#include <viua/types/vector.h>


class PQ_connection : public viua::types::Value {
    PGconn* connection;

  public:
    auto type() const -> std::string override;
    auto str() const -> std::string override;
    auto repr() const -> std::string override;
    auto boolean() const -> bool override;

    auto copy() const -> std::unique_ptr<Value> override;

    auto get() -> PGconn*;

    PQ_connection(PGconn*);
};

auto PQ_connection::type() const -> std::string
{
    return std::string{"PQ_connection"};
}
auto PQ_connection::str() const -> std::string
{
    return std::string{"PQ_connection"};
}
auto PQ_connection::repr() const -> std::string
{
    return std::string{"PQ_connection"};
}
auto PQ_connection::boolean() const -> bool
{
    return true;
}

auto PQ_connection::copy() const -> std::unique_ptr<Value>
{
    throw std::make_unique<viua::types::Exception>("not copyable");
}

auto PQ_connection::get() -> PGconn*
{
    return connection;
}

PQ_connection::PQ_connection(PGconn* c)
    : connection{c}
{}


static void viuapq_connect(Frame* frame,
                      viua::kernel::Register_set*,
                      viua::kernel::Register_set*,
                      viua::process::Process*,
                      viua::kernel::Kernel*)
{
    auto const connection_info = frame->arguments->get(0)->str();

    auto connection = PQconnectdb(connection_info.c_str());
    if (PQstatus(connection) != CONNECTION_OK) {
        auto const error = PQerrorMessage(connection);
        PQfinish(connection);
        throw std::make_unique<viua::types::Exception>(error);
    }

    frame->set_local_register_set(
        std::make_unique<viua::kernel::Register_set>(1));
    frame->local_register_set->set(0,
                                   std::make_unique<PQ_connection>(connection));
}

static auto field_names_of(PGresult* result) -> std::vector<std::string>
{
    auto const n_fields = PQnfields(result);
    using pq_nfields_size_type = std::remove_const_t<decltype(n_fields)>;

    auto fields = std::vector<std::string>{};
    for (auto i = pq_nfields_size_type{0}; i < n_fields; ++i) {
        fields.push_back(PQfname(result, i));
    }

    return fields;
}
static auto for_each_tuple(PGresult* result, std::function<void(PGresult* const, int const)> fn) -> void
{
    auto const n_tuples = PQntuples(result);
    using pq_ntuples_size_type = std::remove_const_t<decltype(n_tuples)>;

    for (auto i = pq_ntuples_size_type{0}; i < n_tuples; ++i) {
        fn(result, i);
    }
}
static auto results_of(PGresult* result) -> std::vector<std::map<std::string, std::string>>
{
    auto fields = field_names_of(result);

    auto results = std::vector<std::map<std::string, std::string>>{};
    for_each_tuple(result, [fields, &results](PGresult* const res, int const n) -> void
    {
        auto r = decltype(results)::value_type{};
        for (auto i = size_t{0}; i < fields.size(); ++i) {
            r[fields[i]] = PQgetvalue(res, n, i);
        }
        results.push_back(std::move(r));
    });

    return results;
}

static void viuapq_get(Frame* frame,
                      viua::kernel::Register_set*,
                      viua::kernel::Register_set*,
                      viua::process::Process* proc,
                      viua::kernel::Kernel*)
{
    auto const connection = static_cast<PQ_connection*>(
        static_cast<viua::types::Pointer*>(frame->arguments->get(0))->to(proc));
    auto const query = frame->arguments->get(1)->str();

    auto const res = PQexec(connection->get(), query.c_str());
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        auto const error = PQerrorMessage(connection->get());
        throw std::make_unique<viua::types::Exception>(error);
    }

    auto ret = std::make_unique<viua::types::Vector>();
    for (auto const& each : results_of(res)) {
        auto item = std::make_unique<viua::types::Struct>();
        for (auto const& [ k, v ] : each) {
            item->insert(k, viua::types::String::make(v));
        }
        ret->push(std::move(item));
    }

    PQclear(res);

    frame->set_local_register_set(
        std::make_unique<viua::kernel::Register_set>(1));
    frame->local_register_set->set(0, std::move(ret));
}

static void viuapq_get_one(Frame* frame,
                      viua::kernel::Register_set*,
                      viua::kernel::Register_set*,
                      viua::process::Process* proc,
                      viua::kernel::Kernel*)
{
    auto const connection = static_cast<PQ_connection*>(
        static_cast<viua::types::Pointer*>(frame->arguments->get(0))->to(proc));
    auto const query = frame->arguments->get(1)->str();

    auto const res = PQexec(connection->get(), query.c_str());
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        auto const error = PQerrorMessage(connection->get());
        throw std::make_unique<viua::types::Exception>(error);
    }

    auto ret = results_of(res);
    auto item = std::make_unique<viua::types::Struct>();
    for (auto const& [ k, v ] : ret.front()) {
        item->insert(k, viua::types::String::make(v));
    }

    PQclear(res);

    frame->set_local_register_set(
        std::make_unique<viua::kernel::Register_set>(1));
    frame->local_register_set->set(0, std::move(item));
}

static void viuapq_finish(Frame* frame,
                      viua::kernel::Register_set*,
                      viua::kernel::Register_set*,
                      viua::process::Process*,
                      viua::kernel::Kernel*)
{
    auto const connection = static_cast<PQ_connection*>(frame->arguments->get(0));

    PQfinish(connection->get());
}


const Foreign_function_spec functions[] = {
    {"viuapq::connect/1", viuapq_connect},
    {"viuapq::get/2", viuapq_get},
    {"viuapq::get_one/2", viuapq_get_one},
    {"viuapq::finish/1", viuapq_finish},
    {nullptr, nullptr},
};

extern "C" const Foreign_function_spec* exports() { return functions; }
