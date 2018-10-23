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

//should be a lambda
isl_stat epilog_per_poly_piece(isl_set* set, isl_qpolynomial* poly, void *user) {
  std::string& str = *(std::string*) user;
  std::string set_code = islw::to_c_string(set);
  std::string poly_code = islw::to_c_string(poly);
  std::string array_ref_str = array_ref_string(set) + ";";
  std::string repl_str      = "_diff |= ((int)" + array_ref_string_in_c(set) +
                         ") ^ (" + poly_code + ");";
  str += replace_all(set_code, array_ref_str, repl_str);
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
    epilog = "{\n";
    isl_union_pw_qpolynomial_foreach_pw_qpolynomial(WC, epilog_per_poly,
                                                    &epilog);
    epilog += "\n assert(_diff == 0); \n";
    epilog += "\n}\n";
    islw::destruct(WC);
    return epilog;
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