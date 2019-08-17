#include <string>
#include <iostream>
#include <regex>
#undef NDEBUG
#include <cassert>
#include <cstdlib>
#include <set>
#include <map>

#include <isl/ctx.h>
#include <isl/union_map.h>
#include <isl/union_set.h>
#include <isl/set.h>
#include <isl/map.h>
#include <isl/printer.h>
#include <isl/point.h>
#include <isl/id.h>
#include <isl/aff.h>
#include <isl/polynomial.h>

#include <pet.h>
#include <barvinok/isl.h>

#include "PolyCheck.hpp"

//#define assert(x) do { if(!(x)) exit(0); } while(0)

std::string array_ref_string(isl_set* iset) {
    std::string aref{isl_set_get_tuple_name(iset)};
    return aref + "(" + join(iter_names(iset), ", ") + ")";
}

std::string array_ref_string_in_c(isl_set* iset) {
    std::string aref{isl_set_get_tuple_name(iset)};
    std::string idx = join(iter_names(iset), "][");
    if (!idx.empty()) {
        return aref + "[" + idx + "]";
    } else {
        return aref;
    }
}


std::string output_file_name(std::string file_name) {
    size_t pos = file_name.find_last_of("/");
    if(pos == std::string::npos) {
        return file_name.insert(0, "pc_");
    } else {
        return file_name.insert(pos+1, "pc_");
    }
}

