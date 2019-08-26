#include "GeneralUtilityClass.hpp"
#include <sstream>
#include <assert.h>
#include <string>
#include <map>
#include <iostream>
#include <fstream>

using namespace std;

void GeneralUtilityClass::DataSetPrintUnionSetCode (isl_union_set* set, 
                                                    int remake, 
                                                    isl_union_map *reverseLiveInMap, 
                                                    vector<string*> *LiveDataCheckerCode, 
                                                    map<string, vector<string*> > *astNoteMap) {
  if (remake){
    char *setStr;
    isl_printer *printer = isl_printer_to_str(isl_union_set_get_ctx(set));
    isl_printer_print_union_set (printer, set);
    setStr = isl_printer_get_str(printer);

    set = isl_union_set_read_from_str(isl_union_set_get_ctx(set), setStr);
  }

  LiveDataInput* liveDataInput = new LiveDataInput;
  liveDataInput->reverseLiveInMap = reverseLiveInMap;
  liveDataInput->LiveDataCheckerCode = LiveDataCheckerCode;
  liveDataInput->astNoteMap = astNoteMap;

  isl_union_set_foreach_set(set, &LiveSetApplyReverseLiveMapOnLiveInSet, liveDataInput);
}

void GeneralUtilityClass::CollectIteratorNames(vector<string*>  &iteratorNames, isl_set *domainSet){
  for (size_t j = 0; j < isl_set_n_dim(domainSet); j++){
    const char* iteratorName = isl_set_get_dim_name(isl_set_copy(domainSet), isl_dim_set, j);
    
    if (iteratorName) 
      iteratorNames.push_back(new string(iteratorName));
    else 
      iteratorNames.push_back(new string("c"+to_string(j)));
  }
}

isl_id_list *GeneralUtilityClass::Generate_ISL_ID_List(isl_ctx *ctx, vector<string*>  &iteratorNames){
  isl_id_list *names = isl_id_list_alloc(ctx, iteratorNames.size());

  for (size_t i = 0; i < iteratorNames.size(); ++i) {
    isl_id *id = isl_id_alloc(ctx, (iteratorNames.at(i))->c_str(), nullptr);
    names = isl_id_list_add(names, id);
  }
  return names;
}


isl_stat GeneralUtilityClass::LiveSetApplyReverseLiveMapOnLiveInSet (isl_set *set, void *user){    
  LiveDataInput* liveDataInput = (LiveDataInput*) user;          
  isl_union_map* reverseLiveInMap = liveDataInput->reverseLiveInMap;
  vector<string*> *LiveDataCheckerCode = liveDataInput->LiveDataCheckerCode;
  map<string, vector<string*> > *astNoteMap = liveDataInput->astNoteMap;

  // Collect iterator names
  vector<string*> iteratorNames;
  GeneralUtilityClass::CollectIteratorNames(iteratorNames, isl_set_coalesce(isl_set_copy(set)));
  // Get the corresponding ISL_IDs
  isl_id_list* iteratorNameIDs = GeneralUtilityClass::Generate_ISL_ID_List(isl_set_get_ctx(set), iteratorNames);

  isl_union_map *schedule = isl_union_set_identity(isl_union_set_from_set(set));
  schedule = isl_union_map_coalesce(schedule);
  isl_set *context = isl_set_universe(isl_union_map_get_space(schedule));
  isl_ast_build *build = isl_ast_build_from_context(context);

  // Set iterator names
  build = isl_ast_build_set_iterators(build, iteratorNameIDs);

  // CreateCustomNode
  build = isl_ast_build_set_at_each_domain(build, &CreateCustomNode, liveDataInput);
  isl_ast_node *node = isl_ast_build_ast_from_schedule (build, schedule);

  __isl_give isl_printer *printer =  isl_printer_to_str(isl_set_get_ctx(set));  
  printer = isl_printer_set_output_format(printer, ISL_FORMAT_C);   

  // Need to set user-defined printer callback
  isl_ast_print_options *options = isl_ast_print_options_alloc(isl_set_get_ctx(set));
  options = isl_ast_print_options_set_print_user(options, &PrintingFunction, NULL);

  // isl_printer_print_ast_node (printer, node);
  isl_ast_node_print(node, printer, options);

  //char *output = isl_printer_get_str(printer);
  //cout << "Output: " << endl << output;

  LiveDataCheckerCode->push_back(new string(isl_printer_get_str(printer)));

  return isl_stat_ok;
}

