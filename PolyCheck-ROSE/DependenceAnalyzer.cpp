#include "DependenceAnalyzer.hpp"
#include "UnusedDeclComputer.hpp"
#include <isl/flow.h>
#include <isl/map.h>
#include <assert.h>

void DependenceAnalyzer::ComputeDependencies(vector<Statement*> &statements, Dependences* deps){
  isl_union_map *sink = NULL, *must_source = NULL, *schedule = NULL, *may_source = NULL;

  isl_union_map* tempMap = NULL;

  for (size_t i = 0; i < statements.size(); i++){
    if (statements[i] == NULL) {
	    cout << "Statement is NULL" << endl;
	    assert(0);
	  }

    if (statements[i]->getWriteReferenceMap() == NULL){
	    cout << "Write map is null" << endl;
	    assert (0);
	  }

    // write maps
    tempMap = isl_union_map_from_map(isl_map_copy(statements[i]->getWriteReferenceMap()));

	  if (must_source == NULL){
	    must_source = tempMap;
    } else {
	    must_source = isl_union_map_union(tempMap, must_source);
	  }

	  vector<isl_map*> read_refs = statements[i]->getReadReferenceMaps();
    for (size_t j = 0; j < read_refs.size(); j++) {
	    tempMap = isl_union_map_from_map(isl_map_copy(read_refs[j]));

      if (sink == NULL) {
		    sink = tempMap;
      } else {
		    sink = isl_union_map_union (tempMap, sink);
	    }
	  }

    // schedule
	  tempMap = isl_union_map_from_map(isl_map_copy(statements[i]->getScheduleMap()));
    if (schedule == NULL) {
	     schedule = tempMap;
    } else {
	     schedule = isl_union_map_union (tempMap, schedule);
	  }
  }

  // FLOW dependences
  isl_union_map *must_dep = NULL, *may_dep = NULL, *must_no_source = NULL, *may_no_source = NULL;
  may_source = isl_union_map_empty (isl_union_map_get_space(must_source));

  isl_union_map_compute_flow(sink, 
                             isl_union_map_copy(must_source), 
                             isl_union_map_copy(may_source), 
                             isl_union_map_copy(schedule), 
                             &must_dep, 
                             &may_dep, 
                             &must_no_source, 
                             &may_no_source);
  // cout << "Returned from isl_union_map_compute_flow()" << endl;

  // set writeMaps
  deps->setWriteMaps (must_source);

  //cout << "Must dependencies" << endl;
  if (must_dep != NULL) {
	  deps->setFlowDependences (must_dep);
    deps->setReverseFlowMap (isl_union_map_reverse(isl_union_map_copy(must_dep)));
  } else {
    cout << "must_dep is NULL" << endl;
  }

  //	cout << "must_no_source " << endl;
  if (must_no_source != NULL) {
    deps->setLiveIn (must_no_source);
  } else {
    cout << "must_no_source is NULL" << endl;
  }// END FLOW dependences

  // WAW dependences
  must_dep = NULL; may_dep = NULL;must_no_source = NULL; may_no_source = NULL;
  
  isl_union_map_compute_flow (isl_union_map_copy(must_source), 
                              isl_union_map_copy(must_source), 
                              isl_union_map_copy(may_source), 
                              isl_union_map_copy(schedule), 
                              &must_dep, 
                              &may_dep, 
                              &must_no_source, 
                              &may_no_source);

  if (must_dep != NULL) {
	  deps->setWAW (must_dep);
    deps->setReverseWAWMap (isl_union_map_reverse(isl_union_map_copy(must_dep)));
  } else {
    cout << "WAW is NULL" << endl;
  }
  // END WAW dependences

  // Reverse maps 
  //vector<isl_map*>* Rmaps = new vector<isl_map*>();
  vector<isl_map*>* Rmaps = deps->getReverseMaps();
  // flow 
  isl_union_map* reverseUnionMap = isl_union_map_copy(deps->getReverseFlowMap());
  isl_union_map_foreach_map(reverseUnionMap , &MapFromUnionMap, Rmaps);
  // WAW
  reverseUnionMap = isl_union_map_copy(deps->getReverseWAWMap());
  isl_union_map_foreach_map(reverseUnionMap , &MapFromUnionMap, Rmaps);
  // END Reverse maps 
  
  // Remake to remove duplication
  RemakeReverseMaps(Rmaps); 
  deps->setReverseMaps(Rmaps);
  
  // comment out to avoid segement fault
  //isl_union_map_free(reverseUnionMap);
  //isl_union_map_free(sink);
  //isl_union_map_free(schedule);
  //isl_union_map_free(may_source);
  //if (tempMap) isl_union_map_free(tempMap);
  //if (may_dep) isl_union_map_free(may_dep);
  //if (may_no_source) isl_union_map_free(may_no_source);
}

void DependenceAnalyzer::RemakeReverseMaps(vector<isl_map*>* Rmaps) {
  for (size_t i=0; i<Rmaps->size(); i++) {
    isl_map* map = Rmaps->at(i);
    for (size_t j=i+1; j<Rmaps->size(); j++)
      if(isl_map_is_equal(map, Rmaps->at(j)))
        Rmaps->erase(Rmaps->begin()+j);
  }
}

isl_stat DependenceAnalyzer::MapFromUnionMap (__isl_take isl_map* eachMap, void* user)
{
  isl_map_foreach_basic_map(isl_map_copy(eachMap), &BasicMapFromMap, user);
  isl_map_free(eachMap);
  return isl_stat_ok;
}

isl_stat DependenceAnalyzer::BasicMapFromMap(__isl_take isl_basic_map *bmap, void* user)
{
  isl_map* map = isl_map_from_basic_map(bmap);
  ((vector<isl_map*>*)(user))->push_back(isl_map_copy(map));

  isl_basic_map_free(bmap);
  isl_map_free(map);

  return isl_stat_ok;
}

isl_stat DependenceAnalyzer::SetFromUnionSet (__isl_take isl_set* eachSet, void* user)
{
  isl_set_foreach_basic_set(isl_set_copy(eachSet), &BasicSetFromSet, user);
  isl_set_free(eachSet);

  return isl_stat_ok;
}

isl_stat DependenceAnalyzer::BasicSetFromSet (__isl_take isl_basic_set *bset, void* user)
{
  // NOTE: gset to avoid the same name with isl_set
  isl_set* gset = isl_set_from_basic_set(bset);
  ((set<isl_set*>*)(user))->insert(isl_set_copy(gset));

  isl_basic_set_free(bset);
  isl_set_free(gset);

  return isl_stat_ok;
}

