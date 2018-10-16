#ifndef DEPENDENCES_HPP
#define DEPENDENCES_HPP

#include <vector>
#include <set>

#include <isl/ast.h>
#include <isl/ast_build.h>
#include <isl/schedule.h>
#include <isl/map.h>

using namespace std;

class Dependences
{
public:
  Dependences();
  ~Dependences();

  isl_union_map* getFlowDependences();
  isl_union_map* getLiveIn();
  isl_union_map* getWriteMaps();
  isl_union_map* getWAW();
  isl_union_map* getReverseFlowMap();
  isl_union_map* getReverseWAWMap();

  void setFlowDependences(isl_union_map*);
  void setLiveIn(isl_union_map*);
  void setWriteMaps(isl_union_map*);
  void setWAW(isl_union_map*);

  void setReverseFlowMap(isl_union_map*);
  void setReverseWAWMap(isl_union_map*);
  void display();

  vector<isl_map*>* getReverseMaps();
  void setReverseMaps(vector<isl_map*>* maps);

private:
  isl_union_map *flow_dep;
  isl_union_map *live_in;
  isl_union_map *writeMaps;
  isl_union_map *WAW;

  isl_union_map *reverseFlowMap;
  isl_union_map *reverseWAWMap;

  vector<isl_map*>* reverseMaps;
  
};

#endif
