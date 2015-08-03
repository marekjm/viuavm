#ifndef VIUA_VERSION_H
#define VIUA_VERSION_H

#pragma once

const char* VERSION = "0.5.0";
const char* MICRO = "238";

// commit pointed by the hash below is HEAD~1
// while the MICRO variable shows actual number of commits added since last release,
// the commit hash is off-by-one
const char* COMMIT = "c0feb7a517d1ca875b8d06c35cb57bc8c99e502d";

#endif
