#ifndef VIUA_VERSION_H
#define VIUA_VERSION_H

#pragma once

const char* VERSION = "0.7.0";
const char* MICRO = "225";

// commit pointed by the hash below is HEAD~1
// while the MICRO variable shows actual number of commits added since last release,
// the commit hash is off-by-one
const char* COMMIT = "6549c2e8889aebdb47a529cac07edad9ad9131a3";

#endif