std::map<std::string, int>
extract_array_name_info(isl_union_map* R, isl_union_map* W) {
    auto RW_R =
      isl_union_map_range(isl_union_map_union(islw::copy(W), islw::copy(R)));
    std::map<std::string, int> array_names_to_int;
    int ctr = 0;
    islw::foreach(RW_R, [&](__isl_keep isl_set* iset) {
        assert(isl_bool_true == isl_set_has_tuple_name(iset));
        const std::string name{isl_set_get_tuple_name(iset)};
        if(array_names_to_int.find(name) == array_names_to_int.end()) {
            array_names_to_int[name] = ctr++;
        }
    });
    std::cout << "Array names:\n";
    for(const auto& it : array_names_to_int) {
        std::cout << "  |" << it.first << "=" << it.second << "|\n";
    }
    islw::destruct(RW_R);
    return array_names_to_int;
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

class ArrayInfo {
    public:
    ArrayInfo(const std::string& name, int id, int ndim, int num_id_bits,
              const std::vector<std::vector<std::string>>& max_exprs,
              const std::vector<std::vector<std::string>>& min_exprs) :
      name_{name},
      id_{id},
      ndim_{ndim},
      num_id_bits_{num_id_bits},
      max_exprs_{max_exprs},
      min_exprs_{min_exprs} {
        assert(ndim >= 0);
        assert(max_exprs_.size() == ndim);
        assert(min_exprs_.size() == ndim);
    }

    std::string name() const { return name_; }

    int ndim() const { return ndim_; }

    int id() const { return id_; }

    int num_id_bits() const { return num_id_bits_; }

    std::string encode_string(const std::vector<std::string>& args) const {
        std::string ret;
        ret = encode_macro_name() + "(" + join(args, ",") + ")";
        return ret;
    }

    std::string decode_string(const std::vector<std::string>& args) const {
        std::string ret;
        ret = decode_macro_name() + "(" + join(args, ",") + ")";
        return ret;
    }

    std::string ver_decode_macro_use(const std::string& arg) const {
        return ver_decode_macro_name() + "("+arg+")";
    }

    std::string extern_preamble() const {
        std::string ret;
        ret += "extern unsigned long long " + ver_bitcount_variable() + ", " +
               ver_bitmask_variable() + ", " + ver_bitoffset_variable() + ";\n";
        ret += "extern unsigned long long "+maxver_variable()+";\n";
        for(int i = 0; i < ndim(); i++) {
            ret += "extern unsigned long long " + dim_bitcount_variable(i) +
                   ", " + dim_bitoffset_variable(i) + ", " +
                   dim_bitmask_variable(i) + ", " + dim_minindex_variable(i) +
                   ", " + dim_maxindex_variable(i) + ";\n";
        }
        return ret;
    }

    std::string macro_defns() const {
        std::string ret;
        ret += id_encode_macro_defn();
        for(int i = 0; i < ndim(); i++) {
            ret += dim_encode_macro_defn(i);
        }
        ret += ver_encode_macro_defn();
        ret += encode_macro_defn();
        ret += id_decode_macro_defn();
        for(int i = 0; i < ndim(); i++) {
            ret += dim_decode_macro_defn(i);
        }
        ret += ver_decode_macro_defn();
        ret += decode_macro_defn();
        return ret;
    }

    std::string global_decls() const {
        std::string ret;
        // ret += "unsigned long long " + id_bitcount_variable() + ", " +
        //        id_bitmask_variable() + ", " + id_bitoffset_variable() + ";\n";
        ret += "unsigned long long " + ver_bitcount_variable() + ", " +
               ver_bitmask_variable() + ", " + ver_bitoffset_variable() + ";\n";
        ret += "unsigned long long "+maxver_variable()+";\n";
        for(int i = 0; i < ndim(); i++) {
            ret += "unsigned long long " + dim_bitcount_variable(i) +
                   ", " + dim_bitoffset_variable(i) + ", " +
                   dim_bitmask_variable(i) + ", " + dim_minindex_variable(i) +
                   ", " + dim_maxindex_variable(i) + ";\n";
        }
        return ret;
    }

    std::string definition_preamble() const {
        std::string ret;
        for(int i=0; i<ndim(); i++) {
            ret += dim_bitcount_stmt(i) + "\n";
        }
        for(int i=0; i<ndim(); i++) {
            ret += dim_bitoffset_stmt(i) + "\n";
        }
        for(int i=0; i<ndim(); i++) {
            ret += dim_bitmask_stmt(i) + "\n";
        }
        for(int i=0; i<ndim(); i++) {
            ret += dim_minindex_stmt(i) + "\n";
        }
        for(int i=0; i<ndim(); i++) {
            ret += dim_maxindex_stmt(i) + "\n";
        }
        ret += maxver_stmt() + "\n";
        ret += ver_bitcount_stmt() + "\n";
        ret += ver_bitoffset_stmt() + "\n";
        ret += ver_bitmask_stmt() + "\n";
        ret += check_total_size_stmt()+"\n";
        return ret;
    }

    protected:
    std::string check_total_size_stmt() const {
        if(ndim() > 0) {
            return "assert(" + dim_bitoffset_variable(ndim() - 1) + " + " +
                   dim_bitcount_variable(ndim() - 1) + "<= 64 );";
        } else {
            return "assert(" + id_bitcount_variable() + " <= 64);";
        }
    }

    std::string maxver_variable() const {
        return "__"+name()+"_maxver";
    }

    std::string id_bitcount_variable() const {
        return std::to_string(num_id_bits_);
    }

    std::string id_bitmask_variable() const {
        return "((1<<"+std::to_string(num_id_bits_)+")-1)";
    }

    std::string id_bitoffset_variable() const {
        return "0";
    }

    std::string dim_bitmask_variable(int dim) const {
        assert(dim >= 0);
        assert(dim < ndim());
        return "__" + name() + "_bmask_d" + std::to_string(dim);
    }

    std::string dim_bitoffset_variable(int dim) const {
        assert(dim >= 0);
        assert(dim < ndim());
        return "__" + name() + "_boffset_d" + std::to_string(dim);
    }

    std::string dim_bitcount_variable(int dim) const {
        assert(dim >= 0);
        assert(dim < ndim());
        return "__" + name() + "_bcount_d" + std::to_string(dim);
    }

    std::string dim_minindex_variable(int dim) const {
        assert(dim >= 0);
        assert(dim < ndim());
        return "__" + name() + "_minindex_d" + std::to_string(dim);
    }

    std::string dim_maxindex_variable(int dim) const {
        assert(dim >= 0);
        assert(dim < ndim());
        return "__" + name() + "_maxindex_d" + std::to_string(dim);
    }

    std::string ver_bitmask_variable() const {
        return "__" + name() + "_bmask_v";
    }

    std::string ver_bitoffset_variable() const {
        return "__" + name() + "_boffset_v";
    }

    std::string ver_bitcount_variable() const {
        return "__" + name() + "_bcount_v";
    }

    std::string dim_encode_macro_name(int dim) const {
        assert(dim >= 0);
        assert(dim < ndim());
        std::string upper_name{name()};
        std::transform(upper_name.begin(), upper_name.end(), upper_name.begin(),
                       ::toupper);
        return "__" + upper_name + "_ENCODE_D" + std::to_string(dim);
    }

    std::string id_encode_macro_name() const {
        std::string upper_name{name()};
        std::transform(upper_name.begin(), upper_name.end(), upper_name.begin(),
                       ::toupper);
        return "__" + upper_name + "_ENCODE_ID";
    }

    std::string ver_encode_macro_name() const {
        std::string upper_name{name()};
        std::transform(upper_name.begin(), upper_name.end(), upper_name.begin(),
                       ::toupper);
        return "__" + upper_name + "_ENCODE_VER";
    }

    std::string dim_decode_macro_name(int dim) const {
        assert(dim >= 0);
        assert(dim < ndim());
        std::string upper_name{name()};
        std::transform(upper_name.begin(), upper_name.end(), upper_name.begin(),
                       ::toupper);
        return "__" + upper_name + "_DECODE_D" + std::to_string(dim);
    }

    std::string id_decode_macro_name() const {
        std::string upper_name{name()};
        std::transform(upper_name.begin(), upper_name.end(), upper_name.begin(),
                       ::toupper);
        return "__" + upper_name + "_DECODE_ID";
    }

    std::string ver_decode_macro_name() const {
        std::string upper_name{name()};
        std::transform(upper_name.begin(), upper_name.end(), upper_name.begin(),
                       ::toupper);
        return "__" + upper_name + "_DECODE_VER";
    }

    std::string encode_macro_name() const {
        std::string upper_name{name()};
        std::transform(upper_name.begin(), upper_name.end(), upper_name.begin(),
                       ::toupper);
        return "__"+upper_name+"_ENCODE";
    }

    std::string decode_macro_name() const {
        std::string upper_name{name()};
        std::transform(upper_name.begin(), upper_name.end(), upper_name.begin(),
                       ::toupper);
        return "__"+upper_name+"_DECODE";
    }

    std::string dim_encode_macro_defn(int dim) const {
        assert(dim >= 0);
        assert(dim < ndim());
        std::string ret;
        ret = "#define " + dim_encode_macro_name(dim) + "(_x) ((((uint64_t)(_x))-" +
              dim_minindex_variable(dim) + ")<<" + dim_bitoffset_variable(dim) +
              ")\n";
        return ret;
    }

    std::string dim_encode_macro_use(int dim, const std::string& arg) const {
        return dim_encode_macro_name(dim) + "(" + arg + ")";
    }

    std::string id_encode_macro_defn() const {
        std::string ret;
        ret = "#define " + id_encode_macro_name() + "() ((uint64_t)" +
              std::to_string(id_) + ")\n";
        return ret;
    }

    std::string ver_encode_macro_defn() const {
        std::string ret;
        ret = "#define " + ver_encode_macro_name() + "(_x) (((uint64_t)(_x))<<" +
              ver_bitoffset_variable() + ")\n";
        return ret;
    }

    std::string id_encode_macro_use() const {
        return id_encode_macro_name() + "()";
    }

    std::string dim_decode_macro_defn(int dim) const {
        assert(dim >= 0);
        assert(dim < ndim());
        std::string ret;
        ret = "#define " + dim_decode_macro_name(dim) + "(_x) " + "(((((uint64_t)(_x))&" +
              dim_bitmask_variable(dim) + ")>>" + dim_bitoffset_variable(dim) +
              ")+" + dim_minindex_variable(dim) + ")\n";
        return ret;
    }

    std::string dim_decode_macro_use(int dim, const std::string& arg) const {
        return dim_decode_macro_name(dim) + "(" + arg + ")";
    }

    std::string id_decode_macro_defn() const {
        std::string ret;
        ret = "#define " + id_decode_macro_name() + "(_x) (((uint64_t)(_x))&" +
              id_mask_expr() + ")\n";
        return ret;
    }

    std::string ver_decode_macro_defn() const {
        std::string ret;
        ret = "#define " + ver_decode_macro_name() + "(_x) " + "((((uint64_t)(_x))&" +
              ver_bitmask_variable() + ")>>" + ver_bitoffset_variable() + ")\n";
        return ret;
    }

    std::string id_decode_macro_use(const std::string& arg) const {
        return id_decode_macro_name() + "("+arg+")";
    }

    std::string encode_macro_defn() const {
        std::string ret;
        ret = "#define " + encode_macro_name() + "(";
        for(int i = 0; i < ndim(); i++) {
            ret += "_x" + std::to_string(i) + ", ";
        }
        ret += "_xv";
        ret += ") (" + id_encode_macro_use();
        for(int i = 0; i < ndim(); i++) {
            ret +=
              " | " + dim_encode_macro_use(i, "(_x" + std::to_string(i) + ")");
        }
        ret += " | " + ver_encode_macro_name() + "((_xv))";
        ret += ")\n";
        return ret;
    }

    std::string decode_macro_defn() const {
        std::string ret;
        ret = "#define " + decode_macro_name() + "(_x) " +
              id_decode_macro_use("_x");
        for(int i = 0; i < ndim(); i++) {
            ret += ", " + dim_decode_macro_use(i, "_x");
        }
        ret += ", "+ ver_decode_macro_name()+"(_x)";
        ret += "\n";
        return ret;
    }

    std::string dim_bitcount_stmt(int dim) const {
        assert(dim >= 0);
        assert(dim < ndim());
        return dim_bitcount_variable(dim) + " = num_bits((" +
               dim_max_string(dim) + ") - (" + dim_min_string(dim) + ") + 1);";
    }

    std::string dim_bitoffset_stmt(int dim) const {
        assert(dim >= 0);
        assert(dim < ndim());
        std::string ret =
          dim_bitoffset_variable(dim) + " = " + id_bitcount_variable();
        for(int i = 0; i < dim; i++) { ret += " + " + dim_bitcount_variable(i); }
        ret += ";";
        return ret;
    }

    std::string dim_bitmask_stmt(int dim) const {
        assert(dim >= 0);
        assert(dim < ndim());
        std::string ret = dim_bitmask_variable(dim) + " = ";
        ret += "( (1ul<<" + dim_bitcount_variable(dim) + ")-1)<<" +
               dim_bitoffset_variable(dim) + ";";
        return ret;
    }

    std::string maxver_stmt() const {
        std::string ret;
        std::string off = id_bitcount_variable();
        for(int i = 0; i < ndim(); i++) {
            off += " + " + dim_bitcount_variable(i);
        }
        ret = "assert(("+off+") <63);\n";
        ret += maxver_variable() + "= 1ul<<(63 - ("+off+"));";
        return ret;
    }

    std::string minver_variable() const { return "0";}

    std::string ver_bitcount_stmt() const {
        return ver_bitcount_variable() + " = num_bits((" + maxver_variable() +
               ") - (" + minver_variable() + ") + 1);";
    }

    std::string ver_bitoffset_stmt() const {
        std::string ret =
          ver_bitoffset_variable() + " = " + id_bitcount_variable();
        for(int i = 0; i < ndim(); i++) {
            ret += " + " + dim_bitcount_variable(i);
        }
        ret += ";";
        return ret;
    }

    std::string ver_bitmask_stmt() const {
        std::string ret = ver_bitmask_variable() + " = ";
        ret += "( (1ul<<" + ver_bitcount_variable() + ")-1)<<" +
               ver_bitoffset_variable() + ";";
        return ret;
    }

    std::string dim_minindex_stmt(int dim) const {
        assert(dim >= 0);
        assert(dim < ndim());
        std::string ret = dim_minindex_variable(dim) + " = ";
        ret += dim_min_string(dim) + ";";
        return ret;
    }

    std::string dim_maxindex_stmt(int dim) const {
        assert(dim >= 0);
        assert(dim < ndim());
        std::string ret = dim_maxindex_variable(dim) + " = ";
        ret += dim_max_string(dim) + ";";
        return ret;
    }

    std::string id_mask_expr() const {
        return "((1ul<<" + id_bitcount_variable() + ")-1)";
    }

    std::string dim_max_string(int dim) const {
        assert(dim >= 0);
        assert(dim < ndim());
        assert(max_exprs_.size() == ndim());
        std::string ret;
        assert(max_exprs_[dim].size() > 0);
        if(max_exprs_[dim].size() == 1) {
            ret = "(" + max_exprs_[dim][0] + ")";
        } else {
            ret = "MAX(" + max_exprs_[dim][0] + ", " + max_exprs_[dim][1] + ")";
            for(size_t i = 2; i < max_exprs_[dim].size(); i++) {
                ret = "MAX(" + ret + ", " + max_exprs_[dim][i] + ")";
            }
        }
        return ret;
    }

    std::string dim_min_string(int dim) const {
        assert(dim >= 0);
        assert(dim < ndim());
        assert(min_exprs_.size() == ndim());
        std::string ret;
        assert(min_exprs_[dim].size() > 0);
        if(min_exprs_[dim].size() == 1) {
            ret = "(" + min_exprs_[dim][0] + ")";
        } else {
            ret = "MIN(" + min_exprs_[dim][0] + ", " + min_exprs_[dim][1] + ")";
            for(size_t i = 2; i < min_exprs_[dim].size(); i++) {
                ret = "MAX(" + ret + ", " + min_exprs_[dim][i] + ")";
            }
        }
        return ret;
    }

    private:
    //array name
    std::string name_;
    int id_; //small integer id for this array. id_>=0
    int ndim_; //number of array dimensions. 0 for scalar
    int num_id_bits_; //number of bits to store array id (id_ < 2^num_id_bits_)

    std::vector<std::vector<std::string>>
      max_exprs_; // Max expressions for each dimension
    std::vector<std::vector<std::string>>
      min_exprs_; // Max expressions for each dimension
};

class ArrayPack {
    public:
    ArrayPack(isl_union_map* R, isl_union_map* W) {
        struct Builder {
            int ndim;
            int id;
            std::vector<std::vector<std::string>> max_exprs, min_exprs;
        };
        std::map<std::string, Builder> builders;

        auto RW_R = isl_union_map_range(
          isl_union_map_union(islw::copy(W), islw::copy(R)));
        islw::foreach(RW_R, [&](__isl_keep isl_set* iset) {
            assert(isl_bool_true == isl_set_has_tuple_name(iset));
            const std::string name{isl_set_get_tuple_name(iset)};
            builders[name].ndim = isl_set_n_dim(iset);
            auto& builder = builders[name];
            if(builder.max_exprs.size() == 0) {
                builder.max_exprs.resize(builder.ndim);
            }
            if(builder.min_exprs.size() == 0) {
                builder.min_exprs.resize(builder.ndim);
            }
            //int ndim = isl_set_n_dim(iset);
            //std::vector<std::string> max_exprs, min_exprs;
            for(int i = 0; i < builder.ndim; i++) {
                builder.max_exprs[i].push_back(islw::to_c_string(
                  isl_set_dim_max(islw::copy(iset), i), true));
                builder.min_exprs[i].push_back(islw::to_c_string(
                  isl_set_dim_min(islw::copy(iset), i), true));
                // max_exprs.push_back(islw::to_c_string(
                //   isl_set_dim_max(islw::copy(iset), i), true));
                // min_exprs.push_back(islw::to_c_string(
                //   isl_set_dim_min(islw::copy(iset), i), true));
                // std::cout << "  dim=" << i << " max aff expr="
                //           << islw::to_c_string(
                //                isl_set_dim_max(islw::copy(iset), i), true)
                //           << "\n";
                // std::cout << "  dim=" << i << " min aff expr="
                //           << islw::to_c_string(
                //                isl_set_dim_min(islw::copy(iset), i), true)
                //           << "\n";
            }
        });
        islw::destruct(RW_R);
        //compute writer card
        //max(card(inv))
        //isl_union_map_card(isl_union_map_reverse(islw::copy(W)));

        int id = 0;
        for(auto& it : builders) {
            it.second.id = id++;
        }
        int num_id_bits = num_bits(id);
        for(const auto& it : builders) {
            const auto& sec = it.second;
            info_.insert(
              {it.first, ArrayInfo{it.first, sec.id, sec.ndim, num_id_bits,
                                   sec.max_exprs, sec.min_exprs}});
        }
    }

    std::string encode_string(const std::string& array_name,
                              const std::vector<std::string>& args) const {
        assert(info_.find(array_name) != info_.end());
        const auto& info = info_.find(array_name)->second;
        return info.encode_string(args);
    }

    std::string decode_string(const std::string& array_name,
                              const std::vector<std::string>& args) const {
        assert(info_.find(array_name) != info_.end());
        const auto& info = info_.find(array_name)->second;
        return info.decode_string(args);
    }

    std::string ver_decode_macro_use(const std::string& array_name,
                                     const std::string& args) const {
        assert(info_.find(array_name) != info_.end());
        const auto& info = info_.find(array_name)->second;
        return info.ver_decode_macro_use(args);
    }

    std::string global_decls() const {
        std::string ret;
        for(const auto& i : info_) {
            ret += i.second.global_decls();
        }
        return ret;
    }

    std::string definition_preamble() const {
        std::string ret;
        for(const auto& i : info_) {
            ret += i.second.definition_preamble();
        }
        return ret;
    }

    std::string extern_preamble() const {
        std::string ret;
        for(const auto& i : info_) {
            ret += i.second.extern_preamble();
        }
        return ret;
    }

    std::string macro_defns() const {
        std::string ret;
        for(const auto& i : info_) {
            ret += i.second.macro_defns();
        }
        return ret;
    }

    std::string entry_function_preamble() const {
        std::string ret = "unsigned num_bits(unsigned long long);\n";
        int num_id_bits = num_bits(info_.size());
        ret += "const int __bmask_id= (1<<" + std::to_string(num_id_bits) + ")-1;\n";
        ret += extern_preamble();
        ret += definition_preamble();
        ret += macro_defns();
        return ret;
    }

    std::string nonentry_function_preamble() const {
        std::string ret;
        ret += extern_preamble();
        ret += macro_defns();
        return ret;
    }

    private:
    std::map<std::string,ArrayInfo> info_;
};

///////////////////////////////////////////////////////////////////////////////

class Prolog {
    public:
    //explicit Prolog() {}
    ~Prolog()             = default;
    Prolog(const Prolog&) = default;
    Prolog(Prolog&&) = default;
    Prolog& operator=(const Prolog&) = default;
    Prolog& operator=(Prolog&&) = default;

    Prolog(isl_union_map* R, isl_union_map* W) : array_pack_{R, W} {
        isl_union_set* RW =
          isl_union_set_union(isl_union_map_range(islw::copy(R)),
                              isl_union_map_range(islw::copy(W)));
        // isl_union_set_foreach_set(RW, fn_prolog_per_set, &this->str_);
        isl_union_set_foreach_set(RW, fn_prolog_per_set, this);
        islw::destruct(RW);
    }

    std::string to_string() const {
        return "#include <assert.h>\n int _diff = 0;\n{\n" +
               array_pack_.entry_function_preamble() + str_ + "\n}\n";
    }

    private:
    // should be a lambda
    static isl_stat fn_prolog_per_set(isl_set* set, void* user) {
        assert(user != nullptr);
        assert(set != nullptr);
        Prolog& prolog_obj = *(Prolog*)user;
        // std::string& prolog = *(std::string*)user;
        std::string& prolog = prolog_obj.str_;
        const unsigned ndim = isl_set_dim(set, isl_dim_set);
        for(unsigned i = 0; i < ndim; i++) {
            set = isl_set_set_dim_name(set, isl_dim_set, i,
                                       ("_i" + std::to_string(i)).c_str());
        }
        std::string set_code      = islw::to_c_string(set);
        std::vector<std::string> args_vec;
        for(unsigned i=0; i<isl_set_dim(set, isl_dim_set); i++) {
            assert(isl_set_has_dim_name(set, isl_dim_set, i) == isl_bool_true);
            assert(isl_set_has_dim_id(set,isl_dim_set, i) == isl_bool_true);
            const char *str =isl_set_get_dim_name(set, isl_dim_set, i);
            assert(str);
            args_vec.push_back(str);
        }
        std::string array_name{isl_set_get_tuple_name(set)};
        std::string array_ref = array_name;;
        if(ndim>0) {
            array_ref += "[" + join(args_vec, "][") + "]";
        }
        std::string pre =
          "#define PC_PR(" + join(args_vec, ",") + ")\t" + array_ref + " = ";
        std::vector<std::string> args{args_vec};
        args.push_back("1");
        pre += prolog_obj.array_pack_.encode_string(array_name, args) + "\n";
        std::string post = "#undef PC_PR\n";
        prolog +=
          pre + replace_all(set_code, array_name + "(", "PC_PR(") + post;
        islw::destruct(set);
        return isl_stat_ok;
    }

    std::string str_;
    ArrayPack array_pack_;
}; // class Prolog

std::string prolog(isl_union_map* R, isl_union_map* W) {
    Prolog prolog{R, W};
    return prolog.to_string();
}

///////////////////////////////////////////////////////////////////////////////

class Epilog {
    public:
    //explicit Epilog() {}
    ~Epilog()             = default;
    Epilog(const Epilog&) = default;
    Epilog(Epilog&&) = default;
    Epilog& operator=(const Epilog&) = default;
    Epilog& operator=(Epilog&&) = default;

    Epilog(isl_union_map* R, isl_union_map* W) : array_pack_{R, W} {
        isl_union_pw_qpolynomial* WC =
          isl_union_map_card(isl_union_map_reverse(islw::copy(W)));
        isl_union_pw_qpolynomial_foreach_pw_qpolynomial(
          WC, Epilog::epilog_per_poly, &this->str_);
        isl_union_set* rset = isl_union_map_range(islw::copy(R));
        isl_union_set* wset = isl_union_map_range(islw::copy(W));
        isl_union_set* ronly_set =
          isl_union_set_subtract(islw::copy(rset), islw::copy(wset));
        // isl_union_set_foreach_set(ronly_set, Epilog::epilog_per_ronly_piece,
        //                           &this->str_);
        isl_union_set_foreach_set(ronly_set, Epilog::epilog_per_ronly_piece,
                                  this);
        islw::destruct(rset);
        islw::destruct(wset);
        islw::destruct(ronly_set);
        islw::destruct(WC);
    }

    std::string to_string() const {
        return "{\n" + array_pack_.nonentry_function_preamble() + str_ +
               "\n assert(_diff == 0); \n}\n";
    }

    private:
    // should be a lambda
    static isl_stat epilog_per_ronly_piece(isl_set* iset, void* user) {
        assert(iset);
        assert(user);
        Epilog& epilog = *(Epilog*)user;
        //std::string& ep = *(std::string*)user;
        std::string& ep = epilog.str_;
        const auto& array_pack = epilog.array_pack_;
        const unsigned ndim = isl_set_dim(iset, isl_dim_set);
        for(unsigned i = 0; i < ndim; i++) {
            iset = isl_set_set_dim_name(iset, isl_dim_set, i,
                                       ("_i" + std::to_string(i)).c_str());
        }
        std::string set_code = islw::to_c_string(iset);
        std::vector<std::string> args_vec;
        for(unsigned i=0; i<isl_set_dim(iset, isl_dim_set); i++) {
            assert(isl_set_has_dim_name(iset, isl_dim_set, i) == isl_bool_true);
            assert(isl_set_has_dim_id(iset,isl_dim_set, i) == isl_bool_true);
            const char *str =isl_set_get_dim_name(iset, isl_dim_set, i);
            assert(str);
            args_vec.push_back(str);
        }
        std::string array_name{isl_set_get_tuple_name(iset)};
        std::string array_ref = array_name;

        if(ndim>0) {
            array_ref += "[" + join(args_vec, "][") + "]";
        }
        std::string ep_macro_def =
          "#define PC_EP_CHECK(" + join(args_vec, ",") +
          ")\t _diff |= 1 ^ (int)" +
          array_pack.ver_decode_macro_use(array_name, array_ref) + "\n";
        std::string ep_macro_undef = "#undef PC_EP_CHECK\n";
        ep += ep_macro_def +
              replace_all(set_code, array_name + "(", "PC_EP_CHECK(") +
              ep_macro_undef;
        islw::destruct(iset);
        return isl_stat_ok;
    }

    // should be a lambda
    static isl_stat epilog_per_poly(isl_pw_qpolynomial* pwqp, void* user) {
        isl_pw_qpolynomial_foreach_piece(pwqp, Epilog::epilog_per_poly_piece, user);
        islw::destruct(pwqp);
        return isl_stat_ok;
    }

    // should be a lambda
    static isl_stat epilog_per_poly_piece(isl_set* set, isl_qpolynomial* poly,
                                          void* user) {
        assert(set);
        assert(poly);
        assert(isl_set_n_dim(set) ==
               isl_qpolynomial_dim(poly, isl_dim_in));
        assert(user != nullptr);
        Epilog& epilog = *(Epilog*)user;
        //std::string& ep = *(std::string*)user;
        std::string& ep = epilog.str_;
        const auto& array_pack = epilog.array_pack_;
        const unsigned ndim = isl_set_dim(set, isl_dim_set);
        for(unsigned i = 0; i < ndim; i++) {
            set = isl_set_set_dim_name(set, isl_dim_set, i,
                                       ("_i" + std::to_string(i)).c_str());
        }
        for(unsigned i = 0; i < ndim; i++) {
            poly = isl_qpolynomial_set_dim_name(
              poly, isl_dim_in, i, ("_i" + std::to_string(i)).c_str());
        }
        std::string set_code      = islw::to_c_string(set);
        std::string poly_code     = islw::to_c_string(poly);
        std::vector<std::string> args_vec;
        for(unsigned i=0; i<isl_set_dim(set, isl_dim_set); i++) {
            assert(isl_set_has_dim_name(set, isl_dim_set, i) == isl_bool_true);
            assert(isl_set_has_dim_id(set,isl_dim_set, i) == isl_bool_true);
            const char *str =isl_set_get_dim_name(set, isl_dim_set, i);
            assert(str);
            args_vec.push_back(str);
        }
        std::string array_name{isl_set_get_tuple_name(set)};
        std::string array_ref = array_name;
        if(ndim>0) {
            array_ref += "[" + join(args_vec, "][") + "]";
        }
        std::string ep_macro_def =
          "#define PC_EP_CHECK(" + join(args_vec, ",") + ")\t _diff |= (1+(" +
          poly_code + ")) ^ (int)" +
          array_pack.ver_decode_macro_use(array_name, array_ref) + "\n";
        std::string ep_macro_undef = "#undef PC_EP_CHECK\n";
        ep += ep_macro_def +
              replace_all(set_code, array_name + "(", "PC_EP_CHECK(") +
              ep_macro_undef;
        islw::destruct(set);
        islw::destruct(poly);
        return isl_stat_ok;
    }

    std::string str_;
    ArrayPack array_pack_;
};  // class Epilog


std::string epilog(isl_union_map* R, isl_union_map* W) {
    Epilog epilog{R, W};
    return epilog.to_string();
}


///////////////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[]) {
    assert(argc == 3);
    std::string filename{argv[1]};
    std::string target{argv[2]};

    struct pet_options* options = pet_options_new_with_defaults();
    isl_ctx* ctx = isl_ctx_alloc_with_options(&pet_options_args, options);
    struct pet_scop* scop =
      pet_scop_extract_from_C_source(ctx, filename.c_str(), NULL);

    isl_union_map *R, *W, *S;
    R = pet_scop_get_may_reads(scop);
    W = pet_scop_get_may_writes(scop);
    isl_schedule* isched = pet_scop_get_schedule(scop);
    S = isl_schedule_get_map(isched);

#if 0//for testing and prototyping
    ArrayPack array_pack{R, W};
    std::cout<<"//DECLS:\n"<<array_pack.global_decls()<<"\n";
    std::cout<<"//NONENTRY FUNCTION:\n"<<array_pack.nonentry_function_preamble()<<"\n";
    // extract_array_name_info(R, W);
    // extract_domain_ranges(R, W);
    // std::cout<<"Write= "<<islw::to_string(W)<<"\n";
    // // auto WD = isl_union_map_domain(islw::copy(W));
    // // std::cout<<"Write domain= "<<islw::to_string(WD)<<"\n";
    // auto RW_R =
    //   isl_union_map_range(isl_union_map_union(islw::copy(W), islw::copy(R)));
    // std::map<std::string,int> array_names_to_int;
    // int ctr = 0;
    // islw::foreach(RW_R, [&](__isl_keep isl_set* iset) {
    //     assert(isl_bool_true == isl_set_has_tuple_name(iset));
    //     std::cout << "xx... " << isl_set_get_tuple_name(iset) << "\n";
    //     const std::string name{isl_set_get_tuple_name(iset)}; 
    //     if(array_names_to_int.find(name) == array_names_to_int.end()) {
    //         array_names_to_int[name] = ctr++;
    //     }
    //     //array_names.insert(std::string(isl_set_get_tuple_name(iset)));
    //     // std::cout<<islw::to_string(iset)<<"\n";
    //     // std::cout<<islw::to_string(isl_set_gist(islw::copy(iset), islw::context(iset)))<<"\n";
    //     // std::cout<<islw::to_string(isl_set_get_tuple_id(iset))<<"\n";
    // });
    // std::cout<<"All array refs= "<<islw::to_string(RW_R)<<"\n";
    // // std::cout<<"All array refs= "<<islw::to_string(isl_union_set_get_space( islw::copy(RW_R)))<<"\n";
    // std::cout<<"Array names:\n";
    // for (const auto& it : array_names_to_int) {
    //     std::cout<<"  |"<<it.first<<"="<<it.second<<"|\n";
    // }
    // islw::destruct(RW_R);
#else
    //std::vector<std::string> inline_checks;
    std::vector<Statement> stmts;
    {
        for(int i = 0; i < scop->n_stmt; i++) {
            auto l = pet_loc_get_line(scop->stmts[i]->loc);
            std::cout << "---------#statement " << i << " in line: " << l << "------------\n";
            if(l!=-1) stmts.push_back(Statement{i, scop});
        }
        // for(const auto& stmt : stmts) {
        //     inline_checks.push_back(stmt.inline_checks());
        // }
    }

    const std::string prologue = prolog(R, W) + "\n";
    const std::string epilogue = epilog(R, W);
    std::cout << "Prolog\n------\n" << prologue << "\n----------\n";
    std::cout << "Epilog\n------\n" << epilogue << "\n----------\n";
    std::cout << "#statements=" << scop->n_stmt << "\n";
    ArrayPack array_pack{R, W};
    std::cout << "//GLOBAL DECLARATIONS:\n-------------------\n"
              << array_pack.global_decls() << "\n"
              <<"-----------------------\n";
    //std::cout << "Inline checks\n------\n";
    // for(size_t i=0; i<inline_checks.size(); i++) {
    //     std::cout<<"  Statement "<<i<<"\n  ---\n"<<inline_checks[i]<<"\n";
    // }
    std::cout<<"-------\n";

    ParseScop(target, stmts, prologue, epilogue, output_file_name(target));
    stmts.clear();
#endif
    isl_schedule_free(isched);
    islw::destruct(R, W, S);
    pet_scop_free(scop);
    islw::destruct(ctx);
    return 0;
}