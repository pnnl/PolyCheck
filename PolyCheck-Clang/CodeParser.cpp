#include <cstdio>
#include <string>
#include <sstream>
#include <iostream>
#include <fstream>

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

#include <Statement.hpp>
#include <CodeParser.hpp>

using namespace clang;
using namespace std;

CompilerInstance* pTheCompInst_g;

struct PdLoc {
        PdLoc() : end(0) {}

        unsigned start;
        unsigned end;
}; // struct PdLoc {

class MyASTVisitor : public RecursiveASTVisitor<MyASTVisitor>
{
public:
  MyASTVisitor (Preprocessor &PP, Rewriter  &R, PdLoc &loc, vector<Statement*> s, vector<string*> *prologue, vector<string*> *epilogueDecl)
    : PP(PP), TheRewriter(R), loc(loc), FaultTolerantRegionStarted(false), statements(s), Prologue(prologue), EpilogueDecl(epilogueDecl), m_inForLine(false)
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
    for (Stmt::child_iterator range = fs->child_begin(); range!=fs->child_end(); ++range)
    {
        // int childrenc=0;
        // for (Stmt::child_iterator rangec = fs->getBody()->child_begin(); rangec!=fs->getBody()->child_end(); ++rangec)
        //    childrenc++;
        // if(childrenc==0) continue;

      if(*range == fs->getBody())
        m_inForLine = false;
      TraverseStmt(fs->getBody());
    }
    return true;
  }//bool TraverseForStmt(ForStmt *fs)


  bool TraverseExprToCheckArray (Expr *expr)
  {
    if (expr->getStmtClass() == Stmt::BinaryOperatorClass)
    {
	    BinaryOperator* b = cast<BinaryOperator>(expr);
	    return ( TraverseExprToCheckArray (b->getLHS()) || TraverseExprToCheckArray (b->getRHS()) );
    }// if (expr->getStmtClass() == Stmt::BinaryOperatorClass)
    else if (expr->getStmtClass() == Stmt::IntegerLiteralClass || expr->getStmtClass() == Stmt::FloatingLiteralClass)
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
 

  void TraverseExprToGetStmtIters (Expr *expr, vector<string> &vecIters, vector<string> &varIds)
  {
    if (expr->getStmtClass() == Stmt::BinaryOperatorClass)
    {
	    BinaryOperator* b = cast<BinaryOperator>(expr);
      varIds.push_back(string((b->getOpcodeStr(b->getOpcode())).str()));//operator
	    TraverseExprToGetStmtIters (b->getLHS(), vecIters, varIds);
	    TraverseExprToGetStmtIters (b->getRHS(), vecIters, varIds);
    }// if (expr->getStmtClass() == Stmt::BinaryOperatorClass)
    else if (expr->getStmtClass() == Stmt::IntegerLiteralClass || expr->getStmtClass() == Stmt::FloatingLiteralClass)
    {
	    // do nothing
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

  bool CheckStmtVarIds(vector<string> stmtVarIds, vector<string>* statementsVarIds)
  {
    if (stmtVarIds.size() != statementsVarIds->size())
    {
      return false;
    }// if
    else
    {
      for (int i=0; i<stmtVarIds.size(); i++)
      {
        //cout << "stmtVarIds: " << stmtVarIds[i] << endl;
        //cout << "statementsVarIds: " << (*statementsVarIds)[i] << endl;
        if ( (stmtVarIds[i]).compare( (*statementsVarIds)[i] ) != 0)// equal
          return false;
      }// end for
      return true;
    }//else
  }//bool CheckStmtVarIds(vector<string> &stmtVarIds, vector<string>* statementsVarIds)

//bug:       LHS could not be expression
//observe:   polyhedral relation will change a-b to -b + a,
           //so every affine iter will have + operator and 
           //with digit at the left and symbol at right.
//fix:       detect +/-[>/% ... and replace with "\n"
//res:       increase the number of symbols in the list
           //obtain the real size by recoginize number 

  bool isNum(string str)
  {
    stringstream sin(str);
    double d;
    char c;
    if(!(sin >> d))
      return false;
    if (sin >> c)
      return false;
    return true;
  }//isNum

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
          // comment out the orginal stmt
          SourceLocation commentLocation = b->getLocStart();
          // bug "{" to avoid checkers go beyond the for loop
	        TheRewriter.InsertText(commentLocation, "{\n//", true, true);

          vector<string> stmtVecIters;        // collect stmt index
          vector<string> stmtVarIds;          // collect stmt var ids
          stmtVarIds.push_back(string((b->getOpcodeStr(b->getOpcode())).str()));//operator
          TraverseExprToGetStmtIters (lhs, stmtVecIters, stmtVarIds);

          // Fix bug for CompoundAssignmentOp
          if (b->isCompoundAssignmentOp())
          {
            // add stmt var ids for compound assignment operator
            if (stmtVarIds.size() > 0 )
            {
              int len = stmtVarIds.size();
              for (int k=0; k<len; k++)
              //for (int ca=0; ca<stmtVarIds.size(); ca++)
              {
                //cout << "stmtVarIds: " << stmtVarIds[k] << endl;
                stmtVarIds.push_back(stmtVarIds[k]);
              }// end for
            }// end stmtVarIds

            // add stmt vector iters for compound assignment operator
            if (stmtVecIters.size() > 0)
            {
              int len = stmtVecIters.size();
              for (int k=0; k<len; k++)
              {
                ////cout << "stmtVarIds: " << stmtVarIds[i] << endl;
                stmtVecIters.push_back(stmtVecIters[k]);
              }// end for
            }
            
          }//if (b->isCompoundAssignmentOp())
          // else do nothing

          TraverseExprToGetStmtIters (rhs, stmtVecIters, stmtVarIds);

          // Insert location for each statement
          SourceLocation END = b->getLocEnd();
	        int offset = Lexer::MeasureTokenLength(END,
	                     pTheCompInst_g->getSourceManager(),
	                     pTheCompInst_g->getLangOpts()) + 1;
	        SourceLocation END1 = END.getLocWithOffset(offset);

          TheRewriter.InsertText(END1, "\n//<checkers>\n", true, true);
          
          // iterate all stmts to find the right one
          for (int i=0; i < statements.size(); i++)
          {
            //for (int i=0; i<stmtVarIds.size(); i++)
              //cout << "stmtVarIds: " << stmtVarIds[i] << endl;
            //cout << "=============" << endl;

            vector<string>* statementsVarIds = statements[i]->getStmtVarIds();
            bool equal = CheckStmtVarIds(stmtVarIds, statementsVarIds);

            if (equal)
            {
              set<string> checkerIters; 
              set<string>::iterator it;
              ComputeCheckerIters(stmtVecIters, statements[i]->getIterators(), checkerIters);
              if (checkerIters.size() > 0)
              {
                for (it=checkerIters.begin(); it!=checkerIters.end(); it++)
                {
                  TheRewriter.InsertText(END1, *it, true, true);
                }
              }

              string* checker = statements[i]->GetDebugCode();
              if (checker != NULL)
                TheRewriter.InsertText(END1, *checker, true, true);

            }//if (equal)
            /*should not include else part since there always exsits unmatched stmt*/ 
            //else 
            //{
              //TheRewriter.InsertText(END1, "Can not find match\n", true, true);
            //}
            
          }//for (int i=0; i < statements.size(); i++)

          TheRewriter.InsertText(END1, "//</checkers>\n}", true, true);

        }//if ( lhsArray || rhsArray)
      }//if (!m_inForLine)
    }// if (b->isAssignmentOp())

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
        if (Prologue != NULL)
        {
          for (int i=0; i < Prologue->size(); i++)
          {
            TheRewriter.InsertTextAfter(s->getLocStart(), "// Prologue\n");
            TheRewriter.InsertTextAfter(s->getLocStart(), *(Prologue->at(i)));
            TheRewriter.InsertTextAfter(s->getLocStart(), "\n");
          }//for (int i=0; i < Prologue->size(); i++)
        }//if (Prologue != NULL)
      }// if (FaultTolerantRegionStarted == false)

      //static int divid = 0;
      if (isa<IfStmt>(s))
      {
	      IfStmt *IfStatement = cast<IfStmt>(s);
	      Stmt *Then = IfStatement->getThen();

	      PresumedLoc PLoc = pTheCompInst_g->getSourceManager().getPresumedLoc(IfStatement->getLocStart());//, 1);
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
	        SourceLocation END = Else->getLocEnd();
	        int offset = Lexer::MeasureTokenLength(END, pTheCompInst_g->getSourceManager(),
	      					 pTheCompInst_g->getLangOpts()) + 1;
          SourceLocation END1 = END.getLocWithOffset(offset);

	        TheRewriter.InsertText(Else->getLocStart(), "// the 'else' part\n", true, true);
	      }
      }

      return true;
  }//bool VisitStmt(Stmt *s)

  bool VisitFunctionDecl (FunctionDecl *f)
  {
     if (f->hasBody())
     {
	      Stmt *FunBody = f->getBody();
	      QualType QT = f->getCallResultType();
	      string TypeStr = QT.getAsString();

	      DeclarationName DeclName = f->getNameInfo().getName();
	      string FuncName = DeclName.getAsString();

	      stringstream SSBefore;
	      SSBefore << "// Epilogue Declaration Code in Global " << "\n";
        for (int i=0; i<EpilogueDecl->size(); i++)
          SSBefore << *(EpilogueDecl->at(i)) ;

	      SSBefore << "// Begin function " << FuncName << " returning " << TypeStr << "\n";
	      SourceLocation ST = f->getSourceRange().getBegin();
	      TheRewriter.InsertText(ST, SSBefore.str(), true, true);

	      stringstream SSAfter;
	      SSAfter << "// End function " << FuncName << "\n";
	      ST = FunBody->getLocEnd().getLocWithOffset(1);
	      TheRewriter.InsertText(ST, SSAfter.str(), true, true);
     }

     return true;
  }//bool VisitFunctionDecl (FunctionDecl *f)

