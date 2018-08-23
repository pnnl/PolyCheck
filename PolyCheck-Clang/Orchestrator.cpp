#include <iostream>
#include <fstream>
#include <sstream>

#include <Orchestrator.hpp>
#include <DebugComputer.hpp>
#include <LiveDataComputer.hpp>
#include <UnusedComputer.hpp>
#include <UnusedDeclComputer.hpp>
#include "expr.h"
#include <isl/union_set.h>

using namespace std;

Orchestrator::Orchestrator(){
  dependences = new Dependences();
}

Orchestrator::~Orchestrator(){
  delete dependences;
}

vector<Statement*> Orchestrator::GetStatements(){
  return stmtVec;
}

ReturnValues Orchestrator::drive(string fileName){
  ReturnValues returnValues;
    
  struct pet_options *options = pet_options_new_with_defaults();
  isl_ctx* ctx = isl_ctx_alloc_with_options(&pet_options_args, options); 
  struct pet_scop* scop = pet_scop_extract_from_C_source(ctx, fileName.c_str(), NULL);

  isl_union_map *R, *W, *S;
  R = pet_scop_get_may_reads(scop);
  W = pet_scop_get_may_writes(scop);
  S = isl_schedule_get_map(pet_scop_get_schedule(scop));


  FormStatements(ctx, scop, R, W, S);

  DependenceAnalyzer::ComputeDependencies(stmtVec, dependences);
  //dependences->display();

   
  // debug code calculate
  DebugComputer::ComputeDebugCode(stmtVec, dependences, R, W, S);

   ////live-in data checker
  LiveDataComputer::ComputeLiveInDataCheckerCode(stmtVec, dependences, &returnValues.LiveDataCheckerCode);

  //// Epilogue declarations
  UnusedDeclComputer::ComputeUnusedDeclCheckerCode(R, W, &returnValues.EpilogueDeclCode);

  //// Epilogue checker
  ////FIXME
  UnusedComputer::ComputeUnusedDataCheckerCode(R, W, stmtVec, dependences, &returnValues.UnusedDataCheckerCode);

  returnValues.stmtVecReturn = GetStatements();    

  // clean memory
  isl_union_map_free(R);
  isl_union_map_free(W);
  isl_union_map_free(S);
  //isl_ctx_free(ctx);
  return returnValues;
}

void GatherReference(Statement* statement, struct pet_expr* expr)
{
  //pet_expr_dump(expr);

  if (pet_expr_get_type(expr) == pet_expr_access){
    if (pet_expr_access_is_read(expr)){
      isl_map* may_read = isl_map_from_union_map(pet_expr_access_get_may_read(expr));
      isl_map* readMap = isl_map_intersect_domain(may_read,isl_set_copy(statement->getDomainSet()));
      statement->addToReadReferenceMaps(readMap);
    }

    if (pet_expr_access_is_write(expr)){
      isl_map* may_write = isl_map_from_union_map(pet_expr_access_get_may_write(expr));
      isl_map* writeMap = isl_map_intersect_domain(may_write,isl_set_copy(statement->getDomainSet()));
      statement->setWriteReferenceMaps(writeMap);
      
      const char* writeArray = isl_map_get_tuple_name(writeMap, isl_dim_out);
      statement->setWriteArrayName(string(writeArray));
    }
  }

  for(int i=0; i<pet_expr_get_n_arg(expr); i++) {
    GatherReference(statement, pet_expr_get_arg(expr,i));
  }
}

// Collect operations first if necessary
	
	

void GatherStmtOpIds(vector<string>* stmtOpIds, struct pet_expr* expr){
  if(pet_expr_op_get_type(expr) == pet_op_add_assign)
    stmtOpIds->push_back(string("+="));
  else if(pet_expr_op_get_type(expr) == pet_op_sub_assign)
    stmtOpIds->push_back(string("-="));
  else if(pet_expr_op_get_type(expr) == pet_op_mul_assign)
    stmtOpIds->push_back(string("*="));
  else if(pet_expr_op_get_type(expr) == pet_op_div_assign)
    stmtOpIds->push_back(string("/="));
  else if(pet_expr_op_get_type(expr) == pet_op_and_assign)
    stmtOpIds->push_back(string("&="));
  else if(pet_expr_op_get_type(expr) == pet_op_xor_assign)
    stmtOpIds->push_back(string("^="));
  else if(pet_expr_op_get_type(expr) == pet_op_or_assign)
    stmtOpIds->push_back(string("|="));

  else if(pet_expr_op_get_type(expr) == pet_op_assign)
    stmtOpIds->push_back(string("="));
  else if(pet_expr_op_get_type(expr) == pet_op_add)
    stmtOpIds->push_back(string("+"));
  else if(pet_expr_op_get_type(expr) == pet_op_sub)
    stmtOpIds->push_back(string("-"));
  else if(pet_expr_op_get_type(expr) == pet_op_mul)
    stmtOpIds->push_back(string("*"));
  else if(pet_expr_op_get_type(expr) == pet_op_div)
    stmtOpIds->push_back(string("/"));
  else if(pet_expr_op_get_type(expr) == pet_op_mod)
    stmtOpIds->push_back(string("%"));
  else if(pet_expr_op_get_type(expr) == pet_op_eq)
    stmtOpIds->push_back(string("=="));
  else if(pet_expr_op_get_type(expr) == pet_op_ne)
    stmtOpIds->push_back(string("!="));
  else if(pet_expr_op_get_type(expr) == pet_op_address_of)
    stmtOpIds->push_back(string("&"));

  else if(pet_expr_op_get_type(expr) == pet_op_and 
      or pet_expr_op_get_type(expr) == pet_op_land)
    stmtOpIds->push_back(string("&"));
  else if(pet_expr_op_get_type(expr) == pet_op_xor)
    stmtOpIds->push_back(string("&"));
  else if(pet_expr_op_get_type(expr) == pet_op_or
      or pet_expr_op_get_type(expr) == pet_op_lor)
    stmtOpIds->push_back(string("&"));
  else if(pet_expr_op_get_type(expr) == pet_op_not
      or pet_expr_op_get_type(expr) == pet_op_lnot)
    stmtOpIds->push_back(string("&"));

  else if(pet_expr_op_get_type(expr) == pet_op_shl)
    stmtOpIds->push_back(string("<<"));
  else if(pet_expr_op_get_type(expr) == pet_op_shr)
    stmtOpIds->push_back(string(">>"));
  else if(pet_expr_op_get_type(expr) == pet_op_le)
    stmtOpIds->push_back(string("<="));
  else if(pet_expr_op_get_type(expr) == pet_op_ge)
    stmtOpIds->push_back(string(">="));

  else if(pet_expr_op_get_type(expr) == pet_op_lt)
    stmtOpIds->push_back(string("<"));
  else if(pet_expr_op_get_type(expr) == pet_op_gt)
    stmtOpIds->push_back(string(">"));

  else if(pet_expr_op_get_type(expr) == pet_op_minus)
    stmtOpIds->push_back(string("-"));
  else if(pet_expr_op_get_type(expr) == pet_op_post_inc)
    stmtOpIds->push_back(string("++"));
  else if(pet_expr_op_get_type(expr) == pet_op_post_dec)
    stmtOpIds->push_back(string("--"));
  else if(pet_expr_op_get_type(expr) == pet_op_pre_inc)
    stmtOpIds->push_back(string("++"));
  else if(pet_expr_op_get_type(expr) == pet_op_pre_dec)
    stmtOpIds->push_back(string("--"));
  else 
    cout << "Operation Id not necessary" << endl;
}


