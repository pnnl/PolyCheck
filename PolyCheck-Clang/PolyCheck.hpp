#include <cstdio>
#include <string>
#include <sstream>
#include <iostream>
#include <fstream>

#include "islw.hpp"
#include "util.hpp"
#include "array_pack.hpp"
#include "statement.hpp"

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/FileManager.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Basic/TargetOptions.h"
#include "clang/Basic/TargetInfo.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Parse/ParseAST.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Rewrite/Frontend/Rewriters.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/raw_ostream.h"
#include <clang/Lex/Pragma.h>
#include <clang/Lex/Preprocessor.h>
#include <clang/AST/Expr.h>
#include <clang/AST/PrettyPrinter.h>

#include <map>
#include <set>

using namespace clang;
using namespace std;

CompilerInstance* pTheCompInst_g;

int ParseScop(string fileName, string prolog, string epilog, string outputFileName);

void GatherStmtVarIds(vector<std::string>& stmtVarIds, struct pet_expr* expr, isl_set* domainSet){
  if (pet_expr_get_type(expr) == pet_expr_op){
    if (pet_expr_op_get_type(expr) == pet_op_assume
        || pet_expr_op_get_type(expr) == pet_op_kill 
        || pet_expr_op_get_type(expr) == pet_op_assume 
        //|| pet_expr_op_get_type(expr) == pet_op_cond
        || pet_expr_op_get_type(expr) == pet_op_last
        ) {
          pet_expr_free(expr);
          return;
        }
    stmtVarIds.push_back(pet_op_str(pet_expr_op_get_type(expr)));
    // GatherStmtOpIds(stmtVarIds, expr);
  }

  if (pet_expr_get_type(expr) == pet_expr_access){
    if(pet_expr_access_is_write(expr)){
      isl_map* may_write = isl_map_from_union_map(pet_expr_access_get_may_write(expr));
      isl_map* wmap = isl_map_intersect_domain(may_write,isl_set_copy(domainSet));
      
      const char* wchar = isl_map_get_tuple_name(wmap, isl_dim_out);
      if (wchar) stmtVarIds.push_back(string(wchar));
      islw::destruct(wmap);
    }

    if(pet_expr_access_is_read(expr)){
      isl_map* may_read = isl_map_from_union_map(pet_expr_access_get_may_read(expr));
      isl_map* rmap = isl_map_intersect_domain(may_read,isl_set_copy(domainSet));

      const char* rchar = isl_map_get_tuple_name(rmap, isl_dim_out);
      if(rchar) stmtVarIds.push_back(rchar);
      islw::destruct(rmap);
    }
  }else if (pet_expr_get_type(expr) == pet_expr_int){
    isl_val* ival = pet_expr_int_get_val(expr);
    string int_str = std::to_string(stod(std::to_string(isl_val_sgn(ival))));
    islw::destruct(ival); 
    //cout << "integer: " << int_str << endl;
    stmtVarIds.push_back(int_str);
  }else if (pet_expr_get_type(expr) == pet_expr_double){
    const char* double_char = pet_expr_double_get_str(expr); 
    // cout << "pet double:" << double_char  << endl;
    if(double_char) stmtVarIds.push_back(std::to_string(stod(double_char))); 
  }

  for(int i=0; i<pet_expr_get_n_arg(expr); i++) {
    GatherStmtVarIds(stmtVarIds, pet_expr_get_arg(expr,i), domainSet);
  }
  pet_expr_free(expr);
}

struct PdLoc {
        PdLoc() : end(0) {}

        unsigned start;
        unsigned end;
}; // struct PdLoc {

class MyASTVisitor : public RecursiveASTVisitor<MyASTVisitor>
{
public:
  MyASTVisitor (Preprocessor &PP, Rewriter  &R, PdLoc &loc, std::vector<Statement> stmts, string prologue, string epilogue)
    :  TheRewriter(R), loc(loc), PP(PP), FaultTolerantRegionStarted(false), stmts(stmts), Prologue(prologue), Epilogue(epilogue), m_inForLine(false)
  {}
  
