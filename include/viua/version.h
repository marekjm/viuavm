#ifndef VIUA_VERSION_H
#define VIUA_VERSION_H

#pragma once

const char* VERSION = "0.5.0";
const char* MICRO = "3";

// commit pointed by the hash below is HEAD~1
// while the MICRO variable shows actual number of commits added since last release,
// the commit hash is off-by-one
const char* COMMIT = "8953dfaa17e700fcdbb2b28bf3bfecb4618648f6";

#endif
