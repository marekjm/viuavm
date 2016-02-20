#ifndef VIUA_VERSION_H
#define VIUA_VERSION_H

#pragma once

const char* VERSION = "0.7.0";
const char* MICRO = "46";

// commit pointed by the hash below is HEAD~1
// while the MICRO variable shows actual number of commits added since last release,
// the commit hash is off-by-one
const char* COMMIT = "d66e8a58b7847184471708ddc5709cc6db881c2f";

#endif