  bool TraverseForStmt(ForStmt *fs)// Traverse for stmt to make mark
  {
    SourceManager &SM = PP.getSourceManager();

    /*Standard checking begins*/
    if (SM.getFileOffset(fs->getLocStart()) <= loc.start)
      return true;
    if (SM.getFileOffset(fs->getLocEnd()) >= loc.end)
    {
      if (FaultTolerantRegionStarted == true)
      {
        FaultTolerantRegionStarted = false;
      }// if (FaultTolerantRegionStarted == true)
      return true;
    }
    /*Standard checking ends*/

    // Enter scop region
    if (!WalkUpFromForStmt(fs))
      return false;

    m_inForLine = true;

      clang::LangOptions LangOpts;
      LangOpts.CPlusPlus = true;
      clang::PrintingPolicy Policy(LangOpts);

    for (Stmt::child_iterator range = fs->child_begin(); range!=fs->child_end(); ++range)
    {
      if(*range == fs->getBody()){
        m_inForLine = false;
        TraverseStmt(fs->getBody());
      }

      // cout << "------------------body start-------" << m_inForLine << "----------------\n";
      // string fbdy;
      // llvm::raw_string_ostream fbdyst(fbdy);
      //  fs->getBody()->printPretty(fbdyst,NULL,Policy);
      //  cout << fbdyst.str() << endl;
      // cout << "------------------body end----------------\n";
      
    }
    return true;
  }//bool TraverseForStmt(ForStmt *fs)


  bool TraverseExprToCheckArray (Expr *expr)
  {
    if (expr->getStmtClass() == Stmt::BinaryOperatorClass)
    {
	    BinaryOperator* b = cast<BinaryOperator>(expr);
	    return ( TraverseExprToCheckArray (b->getLHS()) || TraverseExprToCheckArray (b->getRHS()) );
    }
    else if (expr->getStmtClass() == Stmt::UnaryOperatorClass)
    {
	    UnaryOperator* u = cast<UnaryOperator>(expr);
	    return ( TraverseExprToCheckArray (u->getSubExpr()) );
    }
    else if (expr->getStmtClass() == Stmt::ConditionalOperatorClass)
    {
	    ConditionalOperator* c = cast<ConditionalOperator>(expr);
	    return ( TraverseExprToCheckArray (c->getCond()) ||
              TraverseExprToCheckArray (c->getTrueExpr()) ||
              TraverseExprToCheckArray (c->getFalseExpr()) );
    }
    else if (expr->getStmtClass() == Stmt::IntegerLiteralClass ||
             expr->getStmtClass() == Stmt::FloatingLiteralClass)
    {
	    // do nothing
      return false;
    }// else if (expr->getStmtClass() == Stmt::IntegerLiteralClass)
    else if (expr->getStmtClass() == Stmt::ParenExprClass)
    {
	    ParenExpr *p = cast<ParenExpr>(expr);
	    return TraverseExprToCheckArray(p->getSubExpr());
    }// else if (expr->getStmtClass() == Stmt::ParenExprClass)
    else if (expr->getStmtClass() == Stmt::ImplicitCastExprClass)
    {
	    ImplicitCastExpr *i = cast<ImplicitCastExpr>(expr);
	    return TraverseExprToCheckArray(i->getSubExpr());
    }// else if (expr->getStmtClass() == Stmt::ParenExprClass)
    else if (expr->getStmtClass() == Stmt::ArraySubscriptExprClass)// isa<ArraySubscriptExpr>(expr) 
    {
      return true;
    }//else if (expr->getStmtClass() == Stmt::ArraySubscriptExprClass)
    else
    {
      return false;
    }//else
  }// bool TraverseExprToCheckArray (Expr *expr)
 

