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

#include <polyopt/PolyOpt.hpp>
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
                         ") ^ " + poly_code + ";";
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
  // 1- Read PoCC options.
  // Create argv, argc from string argument list, by removing rose/edg
  // options.
  SgStringList argStrings =
      CommandlineProcessing::generateArgListFromArgcArgv(argc, argv);
  SgFile::stripRoseCommandLineOptions(argStrings);
  SgFile::stripEdgCommandLineOptions(argStrings);

  int newargc = argStrings.size();
  char *newargv[newargc];
  int i = 0;

  std::string transformed_input_source;
  SgStringList::iterator argIter;
  for (argIter = argStrings.begin(); argIter != argStrings.end(); ++argIter) {
    //cout << "args=" << (*argIter) << endl;
    const std::string carg = *argIter;
    if (carg.substr(0, 8) == "-target=") {
      transformed_input_source = carg.substr(8,carg.size());
    }
    else newargv[i++] = strdup(carg.c_str());
  }

  assert(!transformed_input_source.empty());
  cout << "Transformed input source file provided = " << transformed_input_source << endl;

  // 2- Parse PoCC options.
  PolyRoseOptions polyoptions(newargc, newargv);

  // 3- Remove PoCC options from arg list.
  SgStringList args =
      CommandlineProcessing::generateArgListFromArgcArgv(argc, argv);
  CommandlineProcessing::removeArgs(args, "--polyopt-");

  // 4- Invoke the Rose parser.
  SgProject *project = frontend(args);

  std::vector<std::pair<SgNode *, scoplib_scop_p>> polyopt_scops =
      PolyOptRecognizeScopsSubTree(project, polyoptions);

  // std::vector<std::string> args(argv, argv + argc);
  // SgProject *project = frontend(args);
  
  // For now --polyopt-scop-pragmas-only

//   for (auto x: polyopt_scops){
//      cout << "--------------scop--------------------\n";
//      cout << (x.first)->unparseToCompleteString() << endl;
//     cout << "--------------end scop--------------------\n";
//    }

  assert(polyopt_scops.size() == 1);
  ROSE_ASSERT(project != NULL);

  isl_ctx *ctx = isl_ctx_alloc();
  PolyOptISLRepresentation isl_scop =
    PolyOptConvertScopToISL(polyopt_scops[0].second);

  isl_union_map *R = isl_scop.scop_reads;
  // Union of all access functions intersected with domain.
  isl_union_map *W = isl_scop.scop_writes;
  // Union of all schedules.
  isl_union_map *S = isl_scop.scop_scheds;

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