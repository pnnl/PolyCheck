#ifndef PolyCheck_util_hpp_
#define PolyCheck_util_hpp_

#include <iostream>
#include <string>
#include <vector>

#include "islw.hpp"
#include <isl/set.h>
#include <isl/space.h>

inline std::string replace_all(const std::string& str_in,
                               const std::string& from, const std::string& to) {
    std::string str{str_in};
    if(!from.empty()) {
        size_t start_pos = 0;
        while((start_pos = str.find(from, start_pos)) != std::string::npos) {
            str.replace(start_pos, from.length(), to);
            start_pos += to.length();
        }
    }
    return str;
}

inline std::vector<std::string> iter_names(isl_set* iset) {
    std::vector<std::string> ret;
    for(size_t j = 0; j < isl_set_n_dim(iset); j++) {
        const char* iname = isl_set_get_dim_name(iset, isl_dim_set, j);
        if(iname != nullptr)
            ret.push_back(std::string(iname));
        else
            ret.push_back(std::string("c" + std::to_string(j)));
    }
    return ret;
}

inline std::vector<std::string> iter_names(isl_space* space) {
    std::vector<std::string> ret;
    std::cerr << "SPACE ndim=" << isl_space_dim(space, isl_dim_set) << "\n";
    for(size_t j = 0; j < isl_space_dim(space, isl_dim_set); j++) {
        const char* iname = isl_space_get_dim_name(space, isl_dim_set, j);
        if(iname != nullptr)
            ret.push_back(std::string(iname));
        else
            ret.push_back(std::string("c" + std::to_string(j)));
    }
    return ret;
}

template<typename T>
std::string join(const std::vector<T>& vec, const std::string& sep) {
    using islw::to_string;
    using std::to_string;
    std::string ret;
    if(!vec.empty()) { ret += to_string(vec.front()); }
    for(size_t i = 1; i < vec.size(); i++) { ret += sep + to_string(vec[i]); }
    return ret;
}

template<>
std::string join(const std::vector<std::string>& vec, const std::string& sep) {
    using islw::to_string;
    using std::to_string;
    std::string ret;
    if(!vec.empty()) { ret += vec.front(); }
    for(size_t i = 1; i < vec.size(); i++) { ret += sep + vec[i]; }
    return ret;
}

#endif // PolyCheck_util_hpp_
