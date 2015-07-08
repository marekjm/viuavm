#ifndef VIUA_VERSION_H
#define VIUA_VERSION_H

#pragma once

const char* VERSION = "0.4.6";
const char* MICRO = "169";

// commit pointed by the hash below is HEAD~1
// while the MICRO variable shows actual number of commits added since last release,
// the commit hash is off-by-one
const char* COMMIT = "83f5f40ae4910cdd76d31a13875ae0e96f8b1d7e";

#endif
