/*
 *  Copyright (C) 2017 Marek Marecki
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

#include <viua/types/exception.h>
#include <viua/kernel/kernel.h>
#include <viua/scheduler/vps.h>
#include <viua/process.h>
using namespace std;


viua::process::Stack::Stack(string fn, Process* pp, viua::kernel::RegisterSet** curs, viua::kernel::RegisterSet* gs, viua::scheduler::VirtualProcessScheduler* sch):
    entry_function(fn),
    parent_process(pp),
    jump_base(nullptr),
    instruction_pointer(nullptr),
    frame_new(nullptr),
    try_frame_new(nullptr),
    thrown(nullptr),
    caught(nullptr),
    currently_used_register_set(curs),
    global_register_set(gs),
    return_value(nullptr),
    scheduler(sch)
{
}

auto viua::process::Stack::bind(viua::kernel::RegisterSet** curs, viua::kernel::RegisterSet* gs) -> void {
    currently_used_register_set = curs;
    global_register_set = gs;
}

auto viua::process::Stack::begin() const -> decltype(frames.begin()) {
    return frames.begin();
}

auto viua::process::Stack::end() const -> decltype(frames.end()) {
    return frames.end();
}

auto viua::process::Stack::at(decltype(frames)::size_type i) const -> decltype(frames.at(i)) {
    return frames.at(i);
}

auto viua::process::Stack::back() const -> decltype(frames.back()) {
    return frames.back();
}

auto viua::process::Stack::register_deferred_calls(const bool push_this_stack) -> void {
    // Mark current stack as the one to return to after all
    // the deferred calls complete, *but* only if the stack is not exhausted as
    // there is no reason to return to such stacks.
    if (push_this_stack and size() > 1) {
        parent_process->stacks_order.push(this);
    }

    for (auto& each : back()->deferred_calls) {
        unique_ptr<Stack> s { new Stack ( each->function_name, parent_process, currently_used_register_set, global_register_set, scheduler ) };
        s->emplace_back(std::move(each));
        s->instruction_pointer = adjust_jump_base_for(s->at(0)->function_name);
        s->bind(currently_used_register_set, global_register_set);
        parent_process->stacks_order.push(s.get());
        parent_process->stacks[s.get()] = std::move(s);
    }

    // remember to clear deferred calls vector to avoid
    // accidentally deferring a frame twice!
    back()->deferred_calls.clear();
}

auto viua::process::Stack::pop() -> unique_ptr<Frame> {
    unique_ptr<Frame> frame { std::move(frames.back()) };
    frames.pop_back();

    for (viua::internals::types::register_index i = 0; i < frame->arguments->size(); ++i) {
        if (frame->arguments->at(i) != nullptr and frame->arguments->isflagged(i, MOVED)) {
            throw new viua::types::Exception("unused pass-by-move parameter");
        }
    }

    if (size() == 0) {
        return_value = frame->local_register_set->pop(0);
    }

    if (size()) {
        *currently_used_register_set = back()->local_register_set.get();
    } else {
        *currently_used_register_set = global_register_set;
    }

    return frame;
}

auto viua::process::Stack::size() const -> decltype(frames)::size_type {
    return frames.size();
}

auto viua::process::Stack::clear() -> void {
    frames.clear();
}

auto viua::process::Stack::emplace_back(unique_ptr<Frame> frame) -> decltype(frames.emplace_back(frame)) {
    return frames.emplace_back(std::move(frame));
}

viua::internals::types::byte* viua::process::Stack::adjust_jump_base_for_block(const string& call_name) {
    viua::internals::types::byte *entry_point = nullptr;
    auto ep = scheduler->getEntryPointOfBlock(call_name);
    entry_point = ep.first;
    jump_base = ep.second;
    return entry_point;
}
viua::internals::types::byte* viua::process::Stack::adjust_jump_base_for(const string& call_name) {
    viua::internals::types::byte *entry_point = nullptr;
    auto ep = scheduler->getEntryPointOf(call_name);
    entry_point = ep.first;
    jump_base = ep.second;
    return entry_point;
}

auto viua::process::Stack::adjust_instruction_pointer(TryFrame* tframe, string handler_found_for_type) -> void {
    instruction_pointer = adjust_jump_base_for_block(tframe->catchers.at(handler_found_for_type)->catcher_name);
}
auto viua::process::Stack::unwind_call_stack_to(TryFrame* tframe) -> void {
    size_type distance = 0;
    for (size_type j = (size()-1); j > 1; --j) {
        if (at(j).get() == tframe->associated_frame) {
            break;
        }
        ++distance;
    }

    parent_process->stacks_order.push(this);
    for (size_type i = 0; i < distance; --i) {
        const auto frame_index = (size() - i - 1);
        for (auto& each : at(frame_index)->deferred_calls) {
            unique_ptr<Stack> s { new Stack ( each->function_name, parent_process, currently_used_register_set, global_register_set, scheduler ) };
            s->emplace_back(std::move(each));
            s->instruction_pointer = adjust_jump_base_for(s->at(0)->function_name);
            s->bind(currently_used_register_set, global_register_set);
            parent_process->stacks_order.push(s.get());
            parent_process->stacks[s.get()] = std::move(s);
        }
        at(frame_index)->deferred_calls.clear();
    }

    for (size_type j = 0; j < distance; ++j) {
        pop();
    }

    if (not parent_process->stacks_order.empty()) {
        parent_process->stack = parent_process->stacks_order.top();
        parent_process->stacks_order.pop();
        parent_process->currently_used_register_set = parent_process->stack->back()->local_register_set.get();
    }
}
auto viua::process::Stack::unwind_try_stack_to(TryFrame* tframe) -> void {
    while (tryframes.back().get() != tframe) {
        tryframes.pop_back();
    }
}

auto viua::process::Stack::unwind_to(TryFrame* tframe, string handler_found_for_type) -> void {
    adjust_instruction_pointer(tframe, handler_found_for_type);
    unwind_call_stack_to(tframe);
    unwind_try_stack_to(tframe);
}

auto viua::process::Stack::find_catch_frame() -> tuple<TryFrame*, string> {
    TryFrame* found_exception_frame = nullptr;
    string caught_with_type = "";

    for (decltype(tryframes)::size_type i = tryframes.size(); i > 0; --i) {
        TryFrame* tframe = tryframes[(i-1)].get();
        string handler_found_for_type = thrown->type();
        bool handler_found = tframe->catchers.count(handler_found_for_type);

        // FIXME: mutex
        if ((not handler_found) and scheduler->isClass(handler_found_for_type)) {
            vector<string> types_to_check = scheduler->inheritanceChainOf(handler_found_for_type);
            for (decltype(types_to_check)::size_type j = 0; j < types_to_check.size(); ++j) {
                if (tframe->catchers.count(types_to_check[j])) {
                    handler_found = true;
                    handler_found_for_type = types_to_check[j];
                    break;
                }
            }
        }

        if (handler_found) {
            found_exception_frame = tframe;
            caught_with_type = handler_found_for_type;
            break;
        }
    }

    return tuple<TryFrame*, string>(found_exception_frame, caught_with_type);
}

auto viua::process::Stack::unwind() -> void {
    TryFrame* tframe = nullptr;
    string handler_found_for_type = "";

    tie(tframe, handler_found_for_type) = find_catch_frame();
    if (tframe != nullptr) {
        unwind_to(tframe, handler_found_for_type);
        caught = std::move(thrown);
    } else {
        parent_process->stacks_order.push(this);
        for (auto i = (size()-1); i; --i) {
            for (auto& each : at(i)->deferred_calls) {
                unique_ptr<Stack> s { new Stack ( each->function_name, parent_process, currently_used_register_set, global_register_set, scheduler ) };
                s->emplace_back(std::move(each));
                s->instruction_pointer = adjust_jump_base_for(s->at(0)->function_name);
                s->bind(currently_used_register_set, global_register_set);
                parent_process->stacks_order.push(s.get());
                parent_process->stacks[s.get()] = std::move(s);
            }
            at(i)->deferred_calls.clear();
        }
        if (not parent_process->stacks_order.empty()) {
            parent_process->stack = parent_process->stacks_order.top();
            parent_process->stacks_order.pop();
            parent_process->currently_used_register_set = parent_process->stack->back()->local_register_set.get();
        }
    }
}

auto viua::process::Stack::prepare_frame(viua::internals::types::register_index arguments_size, viua::internals::types::register_index registers_size) -> Frame* {
    if (frame_new) { throw "requested new frame while last one is unused"; }
    frame_new.reset(new Frame(nullptr, arguments_size, registers_size));
    return frame_new.get();
}

auto viua::process::Stack::push_prepared_frame() -> void {
    if (size() > MAX_STACK_SIZE) {
        ostringstream oss;
        oss << "stack size (" << MAX_STACK_SIZE << ") exceeded with call to '" << frame_new->function_name << '\'';
        throw new viua::types::Exception(oss.str());
    }

    *currently_used_register_set = frame_new->local_register_set.get();
    if (find(begin(), end(), frame_new) != end()) {
        ostringstream oss;
        oss << "stack corruption: frame ";
        oss << hex << frame_new.get() << dec;
        oss << " for function " << frame_new->function_name << '/' << frame_new->arguments->size();
        oss << " pushed more than once";
        throw oss.str();
    }
    emplace_back(std::move(frame_new));
}
