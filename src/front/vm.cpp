#include <viua/front/vm.h>
using namespace std;


void viua::front::vm::initialise(CPU *cpu, const string& program, vector<string> args) {
    Loader loader(program);
    loader.executable();

    uint64_t bytes = loader.getBytecodeSize();
    byte* bytecode = loader.getBytecode();

    map<string, uint64_t> function_address_mapping = loader.getFunctionAddresses();
    for (auto p : function_address_mapping) { cpu->mapfunction(p.first, p.second); }
    for (auto p : loader.getBlockAddresses()) { cpu->mapblock(p.first, p.second); }

    cpu->commandline_arguments = args;

    cpu->load(bytecode).bytes(bytes);
}

void viua::front::vm::load_standard_prototypes(CPU* cpu) {
    Prototype* proto_object = new Prototype("Object");
    cpu->registerForeignPrototype("Object", proto_object);

    Prototype* proto_string = new Prototype("String");
    proto_string->attach("String::stringify", "stringify");
    proto_string->attach("String::represent", "represent");
    proto_string->attach("String::startswith", "startswith");
    proto_string->attach("String::endswith", "endswith");
    proto_string->attach("String::format", "format");
    proto_string->attach("String::substr", "substr");
    proto_string->attach("String::concatenate", "concatenate");
    proto_string->attach("String::join", "join");
    proto_string->attach("String::size", "size");
    cpu->registerForeignPrototype("String", proto_string);
    cpu->registerForeignMethod("String::stringify", static_cast<ForeignMethodMemberPointer>(&String::stringify));
    cpu->registerForeignMethod("String::represent", static_cast<ForeignMethodMemberPointer>(&String::represent));
    cpu->registerForeignMethod("String::startswith", static_cast<ForeignMethodMemberPointer>(&String::startswith));
    cpu->registerForeignMethod("String::endswith", static_cast<ForeignMethodMemberPointer>(&String::endswith));
    cpu->registerForeignMethod("String::format", static_cast<ForeignMethodMemberPointer>(&String::format));
    cpu->registerForeignMethod("String::substr", static_cast<ForeignMethodMemberPointer>(&String::substr));
    cpu->registerForeignMethod("String::concatenate", static_cast<ForeignMethodMemberPointer>(&String::concatenate));
    cpu->registerForeignMethod("String::join", static_cast<ForeignMethodMemberPointer>(&String::join));
    cpu->registerForeignMethod("String::size", static_cast<ForeignMethodMemberPointer>(&String::size));

    Prototype* proto_process = new Prototype("Process");
    proto_process->attach("Process::joinable", "joinable/1");
    proto_process->attach("Process::detach", "detach/1");
    proto_process->attach("Process::suspend", "suspend/1");
    proto_process->attach("Process::wakeup", "wakeup/1");
    proto_process->attach("Process::suspended", "suspended/1");
    proto_process->attach("Process::getPriority", "getPriority/1");
    proto_process->attach("Process::setPriority", "setPriority/2");
    proto_process->attach("Process::pass", "pass/2");
    cpu->registerForeignPrototype("Process", proto_process);
    cpu->registerForeignMethod("Process::joinable", static_cast<ForeignMethodMemberPointer>(&ProcessType::joinable));
    cpu->registerForeignMethod("Process::detach", static_cast<ForeignMethodMemberPointer>(&ProcessType::detach));
    cpu->registerForeignMethod("Process::suspend", static_cast<ForeignMethodMemberPointer>(&ProcessType::suspend));
    cpu->registerForeignMethod("Process::wakeup", static_cast<ForeignMethodMemberPointer>(&ProcessType::wakeup));
    cpu->registerForeignMethod("Process::suspended", static_cast<ForeignMethodMemberPointer>(&ProcessType::suspended));
    cpu->registerForeignMethod("Process::getPriority", static_cast<ForeignMethodMemberPointer>(&ProcessType::getPriority));
    cpu->registerForeignMethod("Process::setPriority", static_cast<ForeignMethodMemberPointer>(&ProcessType::setPriority));
    cpu->registerForeignMethod("Process::pass", static_cast<ForeignMethodMemberPointer>(&ProcessType::pass));

    Prototype* proto_pointer = new Prototype("Pointer");
    proto_pointer->attach("Pointer::expired", "expired");
    cpu->registerForeignPrototype("Pointer", proto_pointer);
    cpu->registerForeignMethod("Pointer::expired", static_cast<ForeignMethodMemberPointer>(&Pointer::expired));
}

void viua::front::vm::preload_libraries(CPU* cpu) {
    /** This method preloads dynamic libraries specified by environment.
     */
    vector<string> preload_native = support::env::getpaths("VIUAPRELINK");
    for (unsigned i = 0; i < preload_native.size(); ++i) {
        cpu->loadNativeLibrary(preload_native[i]);
    }

    vector<string> preload_foreign = support::env::getpaths("VIUAPREIMPORT");
    for (unsigned i = 0; i < preload_foreign.size(); ++i) {
        cpu->loadForeignLibrary(preload_foreign[i]);
    }
}
