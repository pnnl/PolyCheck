#include <fstream>
#include <iostream>
#include <sstream>

#include "DebugComputer.hpp"
#include "LiveDataComputer.hpp"
#include "Orchestrator.hpp"
#include "UnusedComputer.hpp"
#include "UnusedDeclComputer.hpp"
#include <isl/union_set.h>

using namespace std;

Orchestrator::Orchestrator() {  dependences = new Dependences(); } // Orchestrator()

Orchestrator::~Orchestrator() { delete dependences; } // ~Orchestrator()

vector<Statement *> Orchestrator::GetStatements() {
  return stmtVec;
} // vector<Statement*> Orchestrator::GetStatements()

void GatherStmtVarIds(vector<string> *stmtVarIds, SgNode *sgStmt,
                      isl_set *domainSet, isl_union_map *read,
                      isl_union_map *write);

ReturnValues Orchestrator::drive(
  std::vector<std::pair<SgNode *, scoplib_scop_p>> &polyopt_scops) {
  isl_ctx *ctx = isl_ctx_alloc();
  PolyOptISLRepresentation isl_scop =
    PolyOptConvertScopToISL(polyopt_scops[0].second);

  isl_union_map *R = isl_scop.scop_reads;
  // Union of all access functions intersected with domain.
  isl_union_map *W = isl_scop.scop_writes;
  // Union of all schedules.
  isl_union_map *S = isl_scop.scop_scheds;

  ReturnValues returnValues;

  // cout << "STMT as STRING\n";
  // for (auto i : isl_scop.stmt_body)
  //   cout << (i) << endl;

  // cout << "ACC FUNC READS\n";
  // for (auto i : isl_scop.stmt_accfunc_read)
  //   isl_union_map_dump(i);

  // cout << "STMT READ DOMAIN\n";
  // for (auto i : isl_scop.stmt_read_domain)
  //   isl_union_map_dump(i);

//   cout << "STMT WRITE DOMAIN\n";
//   for (auto i : isl_scop.stmt_write_domain)
//     isl_union_map_dump(i);

  // cout << "STMT SCHEDULE\n";
  // for (auto i : isl_scop.stmt_schedule)
  //   isl_map_dump(i);

  // cout << "STMT ITER DOMAIN \n";
  // for (auto i : isl_scop.stmt_iterdom)
  //   isl_set_dump(i);

//   cout << "ACC FUNC WRITES\n";
//   cout << isl_scop.stmt_accfunc_write.size() << endl;
//   for (auto i : isl_scop.stmt_accfunc_write)
//     isl_union_map_dump(i);

  // FormStatements(ctx, isl_scop); // collect statements
  FormStatements(ctx, isl_scop);

  // dependences = DependenceAnalyzer::ComputeDependencies(stmtVec);
  DependenceAnalyzer::ComputeDependencies(stmtVec, dependences);
  //dependences->display();

  // debug code calculate
  // DebugComputer::ComputeDebugCode(stmtVec, dependences, scop_reads, scop_writes, scop_scheds);
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

} // ReturnValues Orchestrator::drive(string fileName)


// FIXME: GatherReference
void GatherReference(Statement *statement, isl_union_map *read,
                     isl_union_map *write) {
  // read references
  vector<isl_map *> *readMaps = new vector<isl_map *>();
  isl_union_map_foreach_map(isl_union_map_copy(read),
                            &DependenceAnalyzer::MapFromUnionMap, readMaps);

  for (int i = 0; i < readMaps->size(); i++) {
    statement->addToReadReferenceMaps(readMaps->at(i));
  }

  // write references
  isl_map *writeMap = isl_map_from_union_map(write);
  statement->setWriteReferenceMaps(writeMap);

  const char *writeArray = isl_map_get_tuple_name(writeMap, isl_dim_out);
  statement->setWriteArrayName(string(writeArray));
}


