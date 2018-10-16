#ifndef DEPENDENCE_ANALYZER_HPP
#define DEPENDENCE_ANALYZER_HPP

#include <vector>
#include<iostream>
#include <isl/ast_build.h>
#include <isl/ast.h>
#include <isl/schedule.h>
#include <Statement.hpp>
#include <Dependences.hpp>

using namespace std;

class DependenceAnalyzer
{
public:
  static void ComputeDependencies (vector<Statement*>& statements,Dependences* deps);
  static isl_stat MapFromUnionMap(isl_map* eachMap, void* user);
  static isl_stat BasicMapFromMap(isl_basic_map* bMap, void* user);
  static void RemakeReverseMaps(vector<isl_map*>* Rmaps);

  static isl_stat SetFromUnionSet(isl_set* eachSet, void* user);
  static isl_stat BasicSetFromSet(isl_basic_set* bSet, void* user);
};

#endif
