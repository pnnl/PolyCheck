#ifndef ORCHESTRATOR_HPP
#define ORCHESTRATOR_HPP

#include <string>
#include <vector>
#include <map>

#include <polyopt/PolyOpt.hpp>
#include "Statement.hpp"
#include "DependenceAnalyzer.hpp"
#include "AstTraversal.h"
using std::vector;

struct ReturnValues
{
  vector<string*> *LiveDataCheckerCode;
  vector<string*> *UnusedDataCheckerCode;
  vector<Statement*> stmtVecReturn;
  // Epilogue Declares
  vector<string*> *EpilogueDeclCode;
};

class Orchestrator
{
public:
  Orchestrator();
  ~Orchestrator();

  ReturnValues drive(std::vector<std::pair<SgNode *, scoplib_scop_p>>& polyopt_scops);
  vector<Statement*> GetStatements();
  void FormStatements(isl_ctx* ctx, PolyOptISLRepresentation& scop);

private:
  vector<Statement*> stmtVec;
  Dependences* dependences;
};

void
 collectRefsAndOps(SgNode* node,
					std::vector<string>* allRefs);

  
#endif
