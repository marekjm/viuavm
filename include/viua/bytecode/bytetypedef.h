#ifndef VIUA_BYTECODE_BYTETYPEDEF_H
#define VIUA_BYTECODE_BYTETYPEDEF_H

#pragma once

/** This header contains only one typedef - for byte.
 *
 *  This is because this typedef is required by various files across Wudoo
 *  source code, and often they do not need the opcodes.h header.
 *  This header prevents redefinition of `byte` in every file and
 *  helps keeping the definition consistent.
 *
 */

typedef char byte;

#endif