isl_ast_node* GeneralUtilityClass::CreateCustomNode (isl_ast_node *leaf_node, isl_ast_build* build, void* user){
  isl_ctx* ctx = isl_ast_build_get_ctx(build);
  isl_union_map *unionMap = isl_ast_build_get_schedule(build);
  isl_union_set *domain = isl_union_map_domain(isl_union_map_copy(unionMap));

  LiveDataInput* liveDataInput = (LiveDataInput*) user;          
  isl_union_map* reverseLiveInMap = liveDataInput->reverseLiveInMap;
  map<string,vector<string*> > *astNoteMap = liveDataInput->astNoteMap;

  isl_set *set = isl_set_from_union_set(isl_union_set_copy(domain));
  set = isl_set_coalesce(isl_set_remove_redundancies(set));

  vector<string*> iteratorNames;
  GeneralUtilityClass::CollectIteratorNames(iteratorNames, set);

  string arrayName = GeneralUtilityClass::GetSetName(set);
  string ArrayReference = GeneralUtilityClass::ConstructArrayReference(isl_set_copy(set));
  string scalar = "";

  string *messageStr = new string ("\t{\n");
  if (arrayName.length() > 0)
  {         
    if (reverseLiveInMap != NULL && astNoteMap == NULL)
    {
      // BUG: ArrayReference could have scalar variable
      // FIX: add extra check to avoid these variables 
      if (ArrayReference.find("[")!=string::npos)
        *messageStr += "\t\t" + ArrayReference + " = 0;";        
      else
        *messageStr += "\t\t" + scalar + "// Scalar variables";        
    }// if (reverseLiveInMap != NULL)
    else if (reverseLiveInMap == NULL && astNoteMap != NULL)
    {
      vector<string*> astNoteVec = astNoteMap->find(arrayName)->second;
      for (size_t tmp = 0; tmp < astNoteVec.size(); tmp++)
      {
        *messageStr += *(astNoteVec[tmp]);
      }
    }// else if
    else
    {
      cout << "SHOULD NOT GET HERE!" << endl;
    }

  }

  *messageStr += "\n\t}\n";

  isl_id *id = isl_id_alloc(ctx, NULL, messageStr);

  return  isl_ast_node_set_annotation (leaf_node, id);  
}

string GeneralUtilityClass::GetSetName(isl_set *set){
  string setName;
  if (isl_set_has_tuple_id(set)) {
    isl_id *id = isl_set_get_tuple_id(set);
    const char *name = isl_id_get_name(id);
    setName = name;
  } else {
    cout << "WARNING: Set has no name." << endl;
  }
  return setName;
}

string GeneralUtilityClass::ConstructArrayReference(isl_set* set){
    // Get array name
    string arrayName = GeneralUtilityClass::GetSetName(set);
    //cout << "Array name: " << arrayName << endl;
    vector<string*> iteratorNames;
    GeneralUtilityClass::CollectIteratorNames(iteratorNames, set);

    // Construct array reference. E.g., A[i0][i1]
    string Arrayreference = arrayName;
    for (size_t i = 0; i < iteratorNames.size(); i++)
      Arrayreference += "[" + *(iteratorNames.at(i)) + "]";

    return Arrayreference;
}

isl_printer* GeneralUtilityClass::PrintingFunction (isl_printer *p, isl_ast_print_options *options, isl_ast_node *node, void *user){
  isl_id *id = isl_ast_node_get_annotation(node);
  void *usr = isl_id_get_user (id);
  string *userStr = (string*) usr;

  return isl_printer_print_str (p, (*userStr).c_str());
}

