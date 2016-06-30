#include <vector>
#include <string>
#include <viua/machine.h>
#include <viua/types/vector.h>
#include <viua/types/string.h>
#include <viua/process.h>
#include <viua/cpu/cpu.h>
#include <viua/scheduler/vps.h>
using namespace std;


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