  void TraverseExprToGetStmtIters (Expr *expr, vector<std::string> &vecIters, vector<std::string> &varIds)
  {
    if (expr->getStmtClass() == Stmt::BinaryOperatorClass)
    {
	    BinaryOperator* b = cast<BinaryOperator>(expr);
      varIds.push_back(string((b->getOpcodeStr(b->getOpcode())).str()));//operator
	    TraverseExprToGetStmtIters (b->getLHS(), vecIters, varIds);
	    TraverseExprToGetStmtIters (b->getRHS(), vecIters, varIds);
    }
    else if (expr->getStmtClass() == Stmt::UnaryOperatorClass)
    {
	    UnaryOperator* u = cast<UnaryOperator>(expr);
      varIds.push_back(string((u->getOpcodeStr(u->getOpcode())).str()));//operator
	    TraverseExprToGetStmtIters (u->getSubExpr(), vecIters, varIds);
    }
    else if (expr->getStmtClass() == Stmt::CallExprClass)
    {
	    CallExpr* c = cast<CallExpr>(expr);
      for(auto x = 0U; x < c->getNumArgs(); x++)
        TraverseExprToGetStmtIters (c->getArg(x), vecIters, varIds);
    }
        else if (expr->getStmtClass() == Stmt::ConditionalOperatorClass)
    {
	    ConditionalOperator* c = cast<ConditionalOperator>(expr);
      varIds.push_back(string("?:"));
      TraverseExprToGetStmtIters (c->getCond(), vecIters, varIds);
      TraverseExprToGetStmtIters (c->getTrueExpr(), vecIters, varIds);
      TraverseExprToGetStmtIters (c->getFalseExpr(), vecIters, varIds);
    }
    else if (expr->getStmtClass() == Stmt::IntegerLiteralClass ||
             expr->getStmtClass() == Stmt::FloatingLiteralClass)
    {
	    clang::LangOptions LangOpts;
      LangOpts.CPlusPlus = true;
      clang::PrintingPolicy Policy(LangOpts);

      string varIdStr;
      llvm::raw_string_ostream varId(varIdStr);
      expr->printPretty(varId, NULL, Policy);
      //cout << "var ids: " << varId.str() << endl;
      varIds.push_back(std::to_string(stod(varId.str())));
    }// else if (expr->getStmtClass() == Stmt::IntegerLiteralClass)
    else if (expr->getStmtClass() == Stmt::ParenExprClass)
    {
	    ParenExpr *p = cast<ParenExpr>(expr);
	    TraverseExprToGetStmtIters(p->getSubExpr(), vecIters, varIds);
    }// else if (expr->getStmtClass() == Stmt::ParenExprClass)
    else if (expr->getStmtClass() == Stmt::ImplicitCastExprClass)
    {
	    ImplicitCastExpr *i = cast<ImplicitCastExpr>(expr);
	    TraverseExprToGetStmtIters(i->getSubExpr(), vecIters, varIds);
    }// else if (expr->getStmtClass() == Stmt::ParenExprClass)
    else if (expr->getStmtClass() == Stmt::ArraySubscriptExprClass)
    {
      ArraySubscriptExpr* arrayExpr = cast<ArraySubscriptExpr>(expr);
      TraverseExprToGetStmtIters (arrayExpr->getLHS(), vecIters, varIds);
			//TraverseExprToGetStmtIters (arrayExpr->getRHS(), vecIters);
      
      // store vector iterators 
      clang::LangOptions LangOpts;
      LangOpts.CPlusPlus = true;
      clang::PrintingPolicy Policy(LangOpts);

	    string vecIterStr;
      llvm::raw_string_ostream vecIter(vecIterStr);
	    arrayExpr->getRHS()->printPretty(vecIter, NULL, Policy);
			//cout << "Idx: " << s.str() << endl;
	    vecIters.push_back(vecIter.str());

    }// isa<ArraySubscriptExpr>(expr) 
    else// DeclRefExpr: vector name, variable name
    {
      // store vector and variable names
      clang::LangOptions LangOpts;
      LangOpts.CPlusPlus = true;
      clang::PrintingPolicy Policy(LangOpts);

      string varIdStr;
      llvm::raw_string_ostream varId(varIdStr);
      expr->printPretty(varId, NULL, Policy);
      //cout << "var ids: " << varId.str() << endl;
      varIds.push_back(varId.str());

	  }// else
  }//void TraverseExprToGetStmtIters 

  bool CheckStmtVarIds(vector<std::string> stmtVarIds, vector<std::string> petStmtVarIds)
  {
    if (stmtVarIds.size() != petStmtVarIds.size())
      return false;
      
    for (auto i=0U; i<stmtVarIds.size(); i++)
    {
      if ((stmtVarIds[i]).compare(petStmtVarIds[i]) != 0)
        return false;
    }
    return true;
    
  }
 
