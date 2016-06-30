#include <viua/cpu/cpu.h>
#include <viua/scheduler/vps.h>


viua::scheduler::VirtualProcessScheduler::VirtualProcessScheduler(CPU *attached_cpu): cpu(attached_cpu) {
}
