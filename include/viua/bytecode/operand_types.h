#ifndef VIUA_OPERAND_TYPES_H
#define VIUA_OPERAND_TYPES_H

#pragma once

enum OperandType: unsigned char {
    OT_REGISTER,
    OT_REGISTER_REFERENCE,
    OT_ATOM,
};

#endif
