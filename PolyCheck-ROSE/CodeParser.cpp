#include <cstdio>
#include <string>
#include <sstream>
#include <iostream>
#include <fstream>

#include <map>
#include <set>
#include <regex>

#include "Statement.hpp"
#include "CodeParser.hpp"
#include "AstMatching.h"

using namespace std;

AstMatching matcher;

/// Process statements in scop returned by AstMatcher and filter loop initialization statements
void collect_scop_statements(
  int scop_number, MatchResult &assign_matches,
  std::map<int, std::vector<SgNode *>> &collectScopStatements) {
  for (MatchResult::iterator i = assign_matches.begin();
       i != assign_matches.end(); i++) {
    for (auto p : (*i)) {
      SgNode * anode = p.second;
      if (!isSgForInitStatement(anode->get_parent()->get_parent()))
        collectScopStatements[scop_number].push_back(anode);
    }
  }
}

/// check if a statement (var ids and ops) matches b/w original and transformed scops
bool CheckStmtVarIds(vector<string> *stmtVarIds, vector<string> *statementsVarIds) {
  if (stmtVarIds->size() != statementsVarIds->size()) {
    return false;
  }// if
  else {
    for (int i = 0; i < stmtVarIds->size(); i++) {

      if (((*stmtVarIds)[i]).compare((*statementsVarIds)[i]) != 0)// equal
        return false;
    }// end for
    return true;
  }//else
}//bool CheckStmtVarIds(vector<string> &stmtVarIds, vector<string>* statementsVarIds)

bool isNum(const std::string &num) {
    return std::regex_match(num, std::regex("[+-]?[0-9]+"));
}

  void ComputeCheckerIters(vector<string> &stmtVecIters, 
                           vector<string> statementsIters, 
                           set<string>    &checkerIters)
  {
    if (stmtVecIters.size() == statementsIters.size())
    {
      for (int i=0; i < stmtVecIters.size(); i++)
      {
        if ( !isNum(statementsIters[i]) )// if not number move on 
        {
          string* iterRet = new string("");
          //cout << ">>" << statementsIters[i] <<">>\n";
          // if contains "+"
          if (statementsIters[i].length()>1 && (statementsIters[i]).find("+") > 0 && ((statementsIters[i]).find("+"))<(statementsIters[i]).length())
          {
            int pos = (statementsIters[i]).find("+");
            *iterRet += (statementsIters[i]).substr(pos+1)+"="+stmtVecIters[i]+" - ("+(statementsIters[i]).substr(0,pos)+");\n";
          }// if
          else
          {
            *iterRet += statementsIters[i] + "=" + stmtVecIters[i] + ";\n";
          }//else

          checkerIters.insert(*iterRet);

        }//if ( !isNum(statementsIters[i]) )
        // do nothing if numbers 

      }//for (int i=0; i < stmtVecIters.size(); i++)
    }//if (stmtVecIters.size() == statementsIters.size())
    else
      cout << "ERROR: iterator size not equal " << endl;
}

