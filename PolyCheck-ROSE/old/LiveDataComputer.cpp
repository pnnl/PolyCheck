#include "LiveDataComputer.hpp"
#include "GeneralUtilityClass.hpp"
#include "DependenceAnalyzer.hpp"

#include <assert.h>
#include <iostream>
#include <sstream>
#include <regex>

using namespace std;

isl_union_set *ApplyUnionMapOnSet(isl_set *set, isl_union_map *map){
  char *setStr, *mapStr;
  {
    __isl_give isl_printer *printer = isl_printer_to_str(isl_set_get_ctx(set));
    isl_printer_print_set (printer, set);
    setStr = isl_printer_get_str(printer);
    // cout << "set string: " << setStr << endl;
  }
  {
    __isl_give isl_printer *printer = isl_printer_to_str(isl_set_get_ctx(set));
    isl_printer_print_union_map (printer, map);
    mapStr = isl_printer_get_str(printer);
    // cout << "map string: " << mapStr << endl;
  }

  isl_set *newSet = isl_set_read_from_str(isl_set_get_ctx(set), setStr);
  isl_union_map* newMap = isl_union_map_read_from_str(isl_set_get_ctx(set), mapStr);

  return isl_union_set_apply (isl_union_set_from_set (isl_set_copy(newSet)), isl_union_map_copy(newMap));
}

string LiveDataComputer::ast_str_from_set(isl_union_set* set){
  isl_printer *printer = isl_printer_to_str(isl_union_set_get_ctx(set));
  isl_printer_print_union_set (printer, set);
  char *setStr = isl_printer_get_str(printer);
  set = isl_union_set_read_from_str(isl_union_set_get_ctx(set), setStr);
  
  printer = isl_printer_to_str(isl_union_set_get_ctx(set));
  isl_union_map *schedule = isl_union_set_identity(set);
  isl_set *context = isl_set_universe(isl_union_map_get_space(schedule));
  isl_ast_build *build = isl_ast_build_from_context(context);
  isl_ast_node *node = isl_ast_build_ast_from_schedule (build, schedule);
  printer = isl_printer_set_output_format(printer, ISL_FORMAT_C);
  isl_printer_print_ast_node (printer, node);
  char *str = isl_printer_get_str(printer);

  return string(str);
}

// A simpler way to generate prologue code
void LiveDataComputer::Generate_prologue_checker(isl_union_set* live_in_uset, vector<string*> *LiveDataCheckerCode) {

  set<isl_set*> *live_in_sets = new set<isl_set*>();
  isl_union_set_foreach_set(isl_union_set_copy(live_in_uset), &DependenceAnalyzer::SetFromUnionSet, live_in_sets);

  string checker_code = "";
  for (isl_set* live_in_set: *live_in_sets){
    string prologue = ast_str_from_set(isl_union_set_from_set(live_in_set));
    //cout << "prologue:" << prologue << endl;
    stringstream ss(prologue);
    string line;

    while(getline(ss,line)) {
      if (line.find("if (")!=string::npos or line.find("for (")!=string::npos) {
        checker_code += line + "\n";
        //(LiveDataCheckerCode)->push_back(new string(line));
        continue;
      }

      regex scalar("\\(\\)");
      if (regex_search(line, scalar)) 
        line = regex_replace(line,scalar,"");

      regex comma(",\\s");
      if (regex_search(line,comma))
        line = regex_replace(line,comma,"][");

      regex left("\\(");
      if (regex_search(line,left))
        line = regex_replace(line,left,"[");
      regex right("\\)");
      if (regex_search(line,right))
        line = regex_replace(line,right,"]");

      regex semi("\\;");
      if (regex_search(line,semi))
        line = regex_replace(line,semi,"=0;");

      checker_code += line + "\n";
      //(LiveDataCheckerCode)->push_back(new string(line));
    }
  }
  (LiveDataCheckerCode)->push_back(new string(checker_code));
}

void LiveDataComputer::ComputeLiveInDataCheckerCode(vector<Statement*> stmtVec, Dependences* dependences, vector<string*> **LiveDataCheckerCode){
  cout << "-----------Live-in Data Checker Code------------" << endl;

  *LiveDataCheckerCode = new vector<string*>();
  isl_union_set* live_in_uset = NULL;

  for (size_t i=0; i<stmtVec.size(); i++) {
    isl_union_set *localLiveInData = ApplyUnionMapOnSet(stmtVec[i]->getDomainSet(), isl_union_map_coalesce(dependences->getLiveIn()));

    if (live_in_uset == NULL) {
      live_in_uset = localLiveInData;
    }else{
      live_in_uset = isl_union_set_union(isl_union_set_copy(live_in_uset), isl_union_set_coalesce(isl_union_set_copy(localLiveInData)));
    }
  }

  cout << "Live in data of Union set: " << endl;
  GeneralUtilityClass::PrintUnionSetCode(live_in_uset,1);

  live_in_uset = isl_union_set_coalesce(live_in_uset);
  Generate_prologue_checker(live_in_uset, *LiveDataCheckerCode);

  //isl_union_map *reverseLiveInMap = isl_union_map_reverse(isl_union_map_coalesce(isl_union_map_copy(dependences->getLiveIn())));

  //GeneralUtilityClass::DataSetPrintUnionSetCode(live_in_uset, 1, reverseLiveInMap, *LiveDataCheckerCode, NULL);//live data has NULL pre ast notation
}





