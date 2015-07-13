#ifndef VIUA_CPU_TRYFRAME_H
#define VIUA_CPU_TRYFRAME_H

#pragma once

#include <string>
#include <map>
#include <viua/bytecode/bytetypedef.h>
#include <viua/cpu/frame.h>
#include <viua/cpu/catcher.h>

class TryFrame {
    public:
        byte* return_address;
        Frame* associated_frame;

        std::string block_name;

        std::map<std::string, Catcher*> catchers;

        inline byte* ret_address() { return return_address; }

        TryFrame(): return_address(0), associated_frame(0) {}
        ~TryFrame() {
            for (auto p : catchers) {
                delete p.second;
            }
        }
};


#endif
