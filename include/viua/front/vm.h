#include <string>
#include <vector>
#include <viua/support/string.h>
#include <viua/support/env.h>
#include <viua/types/pointer.h>
#include <viua/types/exception.h>
#include <viua/types/string.h>
#include <viua/types/process.h>
#include <viua/loader.h>
#include <viua/cpu/cpu.h>


namespace viua {
    namespace front {
        namespace vm {
            void initialise(CPU*, const std::string&, std::vector<std::string>);
            void load_standard_prototypes(CPU*);
            void preload_libraries(CPU*);
        }
    }
}