void GatherStmtVarIds(vector<string>* stmtVarIds, struct pet_expr* expr, isl_set* domainSet){
  if (pet_expr_get_type(expr) == pet_expr_op){
    if (pet_expr_get_type(expr) == pet_op_assume
        or pet_expr_get_type(expr) == pet_op_kill 
        or pet_expr_get_type(expr) == pet_op_assume 
        or pet_expr_get_type(expr) == pet_op_cond
        or pet_expr_get_type(expr) ==  pet_op_last
        ) return;
    GatherStmtOpIds(stmtVarIds, expr);
  }

  if (pet_expr_get_type(expr) == pet_expr_access){
    if(pet_expr_access_is_write(expr)){
      isl_map* may_write = isl_map_from_union_map(pet_expr_access_get_may_write(expr));
      isl_map* wmap = isl_map_intersect_domain(may_write,isl_set_copy(domainSet));
      
      const char* wchar = isl_map_get_tuple_name(wmap, isl_dim_out);
      if (wchar) stmtVarIds->push_back(string(wchar));
    }

    if(pet_expr_access_is_read(expr)){
      isl_map* may_read = isl_map_from_union_map(pet_expr_access_get_may_read(expr));
      isl_map* rmap = isl_map_intersect_domain(may_read,isl_set_copy(domainSet));

      const char* rchar = isl_map_get_tuple_name(rmap, isl_dim_out);
      if(rchar) stmtVarIds->push_back(rchar);
    }
  }else if (pet_expr_get_type(expr) == pet_expr_int){
    string int_str = to_string(isl_val_sgn(pet_expr_int_get_val(expr))); 
    //cout << "integer: " << int_str << endl;
    stmtVarIds->push_back(int_str);
  }else if (pet_expr_get_type(expr) == pet_expr_double){
    const char* double_char = pet_expr_double_get_str(expr); 
    //cout << "double:" << double_char << endl;
    if(double_char) stmtVarIds->push_back(double_char);
  }

  for(int i=0; i<pet_expr_get_n_arg(expr); i++) {
    GatherStmtVarIds(stmtVarIds, pet_expr_get_arg(expr,i), domainSet);
  }
}

void Orchestrator::FormStatements(
    isl_ctx* ctx, 
    struct pet_scop* scop, 
    isl_union_map* R, 
    isl_union_map* W,
    isl_union_map* S) 
{

  for (int i=0; i < scop->n_stmt; i++)
  {
    struct pet_stmt* petStmt = scop->stmts[i];
    Statement* statement = new Statement(ctx);
    
    statement->setStatementNumber(i); 
    isl_set* domainSet = isl_set_coalesce(isl_set_remove_redundancies(petStmt->domain));
    statement->setDomainSet(isl_set_copy(domainSet));

    isl_union_map* schUMap = isl_union_map_intersect_domain(isl_union_map_copy(S),isl_union_set_from_set(domainSet));
    statement->setScheduleMap(isl_map_from_union_map(schUMap)); 

    struct pet_expr* body = pet_tree_expr_get_expr(petStmt->body);
    //cout << "Statement : " << i << endl;
    GatherReference(statement, body);
    
    
    // gather stmt variable ids 
    vector<string>* stmtVarIds = statement->getStmtVarIds();
    GatherStmtVarIds(stmtVarIds, body, domainSet);
    statement->setStmtVarIds(stmtVarIds);

    if(!stmtVarIds->empty()) stmtVec.push_back(statement);

    cout<<"=======gather stmt var ids==========="<<endl;
    for (size_t i=0; i < stmtVarIds->size(); i++)
      cout << "content: " << stmtVarIds->at(i) << endl;
    cout<<"=================="<<endl;
  }
}

