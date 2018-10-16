#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <regex>


#include <GeneralUtilityClass.hpp>
#include <DependenceAnalyzer.hpp>
#include <DebugComputer.hpp>
#include <assert.h>
#include <barvinok/isl.h>
#include <barvinok/barvinok.h>

using namespace std;

isl_union_map* DebugComputer::ComputerVersion(isl_map* map, isl_union_map* W, isl_union_map* S) {
	isl_union_access_info *access;
	isl_union_flow *flow;
	isl_union_map *may_source = NULL;
	isl_union_map *sink = NULL;
	isl_union_map *may_dep;

  may_source = W;
  sink = isl_union_map_from_map(map);

	access = isl_union_access_info_from_sink(sink);
	access = isl_union_access_info_set_may_source(access, may_source);
  access = isl_union_access_info_set_schedule_map(access, S);
	flow = isl_union_access_info_compute_flow(access);
	may_dep = isl_union_flow_get_may_dependence(flow);
  may_dep = isl_union_map_coalesce(may_dep);
  //isl_union_map_dump(isl_union_map_reverse(may_dep));

  return isl_union_map_reverse(may_dep);
}
string DebugComputer::string_from_isl_map(isl_map* map) {
  if(map==nullptr) return "";
  char *str;
  
  __isl_give isl_printer *printer = isl_printer_to_str(isl_map_get_ctx(map));
  isl_printer_print_map(printer, map);
  str = isl_printer_get_str(printer);
  //cout << "map string: " << str << endl;

  return string(str);
}
string DebugComputer::string_from_isl_set(isl_set* set) {
  char *str;
  
  __isl_give isl_printer *printer = isl_printer_to_str(isl_set_get_ctx(set));
  isl_printer_print_set(printer, set);
  str = isl_printer_get_str(printer);
  //cout << "map string: " << str << endl;

  return string(str);
}

string DebugComputer::DumpArrayRef(string map_str){
  string res;

  regex scalar("\\[\\]");
  regex base_regex(".*\\{.*->\\s(\\w+\\[.*\\]).*\\}");
  smatch base_match;
  if (!regex_match(map_str, base_match, base_regex)) return res;

  if (base_match.size() == 2) {
    ssub_match base_sub_match = base_match[1];
    res = base_sub_match.str();

    if (regex_search(res, scalar)){
      //cout << "warning: scalar array..." << endl;
      res = regex_replace (res,scalar,"");
    }
  }
  return res;
}

string DebugComputer::DumpArrayNum(string map_str) {
  string res;

  regex base_regex(".*\\{.*\\sS_(\\d+)\\[.*->.*\\}");
  smatch base_match;
  if (!regex_match(map_str, base_match, base_regex)) return res;

  if (base_match.size() == 2) {
    ssub_match base_sub_match = base_match[1];
    res = base_sub_match.str();
  }
  return res;
}

void DebugComputer::DumpMapIteratorsToList(isl_map* map, vector<string>& res){
  string map_str = string_from_isl_map(map);

  regex space("\\s+");
  regex base_regex(".*\\{.*->\\s\\w+\\[(.*)\\].*\\}");
  smatch base_match;

  if (!regex_match(map_str, base_match, base_regex)) return ;

  for(size_t i=1; i<base_match.size(); i++) {
    ssub_match base_sub_match = base_match[i];
    string sub_match_str = base_sub_match.str();
    if (sub_match_str.empty()) {
      cout << "scalar" << endl;
      continue;
    }
    sub_match_str = regex_replace (sub_match_str,space,"");
    stringstream ss(sub_match_str);
    string tmp;
    while(getline(ss,tmp,',')) 
      res.push_back(tmp);
  }
}

string array_from_isl_array(string isl_array) {
  string res;
  regex pattern(",\\s");

  if (regex_search(isl_array, pattern))
    res = regex_replace (isl_array,pattern,"][");
  else 
    res = isl_array;
  
  return res;
}

