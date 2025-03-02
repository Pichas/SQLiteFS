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

#ifndef SQLITEFS_SCOPED_PROFILER
#define SQLITEFS_SCOPED_PROFILER ZoneScoped
#endif

#ifndef SQLITEFS_SCOPEDN_PROFILER
#define SQLITEFS_SCOPEDN_PROFILER(...) ZoneScopedN(__VA_ARGS__)
#endif

#ifndef SQLITEFS_LOCABLE_PROFILER
#define SQLITEFS_LOCABLE_PROFILER(TYPE, NAME) TracyLockable(TYPE, NAME)
#endif

#else
#define SQLITEFS_PROFILER(...)
#define SQLITEFS_NO_PROFILER(...) __VA_ARGS__

#define SQLITEFS_SCOPED_PROFILER
#define SQLITEFS_SCOPEDN_PROFILER(...)
#define SQLITEFS_LOCABLE_PROFILER(TYPE, NAME) TYPE NAME
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