void Orchestrator::FormStatements(isl_ctx *ctx, PolyOptISLRepresentation &scop) {

//  cout << "debug isl union map: " << scop.stmt_write_domain.size() << endl;

  for (int i = 0; i < scop.scop_nb_statements; i++) {

    Statement *statement = new Statement(ctx);
    
    statement->setStatementNumber(i); // add to sid
    isl_set* domainSet = isl_set_coalesce(isl_set_remove_redundancies(scop.stmt_iterdom[i]));
    statement->setDomainSet(isl_set_copy(domainSet));

//statement->setScheduleMap(scop.stmt_schedule[i]);
    isl_union_map* schUMap = isl_union_map_intersect_domain(isl_union_map_copy(
       isl_union_map_from_map(scop.stmt_schedule[i])),isl_union_set_from_set(domainSet));
    statement->setScheduleMap(isl_map_from_union_map(schUMap)); 

    GatherReference(statement, scop.stmt_read_domain[i],
                    scop.stmt_write_domain[i]);

    // gather stmt variable ids
    vector<string>* stmtVarIds = statement->getStmtVarIds();
    GatherStmtVarIds(stmtVarIds, scop.stmt_body_ir[i], domainSet,
                     scop.stmt_read_domain[i], scop.stmt_write_domain[i]);
    statement->setStmtVarIds(stmtVarIds);

    if(!stmtVarIds->empty()) stmtVec.push_back(statement);

  }

} // FormStatements

// FIXME: GatherStmtVarIds
void GatherStmtVarIds(vector<string> *stmtVarIds, SgNode *sgStmt,
                      isl_set *domainSet, isl_union_map *read,
                      isl_union_map *write) {

  cout << "sgStmt = " << sgStmt->unparseToString() << endl;
  collectRefsAndOps(sgStmt, stmtVarIds);

  cout << "gather stmt var ids\n";
  for (auto r: *stmtVarIds)
    cout << r << endl;

  cout << "======================\n";

  // GatherStmtOpIds(stmtVarIds, sgStmt);

  // // write references
  // // isl_map* wmap = isl_map_intersect_domain(isl_map_copy((expr->acc).access),
  // // isl_set_copy(domainSet));
  // cout << "debug isl union map\n";
  // isl_union_map_dump(write);

  // isl_map *wmap = isl_map_from_union_map(write);

  // const char *wchar = isl_map_get_tuple_name(wmap, isl_dim_out);
  // string wId = wchar;
  // // cout << "wid: " <<  wId << endl;
  // stmtVarIds->push_back(wId);

  // // read references
  // vector<isl_map *> *rmaps = new vector<isl_map *>();
  // isl_union_map_foreach_map(isl_union_map_copy(read),
  //                           &DependenceAnalyzer::MapFromUnionMap, rmaps);

  // for (int i = 0; i < rmaps->size(); i++) {
  //   const char *rchar = isl_map_get_tuple_name(rmaps->at(i), isl_dim_out);
  //   string rId = rchar;
  //   // cout << "rid: " <<  rId << endl;
  //   stmtVarIds->push_back(rId);
  // }
}

