#ifndef CODE_PARSER_HPP
#define CODE_PARSER_HPP

#include "Orchestrator.hpp"
#include "SgNodeHelper.h"
#include "rose.h"

//record assignment statements in each scop in input program
using ScopStmtVector = std::map<int, std::vector<SgNode *>>;

/// The main driver to match statements in original & transformed scop and insert corresponding checker code
int ParseScop(string transformed_input_source, ReturnValues ftReturnValues);

/// ROSE AST walk - invoked from ParseScop(...)
class CodeParser: public AstSimpleProcessing {
public:

  /// Override visit in AstSimpleProcessing
  virtual void visit(SgNode* n);

  /// Used to recognize statements in scop.
  /// True while scop nodes are traversed during AST walk
  bool mark_scop;

  /// returned by Orchestrator
    vector<string*> *Epilogue;
    vector<string*> *Prologue;
    vector<string*> *EpilogueDecls;
    vector<Statement*> statements;

  /// initialize data retuned by Orchestrator's driver.
    CodeParser(vector<Statement*> statements, vector<string*> *EpilogueDecls, vector<string*> *Epilogue, vector<string*> *Prologue):
        mark_scop(false), statements(statements),EpilogueDecls(EpilogueDecls), Epilogue(Epilogue), Prologue(Prologue) {}
};

/// Type of AST nodes to match within a scop - all possible assignments.
 const std::string pc_assign_nodes = {"$R=SgAssignOp(_,_) | "
                                   "$R=SgPlusAssignOp(_,_) | "
                                   "$R=SgAndAssignOp(_,_) | "
                                   "$R=SgDivAssignOp(_,_) | "
                                   "$R=SgXorAssignOp(_, _) | "
                                   "$R=SgIntegerDivideAssignOp(_,_) | "
                                   "$R=SgIorAssignOp(_,_) | "
                                   "$R=SgLshiftAssignOp(_,_) | "
                                   "$R=SgMinusAssignOp(_,_) | "
                                   "$R=SgModAssignOp(_,_) | "
                                   "$R=SgMultAssignOp(_,_) | "
                                   "$R=SgRshiftAssignOp(_,_) | "
                                   "$R=SgExponentiationAssignOp(_,_)"};

#endif

