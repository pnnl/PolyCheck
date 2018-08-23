#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <regex>

#include <barvinok/isl.h>
#include "UnusedComputer.hpp"
#include "GeneralUtilityClass.hpp"
#include "LiveDataComputer.hpp"
#include <assert.h>

#include "DebugComputer.hpp"
#include "DependenceAnalyzer.hpp"

void UnusedComputer::DumpSetIteratorsToList(isl_set* set, vector<string>& res){
  string set_str = DebugComputer::string_from_isl_set(set);

  regex space("\\s+");
  regex base_regex(".*\\{\\s\\w+\\[(.*)\\].*\\}");
  smatch base_match;

  if (!regex_match(set_str, base_match, base_regex)) return ;

  for(size_t i=1; i<base_match.size(); i++) {
    ssub_match base_sub_match = base_match[i];
    string sub_match_str = base_sub_match.str();
    if (sub_match_str.empty()) {
      cout << "scalar" << endl;
      //res.push_back("0");//scalar return 0 as iter
      continue;
    }
    sub_match_str = regex_replace (sub_match_str,space,"");
    stringstream ss(sub_match_str);
    string tmp;
    while(getline(ss,tmp,',')) 
      res.push_back(tmp);
  }
}

// A simpler way to generate epilogue code
void Generate_epilogue_checker(isl_union_set* unused_data_uset, vector<string*> *UnusedDataCheckerCode) {

  set<isl_set*> *unused_sets = new set<isl_set*>();
  isl_union_set_foreach_set(isl_union_set_copy(unused_data_uset), &DependenceAnalyzer::SetFromUnionSet, unused_sets);

  string checker_code = "";
  for (isl_set* unused_set: *unused_sets){
    string epilogue = LiveDataComputer::ast_str_from_set(isl_union_set_from_set(unused_set));
    stringstream ss(epilogue);
    string line;

    while(getline(ss,line)) {
      if (line.find("if (")!=string::npos or line.find("for (")!=string::npos) {
        checker_code += line + "\n";
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
        line = regex_replace(line,semi,"");

      regex space("\\s+");
      if (regex_search(line,space))
        line = regex_replace(line,space,"");

      checker_code += "\t\tchecksum += (unsigned)( shadow_" + line + " ^ (int)"+line+");\n";
    }
  }
  (UnusedDataCheckerCode)->push_back(new string(checker_code));
}




void UnusedComputer::ComputeUnusedDataCheckerCode(isl_union_map* R, isl_union_map* W, vector<Statement*> stmtVec,  Dependences* dependences, vector<string*> **UnusedDataCheckerCode){
  cout << "-----------Unused data checker code------------"  << endl;

  isl_union_map* read  = isl_union_map_copy(R);
  isl_union_map* write = isl_union_map_copy(W);
  isl_union_set* isl_acc_uset = isl_union_map_range(isl_union_map_union(read, write));
  isl_acc_uset = isl_union_set_coalesce(isl_acc_uset);

  set<isl_set*> *acc_sets = new set<isl_set*>();
  isl_union_set_foreach_set(isl_acc_uset, &DependenceAnalyzer::SetFromUnionSet, acc_sets);

  // key : array reference; value : checker code list
  map<string,vector<string*>> *isl_ast_map = new map<string,vector<string*>>();

  isl_union_set* unused_data_uset = nullptr;
  for (isl_set* acc_set: *acc_sets){
    string str = DebugComputer::string_from_isl_set(isl_set_copy(acc_set));

    if (unused_data_uset == nullptr) {
      unused_data_uset = isl_union_set_from_set(acc_set);
    }else{
      unused_data_uset = isl_union_set_union(isl_union_set_from_set(acc_set),unused_data_uset);
    }

    // Process if set is not empty
    //if (!isl_set_is_empty(isl_set_copy(acc_set))) {
      //string* ast_str_pt = new string("");

      //vector<string> iter_vec;
      //DumpSetIteratorsToList(isl_set_copy(acc_set), iter_vec);

      //string array = GeneralUtilityClass::GetSetName(isl_set_copy(acc_set));
      //*ast_str_pt += "\t\tchecksum += (unsigned)( shadow_" + array;

      //for (size_t j=0; j < iter_vec.size(); j++)
        //*ast_str_pt += "[" + iter_vec[j] + "]";
      //*ast_str_pt += " ^ (int) "+ array;

      //for (size_t j=0; j < iter_vec.size(); j++)
        //*ast_str_pt += "[" + iter_vec[j] + "]";
      //*ast_str_pt += " );\n";

      //(*isl_ast_map)[array].push_back(ast_str_pt);
      ////cout << "array: " << array << endl;
      ////cout << "ast_str_pt: " << *ast_str_pt << endl;
    //}
  }

  unused_data_uset = isl_union_set_coalesce(unused_data_uset);
  //isl_union_set_dump(unused_data_uset);
  GeneralUtilityClass::PrintUnionSetCode(unused_data_uset, 1);

  *UnusedDataCheckerCode = new vector<string*>();
  Generate_epilogue_checker(unused_data_uset, *UnusedDataCheckerCode);
  //GeneralUtilityClass::DataSetPrintUnionSetCode(unused_data_uset, 1, NULL, *UnusedDataCheckerCode, isl_ast_map);
}