  bool VisitBinaryOperator(BinaryOperator *b)
  {
    /*Standard checking begins*/
    SourceManager &SM = PP.getSourceManager();
    if (SM.getFileOffset(b->getLocStart()) <= loc.start)
      return true;
    if (SM.getFileOffset(b->getLocEnd()) >= loc.end)
    {
	    if (FaultTolerantRegionStarted == true)
	    {
	      FaultTolerantRegionStarted = false;
	      TheRewriter.InsertTextBefore(b->getLocStart(), "// Fault tolerance ends here\n");
	    }// if (FaultTolerantRegionStarted == true)
      return true;
    }
    /*Standard checking ends*/
  #if 1
    // Entering the fault tolerant region
    if (b->isAssignmentOp() || b->isCompoundAssignmentOp())
    {
      if (!m_inForLine)// check BinaryOperator not in for loop
      {
        Expr *lhs = b->getLHS();
        Expr *rhs = b->getRHS();

        bool lhsArray = isa<ArraySubscriptExpr>(lhs);
        bool rhsArray = TraverseExprToCheckArray(rhs);

        // Check if array reference Statement
        // if true perform insertion, else do nothing
        if ( lhsArray || rhsArray )
        {
          vector<std::string> stmtVecIters;        // collect stmt index
          vector<std::string> stmtVarIds;          // collect stmt var ids
          stmtVarIds.push_back(string((b->getOpcodeStr(b->getOpcode())).str()));//operator
          TraverseExprToGetStmtIters (lhs, stmtVecIters, stmtVarIds);

          // Fix bug for CompoundAssignmentOp
          if (b->isCompoundAssignmentOp())
          {
            // add stmt var ids for compound assignment operator
            if (stmtVarIds.size() > 0 )
            {
              int len = stmtVarIds.size();
              for (int k=1; k<len; k++)
                stmtVarIds.push_back(stmtVarIds[k]);
            }
            // add stmt vector iters for compound assignment operator
            if (stmtVecIters.size() > 0)
            {
              int len = stmtVecIters.size();
              for (int k=0; k<len; k++)
                stmtVecIters.push_back(stmtVecIters[k]);
            }
          }
         
          TraverseExprToGetStmtIters (rhs, stmtVecIters, stmtVarIds);

          // comment the orginal stmt
          SourceLocation commentLocation = b->getLocStart();
          TheRewriter.InsertText(commentLocation, "/* ", true, true);

          // Insert location for each statement
          SourceManager &src_mgr = pTheCompInst_g->getSourceManager();
          LangOptions &clangopts = pTheCompInst_g->getLangOpts();
          SourceLocation END = b->getLocEnd();
          if(src_mgr.isMacroBodyExpansion(END)){
             auto csr = src_mgr.getExpansionRange(END);
             END = csr.second;
          }

	        int offset = Lexer::MeasureTokenLength(END,
	                     src_mgr, clangopts) + 1;
	        SourceLocation END1 = END.getLocWithOffset(offset);

          TheRewriter.InsertText(END1, " */ \n//---begin checks---\n", true, true);
          
          // iterate all stmts to find the right one
          std::vector<Statement*> pstmts;
          std::vector<std::vector<std::string>> stmtVecItersList;
          for (auto i=0U; i < stmts.size(); i++)
          {
            vector<std::string> petStmtVarIds = stmts[i].stmt_varids();   
            bool equal = CheckStmtVarIds(stmtVarIds, petStmtVarIds);
            if (equal){
              //TheRewriter.InsertText(END1, stmts[i].inline_checks(stmtVecIters), true, true);
              pstmts.push_back(&stmts[i]);
              stmtVecItersList.push_back(stmtVecIters);
              //break;
            }
          } //
          TheRewriter.InsertText(
            END1, Statement::inline_checks(pstmts, stmtVecItersList), true,
            true);

          TheRewriter.InsertText(END1, "//---end checks---\n", true, true);
          

        }//if ( lhsArray || rhsArray)
      }//if (!m_inForLine)
    }// if (b->isAssignmentOp())
    #endif

    return true;
  }//bool VisitBinaryOperator(BinaryOperator *b)

