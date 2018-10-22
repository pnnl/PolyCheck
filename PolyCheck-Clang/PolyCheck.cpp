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
    explicit Prolog() : counter_{0} {}
    ~Prolog()             = default;
    Prolog(const Prolog&) = default;
    Prolog(Prolog&&) = default;
    Prolog& operator=(const Prolog&) = default;
    Prolog& operator=(Prolog&&) = default;

    Prolog(isl_union_map* R, isl_union_map* W) 
    : counter_{0} {
        isl_union_set* RW =
          isl_union_set_union(isl_union_map_range(islw::copy(R)),
                              isl_union_map_range(islw::copy(W)));
        isl_union_set_foreach_set(RW, fn_prolog_per_set, this);
        islw::destruct(RW);
    }

    std::string to_string() const {
        return "#include <assert.h>\n int _diff = 0;\n{\n" + pre_ + "\n" +
               stmts_ + "\n" + post_ + "\n}\n";
    }

    private:
    // should be a lambda
    static isl_stat fn_prolog_per_set(isl_set* set, void* user) {
        assert(user != nullptr);
        assert(set != nullptr);
        // std::string& str   = *(std::string*)user;
        Prolog& prolog            = *(Prolog*)user;
        std::string array_name{isl_set_get_tuple_name(set)};
        std::string macro_name{"PC_PR_" + array_name + "_" +
                               std::to_string(prolog.counter_)};
        std::string array_ref_str = array_ref_string(set) + ";";
        std::string macro_ref_string =
          "PC_PR_" +
          replace_all(array_ref_string(set), array_name + "(",
                      array_name + "_" + std::to_string(prolog.counter_)+"(");
        prolog.pre_ += "#define " + macro_ref_string + "\t" +
                       array_ref_string_in_c(set) + " = 0\n";
        prolog.post_ += "#undef " + macro_name + "\n";
        std::string prolog_str = islw::to_c_string(set);
        // std::string array_ref_str = array_ref_string(set) + ";";
        // std::string repl_str      = array_ref_string_in_c(set) + "=0;";
        prolog.stmts_ += replace_all(prolog_str, array_name, macro_name);
        islw::destruct(set);
        prolog.counter_ += 1;
        return isl_stat_ok;
    }

    int counter_;
    std::string pre_;
    std::string stmts_;
    std::string post_;
}; // class Prolog

#if 0
//should be a lambda
isl_stat fn_prolog_per_set(isl_set* set, void* user) {
    std::string& str   = *(std::string*)user;
    std::string prolog = islw::to_c_string(set);
    std::string array_ref_str = array_ref_string(set) + ";";
    std::string repl_str = array_ref_string_in_c(set) + "=0;";
    str += replace_all(prolog, array_ref_str, repl_str);
    islw::destruct(set);
    return isl_stat_ok;
}

std::string prolog(isl_union_map* R, isl_union_map* W) {
    std::string prolog;
    isl_union_set* RW = isl_union_set_union(isl_union_map_range(islw::copy(R)),
                                            isl_union_map_range(islw::copy(W)));
    isl_union_set_foreach_set(RW, fn_prolog_per_set, &prolog);
    prolog = "#include <assert.h>\n int _diff = 0;\n{\n" + prolog + "\n}\n";
    islw::destruct(RW);
    return prolog;
}
#endif

std::string prolog(isl_union_map* R, isl_union_map* W) {
    Prolog prolog{R, W};
    return prolog.to_string();
}
class Epilog {
    public:
    explicit Epilog() : counter_{0} {}
    ~Epilog()             = default;
    Epilog(const Epilog&) = default;
    Epilog(Epilog&&) = default;
    Epilog& operator=(const Epilog&) = default;
    Epilog& operator=(Epilog&&) = default;

    Epilog(isl_union_map* W) 
    : counter_{0} {
        isl_union_pw_qpolynomial* WC =
          isl_union_map_card(isl_union_map_reverse(islw::copy(W)));
        isl_union_pw_qpolynomial_foreach_pw_qpolynomial(
          WC, Epilog::epilog_per_poly, this);
        islw::destruct(WC);
    }

    std::string to_string() const {
        return "{\n" + macro_defs_ + "\n" + checks_ + "\n" + macro_undefs_ +
             "\n assert(_diff == 0); \n"+
               "}\n";
    }

    private:
    // should be a lambda
    static isl_stat epilog_per_poly(isl_pw_qpolynomial* pwqp, void* user) {
        isl_pw_qpolynomial_foreach_piece(pwqp, Epilog::epilog_per_poly_piece, user);
        islw::destruct(pwqp);
        return isl_stat_ok;
    }

