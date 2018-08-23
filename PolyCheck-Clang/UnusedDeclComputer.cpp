#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <regex>

#include <assert.h>
#include <barvinok/isl.h>
#include <GeneralUtilityClass.hpp>

#include <DebugComputer.hpp>
#include <DependenceAnalyzer.hpp>
#include <UnusedDeclComputer.hpp>

void UnusedDeclComputer::ComputeUnusedDeclCheckerCode(isl_union_map* R, isl_union_map* W, vector<string*> **UnusedDeclCheckerCode){
  *UnusedDeclCheckerCode = new vector<string*>();
  vector<string> remove_duplicates;

  isl_union_map* read = isl_union_map_copy(R);
  isl_union_map* write = isl_union_map_copy(W);

  isl_union_set* isl_access_uset = isl_union_map_range(isl_union_map_union(read, write));
  isl_union_set* isl_access_lexmax = isl_union_set_lexmax(isl_union_set_coalesce(isl_access_uset));

  // Epilogue declarations
  set<isl_set*> *epilogue_decls = new set<isl_set*>();
  isl_union_set_foreach_set(isl_union_set_copy(isl_access_lexmax), &DependenceAnalyzer::SetFromUnionSet, epilogue_decls);
  
  for (isl_set* isl_access_set: *epilogue_decls){
    string decl_str = DumpArrayDecl(DebugComputer::string_from_isl_set(isl_access_set));

    string *epilogue_decl_code = new string("");
    *epilogue_decl_code += "int shadow_" + decl_str + ";\n";

    string arrname = decl_str.substr(0,decl_str.find_first_of("["));
   
    if (std::find(remove_duplicates.begin(), remove_duplicates.end(), arrname) == remove_duplicates.end())
    {
      // Element not in vector.
        remove_duplicates.push_back(arrname);
        (*UnusedDeclCheckerCode)->push_back(epilogue_decl_code);
        cout << "decl: " << *epilogue_decl_code << endl;
    }
   }
  
}

string decl_from_isl_array(string isl_array) {
  string res;
  regex comma(",\\s");
  regex paren("\\]");

  if (regex_search(isl_array, comma))
    res = regex_replace (isl_array,comma,"][");

  if (res.empty()) res=isl_array;
  if (regex_search(res, paren))
    res = regex_replace (res,paren,"+1]");

  bool flag = false;
  for(auto &c: res) {
    if (flag == true) 
      c = toupper(c);

    if (c=='[') flag = true;
    else if (c==']') flag = false;
  }

  return res;
}

string UnusedDeclComputer::DumpArrayDecl(string set_str){
  string res;
   
  regex scalar("\\[\\]");
  regex base_regex(".*\\{\\s(\\w+\\[.*\\]).*\\}");
  smatch base_match;
  if (!regex_match(set_str, base_match, base_regex)) return res;

  if (base_match.size() == 2) {
    ssub_match base_sub_match = base_match[1];
    res = base_sub_match.str();

    if (regex_search(res, scalar)){
      //cout << "warning: scalar array..." << endl;
      res = regex_replace (res,scalar,"");
      return res;
    } 
  }
  return decl_from_isl_array(res);
}





