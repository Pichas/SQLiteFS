#pragma once

#include <sstream>
#include <string>
#include <vector>

#if __has_include(<tracy/Tracy.hpp>)
#include <tracy/Tracy.hpp>
#endif


#ifdef SQLITEFS_PROFILER_ENABLED
#define SQLITEFS_PROFILER(...) __VA_ARGS__
#define SQLITEFS_NO_PROFILER(...)

#ifndef SQLITE_SCOPED_PROFILER
#define SQLITE_SCOPED_PROFILER ZoneScoped
#endif

#ifndef SQLITE_SCOPEDN_PROFILER
#define SQLITE_SCOPEDN_PROFILER(...) ZoneScopedN(__VA_ARGS__)
#endif

#ifndef SQLITE_LOCABLE_PROFILER
#define SQLITE_LOCABLE_PROFILER(...) mutable TracyLockable(__VA_ARGS__)
#endif

#else
#define SQLITEFS_PROFILER(...)
#define SQLITEFS_NO_PROFILER(...) __VA_ARGS__

#define SQLITE_SCOPED_PROFILER
#define SQLITE_SCOPEDN_PROFILER(...)
#define SQLITE_LOCABLE_PROFILER(...)
#endif


template<typename Out>
inline void split(const std::string& s, char delim, Out result) {
    std::istringstream iss(s);
    std::string        item;
    while (std::getline(iss, item, delim)) {
        if (item.empty()) {
            continue;
        }
        *result++ = item;
    }
}

inline std::vector<std::string> split(const std::string& s, char delim) {
    std::vector<std::string> elems;
    split(s, delim, std::back_inserter(elems));
    return elems;
}
