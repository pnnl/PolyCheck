#ifndef UNUSEDCOMPUTER_HPP
#define UNUSEDCOMPUTER_HPP

#include <isl/ast_build.h>
#include <isl/ast.h>
#include <isl/schedule.h>
#include <isl/set.h>
#include <isl/union_map.h>
#include <isl/ctx.h>
#include <isl/map.h>
#include <isl/flow.h>
#include "Statement.hpp"
#include "Dependences.hpp"

#include <map>
#include <set>
#include <string>
#include <vector>

using namespace std;

class UnusedComputer
{
  public:
    static void ComputeUnusedDataCheckerCode(isl_union_map* R, isl_union_map* W, vector<Statement*> stmtVec, Dependences* dependences, vector<string*> **UnusedDataCheckerCode);

    static void DumpSetIteratorsToList(isl_set* set, vector<string>& res);

};
#endif
