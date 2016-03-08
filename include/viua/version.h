#ifndef VIUA_VERSION_H
#define VIUA_VERSION_H

#pragma once

const char* VERSION = "0.7.0";
const char* MICRO = "125";

// commit pointed by the hash below is HEAD~1
// while the MICRO variable shows actual number of commits added since last release,
// the commit hash is off-by-one
const char* COMMIT = "233ea93780403857f083c4352b6dfa7b83707bb4";

#endif