private:
  Rewriter &TheRewriter;
  PdLoc &loc;
  Preprocessor &PP;
  bool FaultTolerantRegionStarted;
  vector<Statement*> statements;
  vector<string*> *Prologue;
  vector<string*> *EpilogueDecl;
  bool m_inForLine;
};

class MyASTConsumer : public ASTConsumer
{
public:
  MyASTConsumer(Preprocessor &PP, Rewriter &R, PdLoc &l, vector<Statement*> s, vector<string*> *prologue, vector<string*> *epilogueDecl) :
      Visitor (PP, R, l, s, prologue, epilogueDecl), loc(l), PP(PP), statements(s), Prologue(prologue), EpilogueDecl(epilogueDecl)
  {
    statements = s;
  }// MyASTConsumer(Preprocessor &PP, Rewriter &R, PdLoc &l, vector<Statement*> s) : Visitor (PP, R, l, s), loc(l), PP(PP), statements(s)

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
  vector<Statement*> &statements;
  vector<string*> *Prologue;
  vector<string*> *EpilogueDecl;

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

	    TheRewriter.InsertTextAfter(sloc, "#include<assert.h>\n");
      TheRewriter.InsertTextAfter(sloc,"#define ceild(n,d)  ceil(((double)(n))/((double)(d))) \n");
      TheRewriter.InsertTextAfter(sloc,"#define floord(n,d) floor(((double)(n))/((double)(d))) \n");