  bool VisitStmt(Stmt *s)
  {
      SourceManager &SM = PP.getSourceManager();

      if (SM.getFileOffset(s->getLocStart()) <= loc.start)
        return true;

      // else if(SM/getFileOffset(s->getLocStart())>loc.start)
      if (SM.getFileOffset(s->getLocEnd()) >= loc.end)
      {
	      if (FaultTolerantRegionStarted == true)
	      {
	        FaultTolerantRegionStarted = false;
	      }// if (FaultTolerantRegionStarted == true)
        return true;
      }

      // else if(SM.getFileOffset(s->getLocStart())>loc.start&&SM.getFileOffset(s->getLocEnd()) < loc.end)
      if (FaultTolerantRegionStarted == false)
      {
      	FaultTolerantRegionStarted = true;

        // ADD Prologue

            TheRewriter.InsertTextAfter(s->getLocStart(), "// Prologue\n");
            TheRewriter.InsertTextAfter(s->getLocStart(), Prologue);
            TheRewriter.InsertTextAfter(s->getLocStart(), "\n");
      }// if (FaultTolerantRegionStarted == false)

      //static int divid = 0;
      if (isa<IfStmt>(s))
      {
	      IfStmt *IfStatement = cast<IfStmt>(s);
	      Stmt *Then = IfStatement->getThen();

	      // PresumedLoc PLoc = pTheCompInst_g->getSourceManager().getPresumedLoc(IfStatement->getLocStart());//, 1);
	      // cout << "Line number is " << PLoc.getLine() << endl;
	      // cout << "Line number is " << pTheCompInst_g->getSourceManager().getSpellingLineNumber(IfStatement->getLocStart(), 0) << endl;
	      // cout << "Line number is " << pTheCompInst_g->getSourceManager().getExpansionLineNumber(IfStatement->getLocStart(), 0) << endl;
	      // cout << "Line At the end: number is " << pTheCompInst_g->getSourceManager().getPresumedLineNumber(IfStatement->getLocStart(), 0) << endl;
	      // cout << "-----------------------\n";

	      stringstream str_before;
	      //divid++;
	      TheRewriter.InsertText(Then->getLocStart(), str_before.str(), true, true);

	      SourceLocation END = Then->getLocEnd();
	      int offset = Lexer::MeasureTokenLength(END, pTheCompInst_g->getSourceManager(),	pTheCompInst_g->getLangOpts()) + 1;
	      SourceLocation END1 = END.getLocWithOffset(offset);

	      stringstream str_after;
	      TheRewriter.InsertText(END1, str_after.str(), true, true);

	      Stmt *Else = IfStatement->getElse();

	      if (Else)
	      {
	        // SourceLocation END = Else->getLocEnd();
	        // int offset = Lexer::MeasureTokenLength(END, pTheCompInst_g->getSourceManager(),
	      					//  pTheCompInst_g->getLangOpts()) + 1;
          // SourceLocation END1 = END.getLocWithOffset(offset);

	        TheRewriter.InsertText(Else->getLocStart(), "// the 'else' part\n", true, true);
	      }
      }

      return true;
  }//bool VisitStmt(Stmt *s)

  bool VisitFunctionDecl (FunctionDecl *f)
  {
    //  if (f->hasBody())
    //  {
	  //     Stmt *FunBody = f->getBody();
	  //     QualType QT = f->getCallResultType();
	  //     string TypeStr = QT.getAsString();

	  //     DeclarationName DeclName = f->getNameInfo().getName();
	  //     string FuncName = DeclName.getAsString();

	  //     stringstream SSBefore;
	  //     SSBefore << "// Epilogue Declaration Code in Global " << "\n";
    //     // for (int i=0; i<EpilogueDecl->size(); i++)
    //     //   SSBefore << *(EpilogueDecl->at(i)) ;

	  //     SSBefore << "// Begin function " << FuncName << " returning " << TypeStr << "\n";
	  //     SourceLocation ST = f->getSourceRange().getBegin();
	  //     TheRewriter.InsertText(ST, SSBefore.str()+Epilogue, true, true);

	  //     stringstream SSAfter;
	  //     SSAfter << "// End function " << FuncName << "\n";
	  //     ST = FunBody->getLocEnd().getLocWithOffset(1);
	  //     TheRewriter.InsertText(ST, SSAfter.str(), true, true);
    //  }

     return true;
  }

private:
  Rewriter &TheRewriter;
  PdLoc &loc;
  Preprocessor &PP;
  bool FaultTolerantRegionStarted;
  std::vector<Statement> stmts;
  string Prologue;
  string Epilogue;
  bool m_inForLine;
};

