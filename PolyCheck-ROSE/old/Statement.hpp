#ifndef STATEMENT_HPP
#define STATEMENT_HPP

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


using namespace std;

class Statement
{
public:
  Statement(isl_ctx *ctx_in);
  ~Statement();
  
  void setCtx(isl_ctx *ctx);
  isl_ctx *getCtx();

  void setDomainSet(isl_set *domainSet);
  isl_set *getDomainSet();

  void setReadReferenceMaps(vector<isl_map*> read_referenceMaps);
  vector<isl_map*> getReadReferenceMaps();
  void addToReadReferenceMaps (isl_map *read_referenceMap);

  void setWriteReferenceMaps(isl_map *write_referenceMap);
  isl_map *getWriteReferenceMap();

  void setScheduleMap(isl_map *scheduleMap);
  isl_map *getScheduleMap();
  
  // debug checker
  vector<string> getIterators();
  void setIterators(vector<string> inIterators);

  string* GetDebugCode();
  void SetDebugCode(string* inDebugCode); 

  isl_id* getId();
  void setId(isl_id* stmtId);

  string getWriteArrayName();
  void setWriteArrayName(string writeArrayName);

  int getStatementNumber();
  void setStatementNumber(int num);

  vector<int>* getIndex();
  void setIndex(vector<int>* inIndex);
  
  vector<string>* getStmtVarIds();
  void setStmtVarIds(vector<string>* inStmtVarIds);

private:
  isl_ctx *ctx;
  isl_set *domainSet;
  vector<isl_map*> read_referenceMaps;
  isl_map *write_referenceMap;
  isl_map *scheduleMap;

  // debug checker
  int stmtNum;
  isl_id* Id;
  string writeArrayName;
  vector<int>* index;
  vector<string> iterators;

  // pointer to debug code
  string* debugCode;
  // stmt variable ids to find correspond stmt in target file
  vector<string>* stmtVarIds;

};


#endif
