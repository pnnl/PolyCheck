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

#include "array_pack.hpp"

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
        cardinality_checks(W);
        islw::destruct(RW);
    }

    std::string to_string() const {
        return "#include <assert.h>\n int _diff = 0;\n{\n" +
               array_pack_.entry_function_preamble() + str_ +
               cardinality_str_ +
               array_pack_.macro_undefs() + "\n}\n";
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
        for(int i=0; i<isl_set_dim(set, isl_dim_set); i++) {
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
          "#define PC_PR(" + join(args_vec, ",") + ")\t *(uint64_t*)&(" + array_ref + ") = ";
        std::vector<std::string> args{args_vec};
        args.push_back("1");
        pre += prolog_obj.array_pack_.encode_string(array_name, args) + "\n";
        std::string post = "#undef PC_PR\n";
        prolog +=
          pre + replace_all(set_code, array_name + "(", "PC_PR(") + post;
        islw::destruct(set);
        return isl_stat_ok;
    }

    void cardinality_checks(isl_union_map* W) {
      auto W2S = isl_union_map_reverse(islw::copy(W));
      auto wcard = isl_union_map_card(islw::copy(W2S));

      struct Args {
        std::map<std::string, std::string> qpmap;
        ArrayPack* array_pack;
      };
      Args args;
      args.array_pack = &array_pack_;
      auto process_pw_qpoly = [](__isl_take isl_pw_qpolynomial* pwqp,
                                 void* user) -> isl_stat {
        assert(pwqp);
        assert(user);
        Args& args = *(Args*)user;
        std::map<std::string, std::string>& qpmap = args.qpmap;
        ArrayPack& array_pack = *args.array_pack;
        auto sp = isl_pw_qpolynomial_get_space(pwqp);
        std::string qp_aname{isl_space_get_tuple_name(sp, isl_dim_in)};
        islw::destruct(sp);
        //@bug @todo If found, this qp should be merged with that one
        assert(qpmap.find(qp_aname) == qpmap.end());

        // std::cout << "pwqp as string :---\n"
        //           << islw::to_string(pwqp) << "\n---\n";
        int dim = isl_pw_qpolynomial_dim(pwqp, isl_dim_in);
        assert(dim == array_pack.ndim(qp_aname));
        for (int i = 0; i < dim; i++) {
          std::string str{"c" + std::to_string(i)};
          pwqp =
              isl_pw_qpolynomial_set_dim_name(pwqp, isl_dim_in, i, str.c_str());
        }
        qpmap[qp_aname] = islw::to_c_string(pwqp);
        // std::cout << "pwqp array name: " << qp_aname << "---\n";
        // std::cout << "pwqp as c string :---\n"
        //           << islw::to_c_string(pwqp) << "\n---\n";
        islw::destruct(pwqp);
        return isl_stat_ok;
      };

      isl_union_pw_qpolynomial_foreach_pw_qpolynomial(wcard, process_pw_qpoly,
                                                      (void*)&args);
      auto& qpmap = args.qpmap;
      std::vector<std::string> array_names = array_pack_.array_names();
      for (const auto& aname : array_names) {
        if (qpmap.find(aname) == qpmap.end()) {
          qpmap[aname] = " 0";
        }
        for (int c = 0; c <= 9; c++) {
          qpmap[aname] = replace_all(qpmap[aname], " " + std::to_string(c),
                                     " (uint64_t)" + std::to_string(c));
        }
      }

      //   std::cout << "-----------qmpap----------\n";
      //   for (const auto& qp : qpmap) {
      //     std::cout << qp.first << ": &&" << qp.second << "&&\n";
      //   }
      //   std::cout << "----------------------\n";

      cardinality_str_ = "{\n/* max version check */\n";
      cardinality_str_ += "#include <assert.h>\n";
      std::vector<std::string> macro_defs, macro_undefs;
      auto macro_name = [](const std::string& aname) -> std::string {
        return "PC_CARD_" + aname;
      };
      auto args_fn = [](int dim) -> std::string {
        std::vector<std::string> retv;
        for (int i = 0; i < dim; i++) {
          retv.push_back("c" + std::to_string(i));
        }
        return "(" + join(retv, ", ") + ")";
      };
      for (const auto& aname : array_names) {
        assert(qpmap.find(aname) != qpmap.end());
        const std::string iargs = args_fn(array_pack_.ndim(aname));
        macro_defs.push_back("#define " + macro_name(aname) + iargs +
                             " ("+qpmap[aname]+")");
        macro_undefs.push_back("#undef " + macro_name(aname));
      }
      cardinality_str_ += join(macro_defs, "\n") + "\n";
      for (const auto& aname : array_names) {
          cardinality_str_ += "uint64_t _d_"+aname+" = 0;\n";
      }
      std::string code = islw::to_c_string(W2S);
      //to avoid duplicated replacements
      for (const auto& aname : array_names) {
        code = replace_all(code, aname+"(", "[["+aname+"]](");
      }
      for (const auto& aname : array_names) {
        code = replace_all(code, "[["+aname+"]](",
                           "_d_" + aname +
                               " |= " + array_pack_.maxver_variable(aname) +
                               " <= " + macro_name(aname)+"(");
      }
      cardinality_str_ += code;
      for (const auto& aname : array_names) {
        cardinality_str_ += "assert(_d_" + aname + " == 0);\n";
      }
      cardinality_str_ += join(macro_undefs, "\n") + "\n";
      cardinality_str_ += "\n} /* max version check */\n";
      //   std::cout << "CARD CHECK CODE=\n" << result << "\n";
      islw::destruct(wcard, W2S);
      }

    std::string str_;
    std::string cardinality_str_;
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
      return "{\n" + check_macro_def_string() +
             array_pack_.nonentry_function_preamble() + str_ +
             "\n assert(_diff == 0);\n" + array_pack_.macro_undefs() +
             check_macro_undef_string() + "\n}\n";
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
        for(int i=0; i<isl_set_dim(iset, isl_dim_set); i++) {
            assert(isl_set_has_dim_name(iset, isl_dim_set, i) == isl_bool_true);
            assert(isl_set_has_dim_id(iset,isl_dim_set, i) == isl_bool_true);
            const char *str =isl_set_get_dim_name(iset, isl_dim_set, i);
            assert(str);
            args_vec.push_back(str);
        }
        std::string array_name{isl_set_get_tuple_name(iset)};
        std::string array_ref = "*(uint64_t*)&" + array_name;

        if(ndim>0) {
            array_ref += "[" + join(args_vec, "][") + "]";
        }
        // std::string ep_macro_def =
        //   "#define PC_EP_CHECK(" + join(args_vec, ",") +
        //   ")\t _diff |= 1 ^ (int)" +
        //   array_pack.ver_decode_macro_use(array_name, array_ref) + "\n";
        std::string ep_macro_def =
            "#define PC_EP_CHECK(" + join(args_vec, ",") +
            ")\t "+check_macro_use("_diff", "1ul", "(uint64_t)" +
            array_pack.ver_decode_macro_use(array_name, array_ref));
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
        for(int i=0; i<isl_set_dim(set, isl_dim_set); i++) {
            assert(isl_set_has_dim_name(set, isl_dim_set, i) == isl_bool_true);
            assert(isl_set_has_dim_id(set,isl_dim_set, i) == isl_bool_true);
            const char *str =isl_set_get_dim_name(set, isl_dim_set, i);
            assert(str);
            args_vec.push_back(str);
        }
        std::string array_name{isl_set_get_tuple_name(set)};
        std::string array_ref = "*(uint64_t*)&" + array_name;
        if(ndim>0) {
            array_ref += "[" + join(args_vec, "][") + "]";
        }
        // std::string ep_macro_def =
        //   "#define PC_EP_CHECK(" + join(args_vec, ",") + ")\t _diff |= (1+(" +
        //   poly_code + ")) ^ (int)" +
        //   array_pack.ver_decode_macro_use(array_name, array_ref) + "\n";
        std::string ep_macro_def =
            "#define PC_EP_CHECK(" + join(args_vec, ",") + ")\t " +
            check_macro_use("_diff", "(1ul+(" + poly_code + "))",
                            "(uint64_t)" + array_pack.ver_decode_macro_use(
                                               array_name, array_ref));
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

const std::string num_bits_func = R"(
/**
 * Pass in @param v the number of values to represented
 *
 * @return Number of bits required to represent 0..(v-1)
 */
#include <stdint.h>
unsigned num_bits(uint64_t v) {
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
})";

