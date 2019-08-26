#ifndef DEBUGCOMPUTER_HPP
#define DEBUGCOMPUTER_HPP

#include <map>
#include <unordered_map>
#include <vector>
#include <iostream>

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

using namespace std;


class DebugComputer
{
public:
  static void ComputeDebugCode(vector<Statement*>& stmtVec, Dependences* dependences, isl_union_map* R, isl_union_map* W, isl_union_map* S);
  
  static isl_union_map* ComputerVersion(isl_map* map, isl_union_map* W, isl_union_map* S);
  static string DumpArrayRef(string map_str);
  static string DumpArrayNum(string map_str);
  static string DumpConstraint(isl_map* map);
  static void DumpMapIteratorsToList(isl_map* map, vector<string>& res);

  static bool MissingConstraint(isl_set* dSet,isl_set* rSet, string* missingCst);
  static vector<string> DumpMissingIterators(isl_set* set);
  static string string_from_isl_map(isl_map* map);
  static string string_from_isl_set(isl_set* set);

};

#endif
