#ifndef  LIVEDATACOMPUTER_HPP
#define  LIVEDATACOMPUTER_HPP

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
#include <string>
#include <vector>

using namespace std;

class LiveDataComputer
{
  public:
    static void ComputeLiveInDataCheckerCode(vector<Statement*> stmtVec, Dependences* dependences, vector<string*> **LiveDataCheckerCode);
    static void Generate_prologue_checker(isl_union_set* live_in_uset, vector<string*> *LiveDataCheckerCode);
    static string ast_str_from_set(isl_union_set* set);
};

#endif
