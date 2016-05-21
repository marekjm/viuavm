#ifndef VIUA_VERSION_H
#define VIUA_VERSION_H

#pragma once

const char* VERSION = "0.8.0";
const char* MICRO = "73";

// commit pointed by the hash below is HEAD~1
// while the MICRO variable shows actual number of commits added since last release,
// the commit hash is off-by-one
const char* COMMIT = "a10727fc3ecc3ed5ca0a0cbf02fa778de31da455";

#endif