class MyASTConsumer : public ASTConsumer
{
public:
  MyASTConsumer(Preprocessor &PP, Rewriter &R, PdLoc &l, std::vector<Statement> stmts, string prologue, string epilogue) :
      Visitor (PP, R, l, stmts, prologue, epilogue), loc(l), PP(PP), stmts(stmts), Prologue(prologue), Epilogue(epilogue)
  {
  }

  virtual bool HandleTopLevelDecl (DeclGroupRef DR)
  {
    for (DeclGroupRef::iterator b = DR.begin(), e = DR.end(); b != e; ++b)
    {
      FunctionDecl *fd = dyn_cast<clang::FunctionDecl>(*b);
      if (!fd)
        continue;
      if (!fd->hasBody())
        continue;
      SourceManager &SM = PP.getSourceManager();

      if (SM.getFileOffset(fd->getLocStart()) > loc.end)
        continue;
      if (SM.getFileOffset(fd->getLocEnd()) < loc.start)
        continue;
      Visitor.TraverseDecl(*b);
    }
    return true;
  }// virtual bool HandleTopLevelDecl (DeclGroupRef DR)

private:
  MyASTVisitor Visitor;
  PdLoc &loc;
  Preprocessor &PP;
  std::vector<Statement> stmts;
  string Prologue;
  string Epilogue;

}; // class MyASTConsumer : public ASTConsumer

/* Handle pragmas of the form
 *      #pragma Pd
 * In particular, store the location of the line containing
 * the pragma in loc.start.
 */
struct PragmaPdHandler : public PragmaHandler {
  PdLoc &loc;
	Rewriter &TheRewriter;

  PragmaPdHandler(PdLoc &loc, Rewriter &R) : PragmaHandler("scop"), loc(loc), TheRewriter(R)
	{}


  virtual void HandlePragma(Preprocessor &PP,
                            PragmaIntroducerKind Introducer,
                            Token &ScopTok) {

      SourceManager &SM = PP.getSourceManager();
      SourceLocation sloc = ScopTok.getLocation();
      int line = SM.getExpansionLineNumber(sloc);
      sloc = SM.translateLineCol(SM.getFileID(sloc), line, 1);
      loc.start = SM.getFileOffset(sloc);

	    // TheRewriter.InsertTextAfter(sloc, "#include<assert.h>\n");
      TheRewriter.InsertTextAfter(sloc, "#define max(x,y)    ((x) > (y)? (x) : (y))\n");
      TheRewriter.InsertTextAfter(sloc, "#define min(x,y)    ((x) < (y)? (x) : (y))\n");
      TheRewriter.InsertTextAfter(sloc,"#define ceild(n,d)  ceil(((double)(n))/((double)(d))) \n");
      TheRewriter.InsertTextAfter(sloc,"#define floord(n,d) floor(((double)(n))/((double)(d))) \n");

	    // TheRewriter.InsertTextAfter(sloc, "int checksum;\n");
	    // TheRewriter.InsertTextAfter(sloc, "int version;\n");
      // The following insert is not necessary
      // since the pocc will keep the original 
      // iterator such as int i j k
			// TheRewriter.InsertTextAfter(sloc, "register int i, j, k;\n");

      //cout << "Pragma scop found at: " << loc.start << endl;
      }
};

/* Handle pragmas of the form
 *      #pragma Pd
 * In particular, store the location of the line containing
 * the pragma in loc.end.
 */

struct PragmaEndPdHandler : public PragmaHandler {
  PdLoc &loc;
	Rewriter &TheRewriter;
  string Epilogue;

  PragmaEndPdHandler(PdLoc &loc, Rewriter &R, string epilogue) : PragmaHandler("endscop"), loc(loc), TheRewriter(R), Epilogue(epilogue)
	{}