    // should be a lambda
    static isl_stat epilog_per_poly_piece(isl_set* set, isl_qpolynomial* poly,
                                   void* user) {
        // std::string& str = *(std::string*) user;
        assert(user != nullptr);
        Epilog& ep           = *(Epilog*)user;
        std::string set_code      = islw::to_c_string(set);
        std::string poly_code     = islw::to_c_string(poly);
        std::string array_ref_str = array_ref_string(set);
#if 0
  std::string repl_str      = "_diff |= ((int) " + array_ref_string_in_c(set) +
                         ") ^ (" + poly_code + ");";
  str += replace_all(set_code, array_ref_str, repl_str);
#else
        //   std::string repl_str      = "_diff |= ((int) pc_ep_" +
        //   array_ref_string(set) +
        //                          ") ^ (" + poly_code + ");";
        //   str += replace_all(set_code, array_ref_str, repl_str);
        std::string array_name{isl_set_get_tuple_name(set)};
        std::string macro_name =
          "PC_EP_" + array_name + "_" + std::to_string(ep.counter_);
        std::string macro_ref_str =
          replace_all(array_ref_str, array_name + "(", macro_name + "(");
        std::string def_str =
          "#define " + macro_ref_str + "\t" + "((int)" +
          replace_all(
            replace_all(replace_all(replace_all(array_ref_str, array_name + "(",
                                                array_name + "["),
                                    ")", "]"),
                        ",", "]["),
            "[]", "") +
          ")\n";
        std::string undef_str = "#undef " + macro_name + "\n";
        std::string check_str = "_diff |= (" + poly_code + ") ^ " + macro_name;
        std::string repl_name = check_str;
        ep.macro_defs_ += def_str;
        ep.macro_undefs_ += undef_str;
        ep.checks_ += replace_all(set_code, array_name + "(", repl_name + "(");
        ep.counter_ += 1;
        //   std::cerr<<"$$$$$$$$$$ set before:"<<set_code<<"\n";
        //   std::cerr<<"$$$$$$$$$$ cum. set code after:"<<str<<"\n";
#endif
        islw::destruct(set);
        islw::destruct(poly);
        return isl_stat_ok;
    }

    int counter_;
    std::string macro_defs_;
    std::string macro_undefs_;
    std::string checks_;
};  // class Epilog

#if 0
//should be a lambda
isl_stat epilog_per_poly_piece(isl_set* set, isl_qpolynomial* poly, void *user) {
  //std::string& str = *(std::string*) user;
  assert(user != nullptr);
  EpilogState& es = *(EpilogState*)user;
  std::string set_code = islw::to_c_string(set);
  std::string poly_code = islw::to_c_string(poly);
  std::string array_ref_str = array_ref_string(set);
  #if 0
  std::string repl_str      = "_diff |= ((int) " + array_ref_string_in_c(set) +
                         ") ^ (" + poly_code + ");";
  str += replace_all(set_code, array_ref_str, repl_str);
  #else
//   std::string repl_str      = "_diff |= ((int) pc_ep_" + array_ref_string(set) +
//                          ") ^ (" + poly_code + ");";
//   str += replace_all(set_code, array_ref_str, repl_str);
  std::string array_name{isl_set_get_tuple_name(set)};
  std::string macro_name =
    "PC_EP_" + array_name + "_" + std::to_string(es.counter_);
  std::string macro_ref_str =
    replace_all(array_ref_str, array_name + "(", macro_name + "(");
  std::string def_str =
    "#define " + macro_ref_str + "\t" + "((int)" +
    replace_all(
      replace_all(replace_all(replace_all(array_ref_str, array_name + "(",
                                          array_name + "["),
                              ")", "]"),
                  ",", "]["),
      "[]", "") +
    ")\n";
  std::string undef_str = "#undef " + macro_name + "\n";
  std::string check_str = "_diff |= ("+poly_code+") ^ "+macro_name;
  std::string repl_name = check_str;
  es.macro_defs_ += def_str;
  es.macro_undefs_ += undef_str;
  es.checks_ += replace_all(set_code, array_name+"(", repl_name+"(");
  es.counter_ += 1;
//   std::cerr<<"$$$$$$$$$$ set before:"<<set_code<<"\n";
//   std::cerr<<"$$$$$$$$$$ cum. set code after:"<<str<<"\n";
  #endif
  islw::destruct(set);
  islw::destruct(poly);
  return isl_stat_ok;
}

//should be a lambda
isl_stat epilog_per_poly(isl_pw_qpolynomial* pwqp, void *user) {
  isl_pw_qpolynomial_foreach_piece(pwqp, epilog_per_poly_piece, user);
  islw::destruct(pwqp);
  return isl_stat_ok;
}

std::string epilog(isl_union_map* W) {
    std::string epilog;
    isl_union_pw_qpolynomial* WC =
      isl_union_map_card(isl_union_map_reverse(islw::copy(W)));
    // epilog = "{\n";
    EpilogState es;
    isl_union_pw_qpolynomial_foreach_pw_qpolynomial(WC, epilog_per_poly,
                                                    &es);
    // epilog += "\n assert(_diff == 0); \n";
    // epilog += "\n}\n";
    islw::destruct(WC);
    return es.to_string();
}
#endif

std::string epilog(isl_union_map* W) {
    Epilog epilog{W};
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
    const std::string epilogue = epilog(W);
    std::cout << "Prolog\n------\n" << prologue << "\n----------\n";
    std::cout << "Epilog\n------\n" << epilogue << "\n----------\n";
    //std::cout << "Inline checks\n------\n";
    // for(size_t i=0; i<inline_checks.size(); i++) {
    //     std::cout<<"  Statement "<<i<<"\n  ---\n"<<inline_checks[i]<<"\n";
    // }
    std::cout<<"-------\n";

    ParseScop(target, stmts, prologue, epilogue, output_file_name(target));
    stmts.clear();
    pet_scop_free(scop);
    isl_schedule_free(isched);
    islw::destruct(R, W, S, ctx);
    return 0;
}