#ifndef UNUSEDDECLCOMPUTER_HPP
#define UNUSEDDECLCOMPUTER_HPP

#include <isl/ast_build.h>
#include <isl/ast.h>
#include <isl/schedule.h>
#include <isl/set.h>
#include <isl/union_map.h>
#include <isl/ctx.h>
#include <isl/map.h>
#include <isl/flow.h>
#include <Statement.hpp>
#include <Dependences.hpp>

#include <map>
#include <string>
#include <vector>
#include <set>

using namespace std;

class UnusedDeclComputer
{
  public:
    static void ComputeUnusedDeclCheckerCode(isl_union_map* R, isl_union_map* W, vector<string*> **UnusedDeclCheckerCode);

    static string DumpArrayDecl(string set_str);
};
#endif
