#ifndef VIUA_VERSION_H
#define VIUA_VERSION_H

#pragma once

const char* VERSION = "0.8.0";
const char* MICRO = "167";

// commit pointed by the hash below is HEAD~1
// while the MICRO variable shows actual number of commits added since last release,
// the commit hash is off-by-one
const char* COMMIT = "dd5dfbbf0a73efb5bcab105c4d65a4c65d6a9b49";

#endif
