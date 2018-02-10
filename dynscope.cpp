#include<iostream>
#include<string.h>
#include<sstream>
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Tooling/Tooling.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "llvm/Support/CommandLine.h"
#include "clang/Driver/Options.h"
#include "clang/AST/AST.h"
#include "clang/AST/ASTContext.h"
#include "clang/Frontend/ASTConsumers.h"
#include "clang/Rewrite/Core/Rewriter.h"

using namespace std;
using namespace llvm;
using namespace clang;
using namespace clang::tooling;
using namespace clang::driver;

Rewriter rewriter;
static llvm::cl::OptionCategory MyToolCategory("example options");
static cl::extrahelp CommonHelp(CommonOptionsParser::HelpMessage);
static cl::extrahelp MoreHelp("\nMore help text...");

class FindNamedClassVisitor : public RecursiveASTVisitor<FindNamedClassVisitor> {
public:
  explicit FindNamedClassVisitor(ASTContext *Context)
    : Context(Context) {
        rewriter.setSourceMgr(Context->getSourceManager(),
            Context->getLangOpts());
    }
  /*
  bool VisitCXXRecordDecl(CXXRecordDecl *Declaration) {
    //if (Declaration->getQualifiedNameAsString() == "q") {
      FullSourceLoc FullLocation = Context->getFullLoc(Declaration->getLocStart());
      if (FullLocation.isValid()){
        llvm::outs() << "Found Record declaration at "
                     << FullLocation.getSpellingLineNumber() << ":"
                     << FullLocation.getSpellingColumnNumber() << "\n";
      //  rewriter.InsertTextBefore(Declaration->getLocStart() , "\/* inserted text *\/");
      }                 
   // }
    return true;
  }
  */
  
	bool VisitFunctionDecl(FunctionDecl *f) {
		if (f->isThisDeclarationADefinition()) {
			FullSourceLoc loc = Context->getFullLoc (f->getLocStart() );
      			llvm::outs()<<f->getQualifiedNameAsString()<<" declared at " << loc.getSpellingLineNumber()<<":"<<loc.getSpellingColumnNumber();
			unsigned num_param =f->getNumParams(); 
			outs()<<".Number of parameters = "<<num_param<<"\n";
			if (num_param > 0 ){
				ArrayRef< ParmVarDecl * > param_array=f->parameters();
				for(unsigned i=0;i<num_param;i++){
					outs()<<"\t"<<param_array[i]->getNameAsString()<<"\t";
					QualType type=param_array[i]->getOriginalType();
					outs()<<type.getAsString()<<"\n";
				}
			}
			Stmt *body = f->getBody();
			body->dumpColor();
			clang::Stmt::child_iterator begin= body->child_begin();
			clang::Stmt::child_iterator end= body->child_end();
			while(begin != end){


				std::string stmt_type=begin->getStmtClassName();
				//cout<<a<<"\n";
				if (stmt_type.compare("DeclStmt")==0 ){
					
		
				}	
				begin++;
			}
			
		}
		return true;
	}
	
	/*
	bool VisitVarDecl(VarDecl* vd){
		FullSourceLoc loc = Context->getFullLoc (vd->getLocStart() );
		llvm::outs()<<vd->getQualifiedNameAsString()<<" declared at " << loc.getSpellingLineNumber()<<":"<<loc.getSpellingColumnNumber();
		
		return true;	
	}
	*/
private:
  ASTContext *Context;
};

class FindNamedClassConsumer : public clang::ASTConsumer {
public:
  explicit FindNamedClassConsumer(ASTContext *Context)
    : Visitor(Context) {}

  virtual void HandleTranslationUnit(clang::ASTContext &Context) {
    Visitor.TraverseDecl(Context.getTranslationUnitDecl());
  }
private:
  FindNamedClassVisitor Visitor;
};

class FindNamedClassAction : public clang::ASTFrontendAction {
public:
  virtual std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(
    clang::CompilerInstance &Compiler, llvm::StringRef InFile) {
    return std::unique_ptr<clang::ASTConsumer>(
        new FindNamedClassConsumer(&Compiler.getASTContext()));
  }
};

int main(int argc,const char **argv) {
  CommonOptionsParser OptionsParser(argc, argv, MyToolCategory);
  ClangTool Tool(OptionsParser.getCompilations(),
                 OptionsParser.getSourcePathList());
  int result = Tool.run(newFrontendActionFactory<FindNamedClassAction>().get());
  //rewriter.getEditBuffer(rewriter.getSourceMgr().getMainFileID()).write(errs());
  return result;
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
}
