/*
 *  Copyright (C) 2015, 2016 Marek Marecki
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

#ifndef VIUA_TYPES_REFERENCE_H
#define VIUA_TYPES_REFERENCE_H

#pragma once

#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <viua/types/type.h>

namespace viua {
    namespace types {
        class Reference: public Type {
            // FIXME maybe just use std::shared_ptr<> ?
            Type **pointer;
            uint64_t *counter;

            /*  This constructor is used internally by the Reference type to
             *  initialise copies of the reference.
             */
            Reference(Type **ptr, uint64_t *ctr): pointer(ptr), counter(ctr) {}

            public:
                virtual std::string type() const override;
                virtual std::string str() const override;
                virtual std::string repr() const override;
                virtual bool boolean() const override;

                virtual std::vector<std::string> bases() const override;
                virtual std::vector<std::string> inheritancechain() const override;

                virtual std::unique_ptr<Type> copy() const override;
                virtual Type* pointsTo() const;
                virtual void rebind(Type*);
                virtual void rebind(std::unique_ptr<Type>);

                Reference(Type *ptr);
                virtual ~Reference();
        };
    }
}


#endif
