#include <barvinok/isl.h>
#include <Dependences.hpp>
#include <iostream>

using namespace std;

Dependences::Dependences(){
   flow_dep = nullptr;
   live_in = nullptr;
   writeMaps = nullptr;
   WAW = nullptr;

   reverseFlowMap=nullptr;
   reverseWAWMap=nullptr;
   reverseMaps = new vector<isl_map*>();
}

Dependences::~Dependences(){
  if(flow_dep) isl_union_map_free(flow_dep);
  if(live_in) isl_union_map_free(live_in);
  if(writeMaps) isl_union_map_free(writeMaps);
  if(WAW) isl_union_map_free(WAW);

  if(reverseFlowMap) isl_union_map_free(reverseFlowMap);
  if(reverseWAWMap) isl_union_map_free(reverseWAWMap);

  //if (reverseMaps) {
    //for(size_t i=0;i<reverseMaps->size();i++) {
      //if (reverseMaps->at(i)) isl_map_free(reverseMaps->at(i));
    //}
  //}

  delete reverseMaps;
}

isl_union_map* Dependences::getReverseFlowMap()
{
  return reverseFlowMap;
}

isl_union_map* Dependences::getReverseWAWMap()
{
  return reverseWAWMap;
}

isl_union_map* Dependences::getFlowDependences()
{
  return flow_dep;
}

isl_union_map* Dependences::getLiveIn()
{
  return live_in;
}

isl_union_map* Dependences::getWriteMaps()
{
  return writeMaps;
}

isl_union_map* Dependences::getWAW()
{
   return WAW;
}

void Dependences::setReverseFlowMap(isl_union_map* reverseFlow)
{
  reverseFlowMap = reverseFlow;
}

void Dependences::setReverseWAWMap(isl_union_map* reverseWAW)
{
  reverseWAWMap = reverseWAW;
}

void Dependences::setFlowDependences(isl_union_map* flow)
{
  flow_dep = flow;
}

void Dependences::setLiveIn(isl_union_map* li)
{
  live_in = li;
}

void Dependences::setWriteMaps(isl_union_map* wm)
{
  writeMaps = wm;
}

void Dependences::setWAW(isl_union_map* inWAW)
{
   WAW = inWAW;
}

vector<isl_map*>* Dependences::getReverseMaps()
{
  return reverseMaps;
}

void Dependences::setReverseMaps(vector<isl_map*>* maps)
{
  reverseMaps = maps;
}
 
void Dependences::display()
{
    cout << "vector isl_map :" << endl;
    for(size_t i=0;i<reverseMaps->size();i++) {
      isl_map_dump(reverseMaps->at(i));
    }

    cout << "Flow Dependences: " << endl;
    isl_union_map_dump(flow_dep);
    cout << "Reverse Flow Dependences: " << endl;
    //isl_union_map_dump(isl_union_map_reverse(isl_union_map_copy(flow_dep)));
    isl_union_map_dump(reverseFlowMap);
    cout << endl;
    
    cout << "Output dependences: " << endl;
    isl_union_map_dump(WAW);
    cout << "Reverse Output dependences: " << endl;
    isl_union_map_dump(reverseWAWMap);
    cout << endl;
    
    cout << "Live-in maps: " << endl;
    isl_union_map_dump(live_in);
    cout << endl;
}