std::vector<char*> split_to_args(const std::string& str) {
    std::stringstream iss{str};
    std::vector<char*> result;
    for(std::string s; iss >> s;) {
        std::cout << "pet args: " << s << "\n";
        result.push_back(strdup(s.c_str()));
    }
    return result;
}

int main(int argc, char* argv[]) {
    //assert(argc == 5);
    std::string filename{argv[1]};
    std::string target{argv[2]};

    struct pet_options* options = pet_options_new_with_defaults();
    isl_ctx* ctx = isl_ctx_alloc_with_options(&pet_options_args, options);
    if (const char* env_p = std::getenv("POLYCHECK_PET_ARGS")) {
      std::vector<char*> pet_args = split_to_args(env_p);
      pet_args.insert(pet_args.begin(), strdup("blah"));
      isl_ctx_parse_options(ctx, pet_args.size(), pet_args.data(), ISL_ARG_ALL);
    }
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
#if 1
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

    for(const auto stmt : stmts) {
        std::cout << "//TEMPLATE for statement " << stmt.name()
                  << ":\n---------\n"
                  << stmt.inline_check_template() << "---------\n";
    }
    ParseScop(target, stmts, prologue, epilogue, output_file_name(target));

    std::string target_defs{std::string{"defs_"} + argv[2]};
    std::ofstream of{target_defs, ios::out};
    of<<num_bits_func<<"\n";
    of<<array_pack.global_decls()<<"\n";
    of.close();
    std::cout<<"Defs written to "<<target_defs<<"\n";
#endif
    stmts.clear();
#endif
    isl_schedule_free(isched);
    islw::destruct(R, W, S);
    pet_scop_free(scop);
    islw::destruct(ctx);
    return 0;
}