string DebugComputer::DumpConstraint(isl_map* map) {
  string res;
  string map_str = string_from_isl_map(map);

  regex base_regex(".*->.*\\[.*\\].*\\:(.*)\\}");
  smatch base_match;
  if (regex_match(map_str, base_match, base_regex)) {
    if (base_match.size() == 2) {
      ssub_match base_sub_match = base_match[1];
      res = base_sub_match.str();
    }
  }else{
    cout << "Cannot find constraint in isl_map..." << endl;
  }

  return res;
}
string cst_from_isl_string(string isl_str) {
  string res;

  regex base_regex(".*\\{\\s*.*\\[.*\\]\\s*\\:\\s*(.*)\\s*\\}");
  smatch base_match;
  if (regex_match(isl_str, base_match, base_regex)) {
    if (base_match.size() == 2) {
      ssub_match base_sub_match = base_match[1];
      if (base_sub_match.str().empty()) 
        cout << "No constraint found in string!" <<endl;
      res = base_sub_match.str();
    }
  }

  return res;
}

bool DebugComputer::MissingConstraint(isl_set* dSet,isl_set* rSet, string* missingCst){
  bool missing = false;

  vector<string> dSetIter = DumpMissingIterators(dSet);
  vector<string> rSetIter = DumpMissingIterators(rSet);

  //cout << "\n : " << dSetIter.size() << ": " << rSetIter.size() << endl;
  assert( dSetIter.size() == rSetIter.size()); 

  for (size_t i=0; i < dSetIter.size(); i++){
    if ((dSetIter[i]).compare(rSetIter[i]) != 0){
      //cout << dSetIter[i] << rSetIter[i] << endl;
      *missingCst += "  if( " + dSetIter[i] + " == " + rSetIter[i] +" )\n" ;    
      missing = true;
    }
  }

  //cout << "\n here " << *missingCst << endl;
  return missing;
}

vector<string> DebugComputer::DumpMissingIterators(isl_set* set){
  vector<string> res;
  string set_str = string_from_isl_set(set);

  regex base_regex(".*->\\s\\{.*\\[(.*)\\]");
  smatch base_match;
  if (regex_match(set_str, base_match, base_regex)) {
  
    for(size_t i=1; i<base_match.size(); i++) {
      ssub_match base_sub_match = base_match[i];
      if (base_sub_match.str().empty())
        cout << "scalar" << endl;
      //cout << "iter: " << base_sub_match.str() << endl; 
      res.push_back(base_sub_match.str());
    }
  }else{
    cout << "Cannot find missing iter in isl_set..." << endl;
  }

  return res;
}


string string_from_isl_qpolynomial(isl_qpolynomial* poly) {
  char *str;
  __isl_give isl_printer *printer = isl_printer_to_str(isl_qpolynomial_get_ctx(poly));
  printer = isl_printer_set_output_format(printer, ISL_FORMAT_C);
  isl_printer_print_qpolynomial (printer, poly);
  str = isl_printer_get_str(printer);
  return string(str);
}

isl_stat piece_of_pw_qpolynomial(isl_set* set, isl_qpolynomial* version_qpolynomial, void *user) {
  string set_str = cst_from_isl_string(DebugComputer::string_from_isl_set(set));
  string poly_str = string_from_isl_qpolynomial(version_qpolynomial);

  if(set_str.empty() or poly_str.empty()) 
    return isl_stat_ok;
  
  //cout << "set:" << set_str << endl;
  //cout << "poly:" << poly_str << endl;

  vector<pair<string,string>>* usr = (vector<pair<string,string>> *) user;
  usr->push_back({set_str, poly_str});
  return isl_stat_ok;
}

