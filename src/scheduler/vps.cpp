#include <vector>
#include <string>
#include <viua/machine.h>
#include <viua/types/vector.h>
#include <viua/types/string.h>
#include <viua/process.h>
#include <viua/cpu/cpu.h>
#include <viua/scheduler/vps.h>
using namespace std;


bool viua::scheduler::VirtualProcessScheduler::executeQuant(Process *th, unsigned priority) {
    if (th->stopped() and th->joinable()) {
        // stopped but still joinable
        // we don't have to deal with "stopped and unjoinable" case here
        // because it is handled later (after ticking code)
        return false;
    }
    if (th->suspended()) {
        // do not execute suspended processes
        return true;
    }

    for (unsigned j = 0; (priority == 0 or j < priority); ++j) {
        if (th->stopped()) {
            // remember to break if the process stopped
            // otherwise the CPU will try to tick the process and
            // it will crash (will try to execute instructions from 0x0 pointer)
            break;
        }
        if (th->suspended()) {
            // do not execute suspended processes
            break;
        }
        th->tick();
    }

    return true;
}

Process* viua::scheduler::VirtualProcessScheduler::bootstrap(const vector<string>& commandline_arguments, byte *jump_base) {
    Frame *initial_frame = new Frame(nullptr, 0, 2);
    initial_frame->function_name = ENTRY_FUNCTION_NAME;

    Vector* cmdline = new Vector();
    auto limit = commandline_arguments.size();
    for (decltype(limit) i = 0; i < limit; ++i) {
        cmdline->push(new String(commandline_arguments[i]));
    }
    initial_frame->regset->set(1, cmdline);

    Process* t = new Process(initial_frame, cpu, jump_base, nullptr);
    t->detach();
    t->priority(16);
    t->begin();

    return t;
}


viua::scheduler::VirtualProcessScheduler::VirtualProcessScheduler(CPU *attached_cpu): cpu(attached_cpu) {
}
