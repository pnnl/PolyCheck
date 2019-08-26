#include "PolyCheck.hpp"
#include "Orchestrator.hpp"
#include "DebugComputer.hpp"
#include "LiveDataComputer.hpp"
#include "UnusedComputer.hpp"
#include "UnusedDeclComputer.hpp"
#include "CodeParser.hpp"
#include <polyopt/PolyOpt.hpp>

using std::cout;
using std::endl;

//./PolyCheck -rose:skipfinalCompileStep --polyopt-safe-math-func --polyopt-scop-pragmas-only
// cholesky.c -target=cholesky.c -I$(polybench_include_path)


int main(int argc, char *argv[]) {

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

  for (auto x: polyopt_scops){
     cout << "--------------scop--------------------\n";
     cout << (x.first)->unparseToCompleteString() << endl;
    cout << "--------------end scop--------------------\n";
   }

  assert(polyopt_scops.size() == 1);
  ROSE_ASSERT(project != NULL);


  //*****************Analyzing*********************

  Orchestrator fTOrchestrator; 
  ReturnValues ftReturnValues = fTOrchestrator.drive(polyopt_scops);

  /// Match statements in scop and insert corresponding checker code
  ParseScop(transformed_input_source, ftReturnValues);

#if 0
  /// Collect Statements in orig/opt scops to compare syntactically
  // Create the traversal object
  ScopTraversal printScops;
  // Call the traversal starting at the project node of the AST
  printScops.traverseInputFiles(project, preorder);

  const std::string opt_path = "polyopt_runs/rose_";
  std::string opt_src_file;
  int arg_no = 0, src_pos = 0;
  for (auto clo : args) {
    if (CommandlineProcessing::isSourceFilename(clo)) {
      src_pos = arg_no;
      // replace filename with rose_test.c
      std::string base_filename = clo.substr(clo.find_last_of("/\\") + 1);
      std::string::size_type const p(base_filename.find_last_of('.'));
      std::string optimized_source_file = base_filename.substr(0, p);
      opt_src_file = opt_path + optimized_source_file + ".c";
    }
    arg_no++;
  }

  if (!CommandlineProcessing::isSourceFilename(opt_src_file))
    assert(0);
  args[src_pos] = opt_src_file;
  //  for (auto clo : args)
  //    cout << clo << endl;
  SgProject *transformed = frontend(args);
  ROSE_ASSERT(transformed != NULL);

  // Create the traversal object
  ScopTraversal printOptScops;
  printOptScops.source_flag = 1;
  // Call the traversal starting at the project node of the AST
  printOptScops.traverseInputFiles(transformed, preorder);

  compareScops();
  // return backend(project);
  #endif
  return 0;
}


#if 0
using ScopStmtVector = std::map<int, std::vector<SgNode *>>;
ScopStmtVector collectScopStatements;    // map with scop number
ScopStmtVector collectOptScopStatements; // map with scop number

void collect_scop_statements(
    int scop_number, MatchResult &assign_matches,
    std::map<int, std::vector<SgNode *>> &collectScopStatements) {
  for (MatchResult::iterator i = assign_matches.begin();
       i != assign_matches.end(); i++) {
    for (auto p : (*i)) {
      SgNode *anode = p.second;
      if (!isSgForInitStatement(anode->get_parent()->get_parent()))
        collectScopStatements[scop_number].push_back(anode);
    }
  }
}

void printScopStmts(ScopStmtVector &collectScopStatements) {
  for (auto i : collectScopStatements) {
    cout << "Scop " << i.first << ":\n";
    for (auto s : i.second) {
      if (isSgAssignOp(s) || isSgPlusAssignOp(s) || isSgMultAssignOp(s) ||
          isSgMinusAssignOp(s) || isSgDivAssignOp(s)) {
        // if(std::is_base_of<SgCompoundAssignOp*,decltype(s)>::value)
        cout << (s)->unparseToString() << endl;
      } else
        assert(0);
    }
    cout << "----------------------------------\n";
  }
}


 void ScopTraversal::visit(SgNode *node) {
   if (SgPragmaDeclaration *pragma = isSgPragmaDeclaration(node)) {
     const string pragmaName = pragma->get_pragma()->get_pragma();
     // cout << pragmaName << endl;
     if ((pragmaName.size() == 4) && (pragmaName.compare(0, 4, "scop") == 0)) {
       SgStatement *stmt = SageInterface::getNextStatement(pragma);
       this->scop_number += 1;

       while (!isSgPragmaDeclaration(stmt)) {
         /// @todo lhs can be a scalar too.
         // for (auto assign_type : record_pc_assign_nodes) {
         MatchResult assign_matches =
             this->matcher.performMatching(record_pc_assign_nodes, stmt);
         if (this->source_flag == 0)
           collect_scop_statements(this->scop_number, assign_matches,
                                   collectScopStatements);
         else
           collect_scop_statements(this->scop_number, assign_matches,
                                   collectOptScopStatements);
         // }
         stmt = SageInterface::getNextStatement(stmt);
       } // end while
       // cout << stmt->unparseToString() << endl; //#pragma endscop
     } // end begin scop
   }   // end scop

 } // end visitor

#endif