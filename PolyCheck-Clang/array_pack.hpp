#pragma once

#include <vector>
#include <algorithm>
#include <map>

#include "util.hpp"

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
        assert(max_exprs_.size() == static_cast<size_t>(ndim));
        assert(min_exprs_.size() == static_cast<size_t>(ndim));
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

    std::string macro_undefs() const {
        std::string ret;
        ret += "#undef " + id_encode_macro_name() + "\n";
        for(int i = 0; i < ndim(); i++) {
            ret += "#undef " + dim_encode_macro_name(i) + "\n";
        }
        ret += "#undef " + ver_encode_macro_name() + "\n";
        ret += "#undef " + encode_macro_name() + "\n";
        ret += "#undef " + id_decode_macro_name() + "\n";
        for(int i = 0; i < ndim(); i++) {
            ret += "#undef " + dim_decode_macro_name(i) + "\n";
        }
        ret += "#undef " + ver_decode_macro_name() + "\n";
        ret += "#undef " + decode_macro_name() + "\n";
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

    //protected:
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
        assert(static_cast<int>(max_exprs_.size()) == ndim());
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
        assert(static_cast<int>(min_exprs_.size()) == ndim());
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
}; // class ArrayInfo

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

    std::string dim_decode_macro_use(const std::string& array_name,
            int dim, const std::string& args) const {
        assert(info_.find(array_name) != info_.end());
        const auto& info = info_.find(array_name)->second;
        assert(dim >= 0);
        assert(dim < info.ndim());
        return info.dim_decode_macro_use(dim, args);
    }

    std::string id_decode_macro_use(const std::string& array_name,
                                    const std::string& args) const {
        assert(info_.find(array_name) != info_.end());
        const auto& info = info_.find(array_name)->second;
        return info.id_decode_macro_use(args);
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

    std::string macro_undefs() const {
        std::string ret;
        for(const auto& i : info_) {
            ret += i.second.macro_undefs();
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
}; // class ArrayPack