void collectRefsAndOps(SgNode *node,
                       std::vector<string> *allRefs) {
  // 1- Traverse the tree, collect all refs.
  // Visit the sub-tree, collecting all referenced variables.
  class ReferenceVisitor : public AstPrePostOrderTraversal {
  public:
    virtual void postOrderVisit(SgNode *node) {};

    virtual void preOrderVisit(SgNode *node) {

      //cout << "node=" << node->unparseToString() << " \t --- parent=" << node->get_parent()->unparseToString() <<  endl;
      if (isSgForInitStatement(node->get_parent()->get_parent())) return;
      if (isSgVarRefExp(node) || isSgPntrArrRefExp(node) ||
          isSgDotExp(node) || isSgBinaryOp(node) || isSgIntVal(node) || isSgDoubleVal(node)) {
        bool is_binop = false;
        SgNode * curref = node;
        node = node->get_parent();

        if (isSgForInitStatement(node->get_parent()->get_parent())) return;


        // if(isSgPntrArrRefExp(curref))
        // cout << "AREF:" << SageInterface::convertRefToInitializedName(curref)->unparseToString() << endl;

        // Special case 1: Array reference, skip inner dimensions
        // of arrays.
        SgPntrArrRefExp *last;
        if ((isSgPntrArrRefExp(node)))
          //if (last->get_lhs_operand() == curref || isSgPntrArrRefExp(last->get_parent()))
          return;


        // Special case 2: Cast, skip them.
        SgCastExp *cexp = isSgCastExp(node);
        while (isSgCastExp(node))
          node = node->get_parent();

        switch (curref->variantT()) {
          // Unary ops.
          case V_SgMinusMinusOp:
          case V_SgPlusPlusOp:
          case V_SgUserDefinedUnaryOp: {
            break;
          }
            // Assign ops.
          case V_SgPlusAssignOp: {
            is_binop = true;
            _allRefs.push_back(std::string("+="));
            break;
          }
          case V_SgMinusAssignOp: {
            is_binop = true;
            _allRefs.push_back(std::string("-="));
            break;
          }
          case V_SgAndAssignOp:
          case V_SgIorAssignOp:
          case V_SgMultAssignOp: {
            is_binop = true;
            _allRefs.push_back(std::string("*="));
            break;
          }
          case V_SgDivAssignOp: {
            is_binop = true;
            _allRefs.push_back(std::string("/="));
            break;
          }
          case V_SgModAssignOp:
          case V_SgXorAssignOp:
          case V_SgLshiftAssignOp:
          case V_SgRshiftAssignOp: {
//	      SgBinaryOp* bop = isSgBinaryOp(curref);
//	      if (bop->get_lhs_operand() == curref);
            break;
          }
          case V_SgAssignOp: {
//	      SgBinaryOp* bop = isSgBinaryOp(curref);
//	      if (bop->get_lhs_operand() == curref);
            is_binop = true;
            _allRefs.push_back(std::string("="));
            break;
          }
            // Skip safe constructs.
          case V_SgPntrArrRefExp:
          case V_SgExprListExp:
          case V_SgDotExp:
          case V_SgExprStatement:
          case V_SgConditionalExp:
            break;
            // Skip binop
          case V_SgEqualityOp:
          case V_SgLessThanOp:
          case V_SgGreaterThanOp:
          case V_SgNotEqualOp:
          case V_SgLessOrEqualOp:
          case V_SgGreaterOrEqualOp:
          case V_SgAddOp: {
            is_binop = true;
            _allRefs.push_back(std::string("+"));
            break;
          }
          case V_SgSubtractOp: {
            is_binop = true;
            _allRefs.push_back(std::string("-"));
            break;
          }
          case V_SgMultiplyOp: {
            is_binop = true;
            _allRefs.push_back(std::string("*"));
            break;
          }
          case V_SgDivideOp: {
            is_binop = true;
            _allRefs.push_back(std::string("/"));
            break;
          }
          case V_SgIntegerDivideOp:
          case V_SgModOp:
          case V_SgAndOp:
          case V_SgOrOp:
          case V_SgBitXorOp:
          case V_SgBitAndOp:
          case V_SgBitOrOp:
          case V_SgCommaOpExp:
          case V_SgLshiftOp:
          case V_SgRshiftOp:
          case V_SgScopeOp:
          case V_SgExponentiationOp:
          case V_SgConcatenationOp:
            // Skip safe unops.
          case V_SgExpressionRoot:
          case V_SgMinusOp: {
            is_binop = true;
            _allRefs.push_back(std::string("-"));
            break;
          }
          case V_SgUnaryAddOp:
          case V_SgNotOp:
          case V_SgBitComplementOp:
          case V_SgCastExp:
          case V_SgThrowOp:
          case V_SgRealPartOp:
          case V_SgImagPartOp:
          case V_SgConjugateOp:
            break;
          case V_SgFunctionCallExp:
            break;
          case V_SgPointerDerefExp:
          case V_SgAddressOfOp:
          default: {
            is_binop = false;
            break;
          }
        }
        if (!is_binop && (isSgPntrArrRefExp(curref) || isSgIntVal(curref) || isSgDoubleVal(curref) || isSgVarRefExp(curref) ) )
          if (isSgPntrArrRefExp(curref))
           _allRefs.push_back(SageInterface::convertRefToInitializedName(curref)->unparseToString());
           else _allRefs.push_back(curref->unparseToString());
      }
    }

    std::vector<string> _allRefs;

  };

  ReferenceVisitor lookupReference;
  lookupReference.traverse(node);

  allRefs->insert(allRefs->end(), lookupReference._allRefs.begin(),
                  lookupReference._allRefs.end());

}
