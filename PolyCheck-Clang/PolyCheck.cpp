#include <string>
#include <iostream>
#include <regex>
#include <cassert>
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

class Prolog {
    public:
    explicit Prolog() {}
    ~Prolog()             = default;
    Prolog(const Prolog&) = default;
    Prolog(Prolog&&) = default;
    Prolog& operator=(const Prolog&) = default;
    Prolog& operator=(Prolog&&) = default;

    Prolog(isl_union_map* R, isl_union_map* W) {
        isl_union_set* RW =
          isl_union_set_union(isl_union_map_range(islw::copy(R)),
                              isl_union_map_range(islw::copy(W)));
        isl_union_set_foreach_set(RW, fn_prolog_per_set, &this->str_);
        islw::destruct(RW);
    }

    std::string to_string() const {
        return "#include <assert.h>\n int _diff = 0;\n{\n" + str_ + "\n}\n";
    }

    private:
    // should be a lambda
    static isl_stat fn_prolog_per_set(isl_set* set, void* user) {
        assert(user != nullptr);
        assert(set != nullptr);
        std::string& prolog = *(std::string*)user;
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
          "#define PC_PR(" + join(args_vec, ",") + ")\t" + array_ref + " = 1\n";
        std::string post = "#undef PC_PR\n";
        prolog +=
          pre + replace_all(set_code, array_name + "(", "PC_PR(") + post;
        islw::destruct(set);
        return isl_stat_ok;
    }

    std::string str_;
}; // class Prolog

std::string prolog(isl_union_map* R, isl_union_map* W) {
    Prolog prolog{R, W};
    return prolog.to_string();
}
class Epilog {
    public:
    explicit Epilog() {}
    ~Epilog()             = default;
    Epilog(const Epilog&) = default;
    Epilog(Epilog&&) = default;
    Epilog& operator=(const Epilog&) = default;
    Epilog& operator=(Epilog&&) = default;

    Epilog(isl_union_map* R, isl_union_map* W) {
        isl_union_pw_qpolynomial* WC =
          isl_union_map_card(isl_union_map_reverse(islw::copy(W)));
        isl_union_pw_qpolynomial_foreach_pw_qpolynomial(
          WC, Epilog::epilog_per_poly, &this->str_);
        isl_union_set* rset = isl_union_map_range(islw::copy(R));
        isl_union_set* wset = isl_union_map_range(islw::copy(W));
        isl_union_set* ronly_set =
          isl_union_set_subtract(islw::copy(rset), islw::copy(wset));
        isl_union_set_foreach_set(ronly_set, Epilog::epilog_per_ronly_piece,
                                  &this->str_);
        islw::destruct(rset);
        islw::destruct(wset);
        islw::destruct(ronly_set);
        islw::destruct(WC);
    }

    std::string to_string() const {
        return "{\n"+str_+"\n assert(_diff == 0); \n}\n";
    }

    private:
    // should be a lambda
    static isl_stat epilog_per_ronly_piece(isl_set* iset, void* user) {
        assert(iset);
        assert(user);
        std::string& ep = *(std::string*)user;
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
        std::string ep_macro_def = "#define PC_EP_CHECK(" +
                                   join(args_vec, ",") +
                                   ")\t _diff |= 1 ^ (int)" + array_ref + "\n";
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
        assert(isl_set_dim(set, isl_set_dim) ==
               isl_qpolynomial_dim(poly, isl_dim_in));
        assert(user != nullptr);
        std::string& ep = *(std::string*)user;
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
          poly_code + ")) ^ (int)" + array_ref + "\n";
        std::string ep_macro_undef = "#undef PC_EP_CHECK\n";
        ep += ep_macro_def +
              replace_all(set_code, array_name + "(", "PC_EP_CHECK(") +
              ep_macro_undef;
        islw::destruct(set);
        islw::destruct(poly);
        return isl_stat_ok;
    }

    std::string str_;
};  // class Epilog


std::string epilog(isl_union_map* R, isl_union_map* W) {
    Epilog epilog{R, W};
    return epilog.to_string();
}

std::string output_file_name(std::string file_name) {
    size_t pos = file_name.find_last_of("/");
    if(pos == std::string::npos) {
        return file_name.insert(0, "pc_");
    } else {
        return file_name.insert(pos+1, "pc_");
    }
}

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

    std::cout << "#statements=" << scop->n_stmt << "\n";

    //std::vector<std::string> inline_checks;
    std::vector<Statement> stmts;
    {
        for(int i = 0; i < scop->n_stmt; i++) {
            stmts.push_back(Statement{i, scop});
        }
        // for(const auto& stmt : stmts) {
        //     inline_checks.push_back(stmt.inline_checks());
        // }
    }

    const std::string prologue = prolog(R, W) + "\n";
    const std::string epilogue = epilog(R, W);
    std::cout << "Prolog\n------\n" << prologue << "\n----------\n";
    std::cout << "Epilog\n------\n" << epilogue << "\n----------\n";
    //std::cout << "Inline checks\n------\n";
    // for(size_t i=0; i<inline_checks.size(); i++) {
    //     std::cout<<"  Statement "<<i<<"\n  ---\n"<<inline_checks[i]<<"\n";
    // }
    std::cout<<"-------\n";

    ParseScop(target, stmts, prologue, epilogue, output_file_name(target));
    stmts.clear();
    isl_schedule_free(isched);
    islw::destruct(R, W, S);
    pet_scop_free(scop);
    islw::destruct(ctx);
    return 0;
}