void collectRefsAndOpsSep(SgNode *node,
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


void CodeParser::visit(SgNode *node) {

  SgNode *scop_stmt = node;
  if (mark_scop && (isSgAssignOp(scop_stmt) || isSgPlusAssignOp(scop_stmt) || isSgMultAssignOp(scop_stmt) ||
      isSgMinusAssignOp(scop_stmt) || isSgDivAssignOp(scop_stmt) || isSgAndAssignOp(scop_stmt)
      || isSgXorAssignOp(scop_stmt) || isSgIntegerDivideAssignOp(scop_stmt) ||
      isSgIorAssignOp(scop_stmt) || isSgLshiftAssignOp(scop_stmt) || isSgModAssignOp(scop_stmt)
      || isSgRshiftAssignOp(scop_stmt) || isSgExponentiationAssignOp(scop_stmt)) ) {

    vector<string> *stmtVarIds = new vector<string>();
    vector<string> stmtVecIters;        // collect stmt index
          
    SgNode * scop_stmt_copy = scop_stmt;
    collectRefsAndOpsSep(scop_stmt_copy, stmtVarIds);

    // cout << "Transformed source ---- gather stmt var ids\n";
    // cout << scop_stmt->unparseToCompleteString() << endl;
    // for (auto vid_op: *stmtVarIds)
    //   cout << vid_op << endl;
    // cout << "-------------------\n";

    string scop_stmt_replace_code = "\n//<checkers>\n";
    for (int stmt_iter = 0; stmt_iter < statements.size(); stmt_iter++) {

      vector<string> *statementsVarIds = statements[stmt_iter]->getStmtVarIds();
      bool equal = CheckStmtVarIds(stmtVarIds, statementsVarIds);

      if (equal) {
              set<string> checkerIters; 
              set<string>::iterator it;
              ComputeCheckerIters(stmtVecIters, statements[stmt_iter]->getIterators(), checkerIters);
              if (checkerIters.size() > 0)
              {
                for (it=checkerIters.begin(); it!=checkerIters.end(); it++)
                {
                  scop_stmt_replace_code += *it;
                }
              }

        string *checker_code = statements[stmt_iter]->GetDebugCode();
        if (checker_code != NULL) {
          cout << "checker code \n----------\n" << scop_stmt->unparseToCompleteString() << "\n------------\n" << *checker_code << endl;
          scop_stmt_replace_code += scop_stmt->unparseToCompleteString() + (*checker_code) + "\n";
        }

      }//if (equal)
    }//for (int i=0; i < statements.size(); i++)
    scop_stmt_replace_code +="\n//<checkers>\n";
    SgNodeHelper::replaceAstWithString(scop_stmt, scop_stmt_replace_code);
    return;
  } //check if assignment

  else if (SgPragmaDeclaration * pragma = isSgPragmaDeclaration(node)) {
    const string pragmaName = pragma->get_pragma()->get_pragma();
    // cout << pragmaName << endl;
    if ((pragmaName.size() == 7) && (pragmaName.compare(0, 7, "endscop") == 0)) {
      /// Mark end of scop
      mark_scop = false;

      /// Insert epilogue code just before the end of current scop
      string ecode = "// Epilogue\n";
      if (Epilogue != NULL) {
        for (int i = 0; i < Epilogue->size(); i++) {
          ecode += *(Epilogue->at(i)) + "\n";
        }
      }
      ecode += "\n assert(checksum == 0);\n";
      SgNodeHelper::replaceAstWithString(node, "\n" + ecode + node->unparseToCompleteString());
      return;
    } else if ((pragmaName.size() == 4) && (pragmaName.compare(0, 4, "scop") == 0)) {
      /// Mark start of scop
      mark_scop = true;
      //FIXME: Epilogue declarations are inserted before the surrounding scope of the scop
      // in original polycheck code - We are inserting them just before the start of scop
      string ecode = "// Epilogue Declaration Code\n";
      for (int i = 0; i < EpilogueDecls->size(); i++)
        ecode += *(EpilogueDecls->at(i)) + "\n";
      ecode += "\n int checksum;\n";
      ecode += "\n int version;\n";
      //SgNodeHelper::replaceAstWithString(node, "\n" + ecode + node->unparseToString());

      /// Insert prologue code right after start of current scop
      string pcode = "";
      if (Prologue != NULL) { // ADD Prologue
        pcode = "\n// Prologue\n";
        for (int i = 0; i < Prologue->size(); i++)
          pcode += *(Prologue->at(i)) + "\n";

        //SgNodeHelper::replaceAstWithString(scop, "\n" + ecode + scop->unparseToCompleteString());
      }
      SgNodeHelper::replaceAstWithString(node, "\n" + ecode + node->unparseToCompleteString() + pcode);

      /// NOTE: Do not match scop assign statements here and process them - set mark_scop to true
      /// when pragma scop is encountered and false when endscop is seen - all statements found when
      /// mark_scop=true are inside scop. AstMatcher needs root of scop - ie next basic block (a for stmt)
      /// after pragma - but scop can contain other basic blocks - harder to recognize them during walk -
      /// easier is to record assignments when mark_scop=true. This will identify all assignments across
      /// all basic blocks in scop and is cleaner.
//      ScopStmtVector collectScopStatements;
//      SgStatement *scop = SageInterface::getNextStatement(pragma);
//        ROSE_ASSERT(scop != NULL);
//      MatchResult assign_matches = matcher.performMatching(pc_assign_nodes, scop);
//      collect_scop_statements(1, assign_matches, collectScopStatements);
//
//      for (auto scop_iter : collectScopStatements) {
//        cout << "\n\nScop number " << scop_iter.first << ":\n";
//        cout << "======================\n";

//        for (auto scop_stmt : scop_iter.second) {
//        } //iterate stmts in scop
//      } //collectscopstmts iterator

      return;
    }
  } //isSgPragma
}


int ParseScop(string transformed_input_source, ReturnValues returnValues) {
  //vector<Statement *> statements = returnValues.stmtVecReturn; //from Orchestrator

  /// create rose project using transformed source file
  std::vector<std::string> args(2);
  args[0] = "-rose:skipfinalCompileStep";
  args[1] = transformed_input_source;
  SgProject *project = frontend(args);

  SgFilePtrList &file_list = project->get_fileList();
  ROSE_ASSERT(file_list.size() == 1);
  SgSourceFile *sg_transformed_input = isSgSourceFile(file_list[0]);
  //insert assert header - required for assertions in checker code
  SageInterface::insertHeader(sg_transformed_input, "assert.h", true, true);

  /// ROSE AST walk over the transformed source file
  CodeParser parse_transformed_input(returnValues.stmtVecReturn, returnValues.EpilogueDeclCode, returnValues.UnusedDataCheckerCode,
                     returnValues.LiveDataCheckerCode);
  parse_transformed_input.traverseInputFiles(project, preorder);

  return backend(project);

}
