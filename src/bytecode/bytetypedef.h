#ifndef WUDOO_BYTECODE_BYTETYPEDEF_H
#define WUDOO_BYTECODE_BYTETYPEDEF_H

#pragma once

/** This header contains only one typedef - for byte.
 *
 *  This is because this typedef is required by various files across Wudoo
 *  source code, and often they do not the opcodes.h header.
 *  Such header would prevent redefinition of `byte` in every file and
 *  will help to keep the definition consitent.
 *
 */

typedef char byte;

#endif