      TheRewriter.InsertTextAfter(sloc, "#define max(x,y)    ((x) > (y)? (x) : (y))\n");
      TheRewriter.InsertTextAfter(sloc, "#define min(x,y)    ((x) < (y)? (x) : (y))\n");
	    TheRewriter.InsertTextAfter(sloc, "int checksum;\n");
	    TheRewriter.InsertTextAfter(sloc, "int version;\n");
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
  vector<string*> *Epilogue;

  PragmaEndPdHandler(PdLoc &loc, Rewriter &R, vector<string*> *epilogue) : PragmaHandler("endscop"), loc(loc), TheRewriter(R), Epilogue(epilogue)
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
      
		  TheRewriter.InsertTextBefore(sloc, "\n assert(checksum == 0);\n");
      // ADD Epilogue
      if (Epilogue != NULL)
      {
        for (int i=0; i < Epilogue->size(); i++)
        {
          TheRewriter.InsertTextBefore(sloc, *(Epilogue->at(i)));
          TheRewriter.InsertTextBefore(sloc, "// Epilogue\n");
        }//for (int i=0; i < Epilogue->size(); i++)
      }//if (Epilogue != NULL)
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
int ParseScop(string fileName, ReturnValues returnValues, string outputFileName)
{
  vector<Statement*> statements = returnValues.stmtVecReturn; //from Orchestrator

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
  PP.AddPragmaHandler(new PragmaEndPdHandler(loc, TheRewriter, returnValues.UnusedDataCheckerCode));

  // Create an AST consumer instance which is going to get called by ParseAST
  //MyASTConsumer TheConsumer(PP, TheRewriter, loc, statements,returnValues.LiveDataCheckerCode);
  MyASTConsumer TheConsumer(PP,TheRewriter,loc,statements,returnValues.LiveDataCheckerCode,returnValues.EpilogueDeclCode);

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
