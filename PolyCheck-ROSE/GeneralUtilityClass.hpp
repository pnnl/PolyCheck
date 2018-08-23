#ifndef GENERAL_UTILITY_CLASS_HPP
#define GENERAL_UTILITY_CLASS_HPP

#include <string>
#include <vector>

#include <isl/ast_build.h>
#include <isl/ast.h>
#include <isl/schedule.h>
#include <isl/set.h>
#include <isl/union_map.h>
#include <isl/ctx.h>
#include <isl/map.h>
#include <isl/flow.h>
#include <isl/polynomial.h>
#include <isl/aff.h>
#include <isl/point.h>
#include <isl/union_set.h>
#include <map>

using namespace std;

struct LiveDataInput
{
  isl_union_map* reverseLiveInMap;
  vector<string*> *LiveDataCheckerCode;
  map<string, vector<string*> > *astNoteMap;
};

class GeneralUtilityClass
{
public:
  //static void DataSetPrintUnionSetCode (isl_union_set* set, int remake, isl_union_map *reverseLiveInMap, vector<string*> *LiveDataCheckerCode, string *astNotation);
  static void DataSetPrintUnionSetCode (isl_union_set* set, int remake, isl_union_map *reverseLiveInMap, vector<string*> *LiveDataCheckerCode, map<string, vector<string*> > *astNoteMap);
  static isl_stat LiveSetApplyReverseLiveMapOnLiveInSet (isl_set *set, void *user);
  static isl_printer* PrintingFunction (isl_printer *p, isl_ast_print_options *options, isl_ast_node *node, void *user);
  static isl_ast_node* CreateCustomNode (isl_ast_node *leaf_node, isl_ast_build* build, void* user);
  static isl_id_list *Generate_ISL_ID_List(isl_ctx *ctx, vector<string*>  &iteratorNames);
  static void CollectIteratorNames(vector<string*>  &iteratorNames, isl_set *);
  static string ConstructArrayReference(isl_set* set);
  static string ConvertIntToString(int Number);
  static string GetSetName(isl_set *set);

  /* Extra Functions */
  static isl_union_set *ApplyUnionMapOnSet(isl_set *set, isl_union_map* map, int remake = 0);
  static isl_union_set *ApplyUnionMapOnUnionSet(isl_union_set *set, isl_union_map* map, int remake = 0);
  static isl_union_set *ApplyMapOnUnionSet(isl_union_set *set, isl_map* map);
  static void PrintSetCode (isl_set* set);
  static void PrintUnionSetCode (isl_union_set* set, int remake = 0);
  static void PrintUnion_pw_qpolynomialCode(isl_union_pw_qpolynomial *poly, vector<string*> **use_counts);
  static void PrintPw_qpolynomialCode(isl_pw_qpolynomial *poly);
  static isl_stat PrintPw_qpolynomialCode(isl_pw_qpolynomial *poly, void *user);

};

#endif
