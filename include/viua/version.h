#ifndef VIUA_VERSION_H
#define VIUA_VERSION_H

#pragma once

const char* VERSION = "0.5.0";
const char* MICRO = "271";

// commit pointed by the hash below is HEAD~1
// while the MICRO variable shows actual number of commits added since last release,
// the commit hash is off-by-one
const char* COMMIT = "1d6c4f72e28d333b596c66c377cfe6217f40d711";

#endif
