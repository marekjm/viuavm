#ifndef VIUA_VERSION_H
#define VIUA_VERSION_H

#pragma once

const char* VERSION = "0.4.6";
const char* MICRO = "214";

// commit pointed by the hash below is HEAD~1
// while the MICRO variable shows actual number of commits added since last release,
// the commit hash is off-by-one
const char* COMMIT = "c06ab15f6b83a11e23af4da850cd7ec7f0b1765c";

#endif
