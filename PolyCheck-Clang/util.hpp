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
    // std::cerr << "SPACE ndim=" << isl_space_dim(space, isl_dim_set) << "\n";
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

template<typename T>
std::ostream& operator<<(std::ostream& os, std::vector<T>& vec) {
    os << "[";
    for(auto& x : vec) os << x << ",";
    os << "]\n";
    return os;
}

/**
 * Pass in @param v the number of values to represented
 *
 * @return Number of bits required to represent 0..(v-1)
 */
static unsigned num_bits(uint64_t v) {
    // http://graphics.stanford.edu/~seander/bithacks.html#IntegerLogObvious
    uint64_t r; // result of log2(v) will go here
    uint64_t shift;
    uint64_t v1 = v;
    r = (v > 0xFFFFFFFF) << 5;
    v >>= r;
    r |= (v > 0xFFFF) << 4;
    v >>= r;
    shift = (v > 0xFF) << 3;
    v >>= shift;
    r |= shift;
    shift = (v > 0xF) << 2;
    v >>= shift;
    r |= shift;
    shift = (v > 0x3) << 1;
    v >>= shift;
    r |= shift;
    r |= (v >> 1);
    //the result is 1 + floor(log2(v))
    return r + ((v1^(1<<r))>0);
}

#endif // PolyCheck_util_hpp_
