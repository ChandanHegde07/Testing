/**
 * @file version.h
 * @brief Version information for PCC (Prompt Context Controller)
 * 
 * This file contains version macros and constants that can be used
 * throughout the project for version checking and display.
 */

#ifndef VERSION_H
#define VERSION_H

/* Major version number */
#define VERSION_MAJOR 1

/* Minor version number */
#define VERSION_MINOR 0

/* Patch version number */
#define VERSION_PATCH 0

/* Version string macro */
#define VERSION_STRINGIFY(x) #x
#define VERSION_TOSTRING(x) VERSION_STRINGIFY(x)

/* Full version as a string */
#define VERSION_STRING VERSION_TOSTRING(VERSION_MAJOR) "." \
                       VERSION_TOSTRING(VERSION_MINOR) "." \
                       VERSION_TOSTRING(VERSION_PATCH)

/* Version packed into a single integer for comparisons */
/* Format: 0xMMNNPP (Major << 16 | Minor << 8 | Patch) */
#define VERSION_PACKED ((VERSION_MAJOR << 16) | (VERSION_MINOR << 8) | VERSION_PATCH)

/* Library name */
#define LIBRARY_NAME "PCC"
#define LIBRARY_DESCRIPTION "Prompt Context Controller"

/**
 * Check if version is at least major.minor.patch
 * @param major Major version required
 * @param minor Minor version required
 * @param patch Patch version required
 * @return 1 if current version >= required version, 0 otherwise
 */
#define VERSION_AT_LEAST(major, minor, patch) \
    (VERSION_PACKED >= (((major) << 16) | ((minor) << 8) | (patch)))

/**
 * Compile-time assertion for minimum required version
 * Use like: VERSION_REQUIRE(1, 0, 0)
 */
#define VERSION_REQUIRE(major, minor, patch) \
    typedef char static_assert_version_##major##_##minor##_##patch[VERSION_AT_LEAST(major, minor, patch) ? 1 : -1]

#endif /* VERSION_H */