isl_stat checker_from_pw_qpolynomial(isl_pw_qpolynomial *version_pw_qpolynomial, void *user){
  isl_pw_qpolynomial_foreach_piece(version_pw_qpolynomial, &piece_of_pw_qpolynomial, user);
  return isl_stat_ok;
}
void checker_from_union_pw_qpolynomial(isl_union_pw_qpolynomial *version_union_pw_qpolynomial, vector<pair<string,string>> **user){
  *user = new vector<pair<string,string>>();
  isl_union_pw_qpolynomial_foreach_pw_qpolynomial(version_union_pw_qpolynomial, &checker_from_pw_qpolynomial, *user);
}



void DebugComputer::ComputeDebugCode(vector<Statement*>& stmtVec, Dependences* dependences, isl_union_map* R, isl_union_map* W, isl_union_map* S){
  vector<isl_map*> *RWmaps = new vector<isl_map*>();
  isl_union_map_foreach_map(isl_union_map_copy(R), &DependenceAnalyzer::MapFromUnionMap, RWmaps);
  isl_union_map_foreach_map(isl_union_map_copy(W), &DependenceAnalyzer::MapFromUnionMap, RWmaps);
  DependenceAnalyzer::RemakeReverseMaps(RWmaps);

  map<string,vector<pair<string,string>>> version_map;
  for (size_t i=0; i<RWmaps->size(); i++) {
    isl_map* rwmap = RWmaps->at(i);
    string arrayRef = DumpArrayRef(string_from_isl_map(isl_map_copy(rwmap)));
    string arrayNum = DumpArrayNum(string_from_isl_map(isl_map_copy(rwmap)));

    isl_union_map* isl_version_map = ComputerVersion(isl_map_copy(rwmap), isl_union_map_copy(W), isl_union_map_copy(S));
    isl_union_pw_qpolynomial* isl_version_union_pw_qpolynomial = isl_union_map_card(isl_version_map);
    isl_version_union_pw_qpolynomial = isl_union_pw_qpolynomial_coalesce(isl_version_union_pw_qpolynomial);

    vector<pair<string,string>>* vec_cst_and_version;
    checker_from_union_pw_qpolynomial(isl_version_union_pw_qpolynomial, &vec_cst_and_version);

    //isl_map_dump(rwmap);
    string key = arrayNum+arrayRef;
    //cout << "key: " << key << " ";
    for (size_t j=0; j<vec_cst_and_version->size(); j++) {
      auto val = vec_cst_and_version->at(j);
      version_map[key].push_back(val);
      //cout << "val: " << val.first << " " << val.second;
    }
    //cout << endl;
  }

  // Epilogue: All write arrays
  std::set<string> write_set;
  for (size_t i=0; i < stmtVec.size(); i++) {
    isl_map* write_isl_map = stmtVec[i]->getWriteReferenceMap();
    string wNum = DumpArrayNum(string_from_isl_map(isl_map_copy(write_isl_map)));
    string wRef = DumpArrayRef(string_from_isl_map(isl_map_copy(write_isl_map)));
    write_set.insert(wNum+wRef);
    //cout << "write:" << wNum+wRef << endl;
  }

  // Step1: build relation of source/target iterators 
  for (size_t i=0; i < stmtVec.size(); i++) {
    isl_map* write = stmtVec[i]->getWriteReferenceMap();

    // get tuple name as statement name
    stmtVec[i]->setId(isl_map_get_tuple_id(isl_map_copy(write),isl_dim_in));

    vector<string> iterList;
    DumpMapIteratorsToList(isl_map_copy(write),iterList);
    vector<isl_map*> read_maps = stmtVec[i]->getReadReferenceMaps();
    for(auto read_map: read_maps) 
      DumpMapIteratorsToList(read_map, iterList);
    stmtVec[i]->setIterators(iterList);
    //for(auto s: iterList) 
      //cout << "iterList:" << s << endl;

    
    // Pre-Step3
    vector<int>* index = new vector<int>();
    size_t dimOut = isl_map_dim(isl_map_copy(write), isl_dim_out);
    size_t dimIn  = isl_map_dim(isl_map_copy(write), isl_dim_in);
    //isl_map_dump(write);

    for (size_t out=0; out < dimOut; out++) 
      for (size_t in=0; in < dimIn; in++) {
        string in_str = isl_map_get_dim_name(isl_map_copy(write), isl_dim_in, in);
        if (!iterList.empty())
          if(in_str == iterList[out])
            index->push_back(in);
      }

    stmtVec[i]->setIndex(index);
    //for (size_t r=0; r < (stmtVec[i]->getIndex())->size(); r++)
      //cout << "\n re: " << (*(stmtVec[i]->getIndex()))[r] << endl;
  }

  // Step2: extract constraints from dependences
  vector<isl_map*>* reverse_dep_maps = dependences->getReverseMaps();

  for (size_t i=0; i < stmtVec.size(); i++) {
    string* checker_code = new string("");
    vector<isl_map*> read_maps = stmtVec[i]->getReadReferenceMaps();

    for(auto read_map: read_maps) {
      string stmt_num = DumpArrayNum(string_from_isl_map(isl_map_copy(read_map)));
      string stmt_arr = DumpArrayRef(string_from_isl_map(isl_map_copy(read_map)));
      string key = stmt_num + stmt_arr;
      string stmt_array = array_from_isl_array(stmt_arr);

      if (version_map.count(key) and !stmt_array.empty()) {
        vector<pair<string,string>> key_to_cst_ver_pair = version_map[key];

        for(auto cst_ver_pair: key_to_cst_ver_pair) {
          string cst = cst_ver_pair.first;   
          string ver = cst_ver_pair.second;

          regex r_and("\\sand\\s");
          regex r_or("\\sor\\s");

          if (regex_search(cst, r_and))
            cst = regex_replace (cst,r_and," && ");

          if (regex_search(cst, r_or))
            cst = regex_replace (cst,r_or," || ");

          *checker_code += "\nif (" + cst + ")\n{\n";
          *checker_code += "version = " + ver + ";\n";
          *checker_code += "checksum += (unsigned)(version ^  (int)" + stmt_array + ");\n"; 
          *checker_code += "}\n"; 
        }
      }
    }

    isl_map* write_map = stmtVec[i]->getWriteReferenceMap();
    string stmt_num = DumpArrayNum(string_from_isl_map(isl_map_copy(write_map)));
    string stmt_arr = DumpArrayRef(string_from_isl_map(isl_map_copy(write_map)));
    string key = stmt_num + stmt_arr;
    string stmt_array = array_from_isl_array(stmt_arr);

    if (version_map.count(key) and !stmt_array.empty()) {
      vector<pair<string,string>> key_to_cst_ver_pair = version_map[key];

      for(auto cst_ver_pair: key_to_cst_ver_pair) {
        string cst = cst_ver_pair.first;   
        string ver = cst_ver_pair.second;

        regex r_and("\\sand\\s");
        regex r_or("\\sor\\s");

        if (regex_search(cst, r_and))
          cst = regex_replace (cst,r_and," && ");

        if (regex_search(cst, r_or))
          cst = regex_replace (cst,r_or," || ");

        *checker_code += "\nif (" + cst + ")\n{\n";
        *checker_code += "version = " + ver + ";\n";
        *checker_code += "checksum += (unsigned)(version ^ (int)" + stmt_array + ");\n"; 
        *checker_code += "shadow_" + stmt_array + " = version;\n";
        *checker_code += "}\n"; 
      }
    }

    if (!stmt_array.empty()) {
      *checker_code += stmt_array + "++;\n";
      *checker_code += "shadow_" + stmt_array + "++;\n";
    }
    stmtVec[i]->SetDebugCode(checker_code);

    cout << "\n -----------PolyCheck Inserted Code------------"  << endl;
    cout << "\n " << *checker_code << endl;

  }
}