  virtual void HandlePragma(Preprocessor &PP,
                            PragmaIntroducerKind Introducer,
                            Token &ScopTok) {

      SourceManager &SM = PP.getSourceManager();
      SourceLocation sloc = ScopTok.getLocation();
      int line = SM.getExpansionLineNumber(sloc);
      sloc = SM.translateLineCol(SM.getFileID(sloc), line, 1);
      loc.end = SM.getFileOffset(sloc);
      //cout << "Pragma endscop found at: " << loc.end << endl;
      
      // ADD Epilogue
          TheRewriter.InsertTextBefore(sloc, Epilogue);
          TheRewriter.InsertTextBefore(sloc, "// Epilogue\n");
          TheRewriter.InsertTextBefore(sloc, "\n");
  }
};

void WriteToFile(string outputFileName, string output)
{
  ofstream ofs (outputFileName.c_str(), ofstream::out);
  ofs << output;
  ofs.close();
}// void WriteToFile(string outputFileName, string output)

// ParseScop: the main file 
int ParseScop(string fileName, std::vector<Statement> &stmts, string prologue, string epilogue, string outputFileName)
{
  // CompilerInstance will hold the instance of the Clang compiler,
  // managing the various objects needed to run the compiler.
  CompilerInstance TheCompInst;
  pTheCompInst_g = &TheCompInst;
  TheCompInst.createDiagnostics ();

  // Initialize target info with the default triple for our platform.
//  TargetOptions TO;
//  TO.Triple = llvm::sys::getDefaultTargetTriple();
  std::shared_ptr<clang::TargetOptions> TO = std::make_shared<clang::TargetOptions>();
  TO->Triple = llvm::sys::getDefaultTargetTriple();

  TargetInfo *TI = TargetInfo::CreateTargetInfo(
		    TheCompInst.getDiagnostics(), TO);
  TheCompInst.setTarget(TI);

  TheCompInst.createFileManager();
  FileManager &FileMgr = TheCompInst.getFileManager();
  TheCompInst.createSourceManager (FileMgr);
  SourceManager &SourceMgr = TheCompInst.getSourceManager();
  TheCompInst.createPreprocessor(clang::TU_Complete);
  TheCompInst.createASTContext();

  // A Rewriter helps us manage the code rewriting task.
  Rewriter TheRewriter;
  TheRewriter.setSourceMgr(SourceMgr, TheCompInst.getLangOpts());

  // Set the main file handled by the source manager to the input file.
  const FileEntry *FileIn = FileMgr.getFile(fileName);
  clang::FileID pcFileID = SourceMgr.createFileID(FileIn,clang::SourceLocation(), clang::SrcMgr::C_User);
  SourceMgr.setMainFileID(pcFileID);
  TheCompInst.getDiagnosticClient().BeginSourceFile(
       TheCompInst.getLangOpts(), &TheCompInst.getPreprocessor());


  Preprocessor &PP = TheCompInst.getPreprocessor();

  // Data structures to store #pragma PD, and #pragma endPD information
  PdLoc loc;
  PP.AddPragmaHandler(new PragmaPdHandler(loc, TheRewriter));
  PP.AddPragmaHandler(new PragmaEndPdHandler(loc, TheRewriter, epilogue));

  // Create an AST consumer instance which is going to get called by ParseAST
  //MyASTConsumer TheConsumer(PP, TheRewriter, loc, statements,returnValues.LiveDataCheckerCode);
  MyASTConsumer TheConsumer(PP,TheRewriter,loc,stmts, prologue,epilogue);

  // Parse the file to AST, registering our consumer as the AST consumer.
  ParseAST(PP, &TheConsumer, TheCompInst.getASTContext());

  // At this point the rewriter's buffer should be full with the rewritten file contents.
  const RewriteBuffer *RewriteBuf =
   TheRewriter.getRewriteBufferFor(SourceMgr.getMainFileID());

  string Output = string (RewriteBuf->begin(), RewriteBuf->end());

  WriteToFile(outputFileName, Output);
  cout << "Wrote result to " << outputFileName << endl;
 // exit(-1);
  return 0;
}//int ParseScop(char *fileName, ReturnValues returnValues, string outputFileName)
