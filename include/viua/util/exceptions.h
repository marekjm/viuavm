#ifndef VIUA_UTIL_EXCEPTIONS_H
#define VIUA_UTIL_EXCEPTIONS_H

#include <memory>
#include <viua/types/exception.h>

namespace viua {
    namespace util {
        namespace exceptions {
            template<typename Ex, typename... Ts>
                auto make_unique_exception(Ts&&... args) -> std::unique_ptr<viua::types::Exception> {
                    auto e = std::make_unique<Ex>(std::forward<Ts>(args)...);
                    auto ex = std::unique_ptr<viua::types::Exception>{};
                    ex.reset(e.release());
                    return ex;
                }
        }
    }
}

#endif