/* Extra Functions */
isl_union_set *GeneralUtilityClass::ApplyUnionMapOnSet(isl_set *set, isl_union_map* map, int remake){
  if (remake == 0){
    return isl_union_set_apply (isl_union_set_from_set (isl_set_copy(set)), isl_union_map_copy(map));
  } else {
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
}

isl_union_set *GeneralUtilityClass::ApplyUnionMapOnUnionSet(isl_union_set *set, isl_union_map* map, int remake)
{
  if (remake == 0) {
    return isl_union_set_apply (isl_union_set_copy(set), isl_union_map_copy(map));
  } else {
    char *setStr, *mapStr;
    {
      __isl_give isl_printer *printer = isl_printer_to_str(isl_union_set_get_ctx(set));
      isl_printer_print_union_set (printer, set);
      setStr = isl_printer_get_str(printer);
      // cout << "set string: " << setStr << endl;
    }
    {
      __isl_give isl_printer *printer = isl_printer_to_str(isl_union_set_get_ctx(set));
      isl_printer_print_union_map (printer, map);
      mapStr = isl_printer_get_str(printer);
      // cout << "map string: " << mapStr << endl;
    }

    isl_union_set *newSet = isl_union_set_read_from_str(isl_union_set_get_ctx(set), setStr);
    isl_union_map* newMap = isl_union_map_read_from_str(isl_union_set_get_ctx(set), mapStr);
    return isl_union_set_apply (isl_union_set_copy(newSet), isl_union_map_copy(newMap));
  }
}

isl_union_set *GeneralUtilityClass::ApplyMapOnUnionSet(isl_union_set *set, isl_map* map){
  return isl_union_set_apply (isl_union_set_copy (set), isl_union_map_from_map (isl_map_copy(map)));
}

isl_stat GeneralUtilityClass::PrintPw_qpolynomialCode(isl_pw_qpolynomial *poly, void *user){

  isl_printer *printer = isl_printer_to_str(isl_pw_qpolynomial_get_ctx(poly));
  printer = isl_printer_set_output_format(printer, ISL_FORMAT_C);
  isl_printer_print_pw_qpolynomial (printer, poly);
  char *output = isl_printer_get_str(printer);
  cout << "pw_qpolynomial: " << output << endl;

  vector<string*> *use_counts = (vector<string*> *) user;
  use_counts->push_back(new string(output));

  return isl_stat_ok;
}


void GeneralUtilityClass::PrintPw_qpolynomialCode(isl_pw_qpolynomial *poly){
  PrintPw_qpolynomialCode(poly, nullptr);
}

void GeneralUtilityClass::PrintUnion_pw_qpolynomialCode(isl_union_pw_qpolynomial *poly, vector<string*> **use_counts){
  *use_counts = new vector<string*>();
  isl_union_pw_qpolynomial_foreach_pw_qpolynomial(poly, &PrintPw_qpolynomialCode, *use_counts);
}

void GeneralUtilityClass::PrintSetCode (isl_set* set) {
  isl_printer *printer = isl_printer_to_file(isl_set_get_ctx(set), stdout);
  isl_union_set *unionSet = isl_union_set_from_set(set);
  isl_union_map *schedule = isl_union_set_identity(unionSet);
  isl_set *context = isl_set_universe(isl_union_map_get_space(schedule));
  isl_ast_build *build = isl_ast_build_from_context(context);
  isl_ast_node *node = isl_ast_build_ast_from_schedule (build, schedule);
  printer = isl_printer_set_output_format(printer, ISL_FORMAT_C);
  isl_printer_print_ast_node (printer, node);
}

void GeneralUtilityClass::PrintUnionSetCode(isl_union_set* set, int remake){
  if (remake == 1){
    char *setStr, *mapStr;
    isl_printer *printer = isl_printer_to_str(isl_union_set_get_ctx(set));
    isl_printer_print_union_set (printer, set);
    setStr = isl_printer_get_str(printer);

    set = isl_union_set_read_from_str(isl_union_set_get_ctx(set), setStr);
  }else{
    set = isl_union_set_copy(set);
  }

  __isl_give isl_printer *printer = isl_printer_to_file(isl_union_set_get_ctx(set), stdout);
  isl_union_map *schedule = isl_union_set_identity(set);
  isl_set *context = isl_set_universe(isl_union_map_get_space(schedule));
  isl_ast_build *build = isl_ast_build_from_context(context);
  isl_ast_node *node = isl_ast_build_ast_from_schedule (build, schedule);
  printer = isl_printer_set_output_format(printer, ISL_FORMAT_C);
  isl_printer_print_ast_node (printer, node);
}


