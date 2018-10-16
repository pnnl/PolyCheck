#ifndef ORCHESTRATOR_HPP
#define ORCHESTRATOR_HPP

#include <string>
#include <vector>
#include <map>

#include <pet.h>
#include <Statement.hpp>
#include <DependenceAnalyzer.hpp>
using namespace std;

struct ReturnValues
{
  vector<string*> *LiveDataCheckerCode;
  vector<string*> *UnusedDataCheckerCode;
  vector<Statement*> stmtVecReturn;
  // Epilogue Declares
  vector<string*> *EpilogueDeclCode;
};

class Orchestrator
{
public:
  Orchestrator();
  ~Orchestrator();

  ReturnValues drive(string fileName);
  vector<Statement*> GetStatements();
  void FormStatements (
      isl_ctx* ctx, 
      struct pet_scop* scop, 
      isl_union_map* R, 
      isl_union_map* W, 
      isl_union_map* S);

private:
  vector<Statement*> stmtVec;
  Dependences* dependences;
};
  
#endif
