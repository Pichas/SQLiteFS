#pragma once

#include <sstream>
#include <string>
#include <vector>


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
