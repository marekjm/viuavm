/*
 *  Copyright (C) 2015-2020 Marek Marecki
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

#include <memory>
#include <viua/exceptions.h>
#include <viua/kernel/kernel.h>
#include <viua/scheduler/process.h>
#include <viua/types/atom.h>
#include <viua/types/exception.h>
#include <viua/types/integer.h>


auto viua::process::Process::optry(Op_address_type addr) -> Op_address_type
{
    if (stack->try_frame_new) {
        throw std::make_unique<viua::types::Exception>(
            "new try frame requested while last one is unused");
    }
    stack->try_frame_new = std::make_unique<Try_frame>();
    return addr;
}

auto viua::process::Process::opcatch(Op_address_type addr) -> Op_address_type
{
    auto tag                = decoder.fetch_string(addr);
    auto catcher_block_name = decoder.fetch_string(addr);

    if (not attached_scheduler->is_block(catcher_block_name)) {
        throw std::make_unique<viua::types::Exception>(
            "registering undefined handler block '" + catcher_block_name
            + "' to handle " + tag);
    }

    stack->try_frame_new->catchers[tag] =
        std::make_unique<Catcher>(tag, catcher_block_name);

    return addr;
}

auto viua::process::Process::opdraw(Op_address_type addr) -> Op_address_type
{
    if (auto target = decoder.fetch_register_or_void(addr, *this);
        target.has_value()) {
        if (not stack->caught) {
            throw std::make_unique<viua::types::Exception>(
                "no caught object to draw");
        }
        **target = std::move(stack->caught);
    } else {
        /*
         * Drawing thrown objects into a void register will just discard them.
         */
        stack->caught.reset(nullptr);
    }

    return addr;
}

auto viua::process::Process::openter(Op_address_type addr) -> Op_address_type
{
    auto const block_name = decoder.fetch_string(addr);

    if (not attached_scheduler->is_block(block_name)) {
        throw std::make_unique<viua::types::Exception>(
            "cannot enter undefined block: " + block_name);
    }

    auto block_address = adjust_jump_base_for_block(block_name);

    stack->try_frame_new->return_address   = addr;
    stack->try_frame_new->associated_frame = stack->back().get();
    stack->try_frame_new->block_name       = block_name;

    stack->tryframes.emplace_back(std::move(stack->try_frame_new));

    return block_address;
}

auto viua::process::Process::opthrow(Op_address_type addr) -> Op_address_type
{
    auto source = decoder.fetch_register(addr, *this);

    if (source->empty()) {
        throw std::make_unique<viua::types::Exception>("throw from null register");
    }

    auto value = source->give();
    if (dynamic_cast<viua::types::Exception*>(value.get())) {
        auto ex = std::unique_ptr<viua::types::Exception>{};
        ex.reset(static_cast<viua::types::Exception*>(value.release()));
        throw ex;
    }

    throw std::make_unique<viua::types::Exception>(std::move(value));
}

auto viua::process::Process::opleave(Op_address_type addr) -> Op_address_type
{
    if (stack->tryframes.size() == 0) {
        throw std::make_unique<viua::types::Exception>(
            "bad leave: no block has been entered");
    }
    addr = stack->tryframes.back()->return_address;
    stack->tryframes.pop_back();

    if (stack->size() > 0) {
        adjust_jump_base_for(stack->back()->function_name);
    }
    return addr;
}

auto viua::process::Process::op_exception(Op_address_type addr) -> Op_address_type
{
    auto target = decoder.fetch_register(addr, *this);
    auto const tag = decoder.fetch_value_of<viua::types::Atom>(addr, *this);
    auto const value = decoder.fetch_register_or_void(addr, *this);

    using viua::types::Exception;
    auto ex = (value.has_value()
        ? std::make_unique<Exception>(Exception::Tag{std::string{*tag}}, (*value)->give())
        : std::make_unique<Exception>(Exception::Tag{std::string{*tag}}));

    *target = std::move(ex);

    return addr;
}

auto viua::process::Process::op_exception_tag(Op_address_type addr) -> Op_address_type
{
    auto target = decoder.fetch_register(addr, *this);
    auto const ex = decoder.fetch_value_of<viua::types::Exception>(addr, *this);

    *target = std::make_unique<viua::types::Atom>(ex->tag);

    return addr;
}

auto viua::process::Process::op_exception_value(Op_address_type addr) -> Op_address_type
{
    auto target = decoder.fetch_register(addr, *this);
    auto ex = decoder.fetch_value_of<viua::types::Exception>(addr, *this);

    if (not ex->value) {
        using viua::types::Exception;
        throw std::make_unique<Exception>(
            Exception::Tag{"Empty_exception"}, "exception has no value");
    }

    /*
     * The value is moved out of the exception so we avoid a copy operation, but
     * effectively destroy the exception. Is there any downside to this except
     * the fact that the exception must be re-constructed if it needs to be
     * rethrown?
     */
    *target = std::move(ex->value);

    return addr;
}
