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
  
 /* bool VisitCXXRecordDecl(CXXRecordDecl *rd) {
      FullSourceLoc FullLocation = Context->getFullLoc(Declaration->getLocStart());
      if (FullLocation.isValid()){
      
      //`  rewriter.InsertTextBefore(Declaration->getLocStart() , "\/* inserted text *\/");
      }                 
    }
    return true;
  }*/
  
  
	bool VisitFunctionDecl(FunctionDecl *f) {
		SourceManager &SM = rewriter.getSourceMgr();
		stringstream buf;
		if (f->isThisDeclarationADefinition()) {
			string fname;
			fname.assign(f->getQualifiedNameAsString());
			FullSourceLoc loc = Context->getFullLoc (f->getLocStart() );
      		//	llvm::outs()<<f->getQualifiedNameAsString()<<" declared at " << loc.getSpellingLineNumber()<<":"<<loc.getSpellingColumnNumber();
			/*unsigned num_param =f->getNumParams(); 
			outs()<<".Number of parameters = "<<num_param<<"\n";
			if (num_param > 0 ){
				ArrayRef< ParmVarDecl * > param_array=f->parameters();
				for(unsigned i=0;i<num_param;i++){
					//outs()<<"\t"<<param_array[i]->getNameAsString()<<"\t";
					QualType type=param_array[i]->getOriginalType();
					//outs()<<type.getAsString()<<"\n";
				}
			}
			Stmt *body = f->getBody();
			//body->dumpColor();
			clang::Stmt::child_iterator begin= body->child_begin();
			clang::Stmt::child_iterator end= body->child_end();
			while(begin != end){


				std::string stmt_type=begin->getStmtClassName();
				//cout<<a<<"\n";
				if (stmt_type.compare("DeclStmt")==0 ){
					
		
				}	
				begin++;
			}
			*/
			struct func_location object;
			object.name.assign(f->getQualifiedNameAsString());
			object.sl=(f->getBody())->getLocStart();
			object.sl=(object.sl).getLocWithOffset(1);
			object.return_type.assign(QualType::getAsString(f->getReturnType().split()));
			func_loc.push_back(object);
		}
		return true;
	}
	
	int get_size(VarDecl* vd){
		if (auto t =  dyn_cast_or_null<ConstantArrayType>(vd->getType().getTypePtr())) {
                	llvm::APInt size= t->getSize();            
                        return size.getLimitedValue();
                } 
		else
			return 1;
	}

	void func_list_create(VarDecl* vd){
		DeclContext* dc=vd->getParentFunctionOrMethod();
		if(auto fd=dyn_cast_or_null<FunctionDecl>(dc)){
			string func_name;
			func_name.assign(fd->getQualifiedNameAsString());
			//outs()<<vd->getQualifiedNameAsString()<<"\t"<<fd->getQualifiedNameAsString()<<"\n";
			if( fl_exists( func_name ) ){
				int index=get_fl_index(func_name);
				func_list[index].lvar_count++;
			}
			else{
				struct function_list object;
				object.name.assign(func_name);
				object.lvar_count=1;
				func_list.push_back(object);
			}
		}
		return;
	}

	bool VisitVarDecl(VarDecl* vd){
		stringstream buf;
		bool is_new;
		SourceManager &SM = rewriter.getSourceMgr();		
		FullSourceLoc loc = Context->getFullLoc (vd->getLocStart() );
		if(is_new=var_exists(vd->getQualifiedNameAsString())){
				//inserting globa_storage here.
				struct global_storage object;
				object.name.assign(vd->getQualifiedNameAsString());
				glob_stor.push_back(object);;
			}
	
		if(!SM.isInSystemHeader(loc) && !vd->hasGlobalStorage() && !vd->isLocalVarDeclOrParm () || vd->isLocalVarDecl ()){ 
			//llvm::outs()<<vd->getQualifiedNameAsString()<<"\t"<<QualType::getAsString(vd->getType().split())<<"\t"<<get_size(vd)<<"\t" <<loc.getSpellingLineNumber()<<":"<<loc.getSpellingColumnNumber()<<"\n";
			func_list_create(vd);
					for(vector<global_storage>::iterator i=glob_stor.begin();i!=glob_stor.end();i++){
				//outs()<<i->name<<"\n";
			}
			buf<<"if(add_node("<<get_index(vd->getQualifiedNameAsString())<<","<<get_type_enc(vd)<<","<<get_size(vd)<<",&ltb[l_varcount])==1)\n";
			buf<<"\ttop=na_push(top,"<<get_index(vd->getQualifiedNameAsString())<<");\n";
			buf<<"else\n\tl_varcount++;\n";
			/*

			find out how to get initilaization of variable.
			handle if initialization is an expr.


			*/
			rewriter.InsertText(vd->getLocStart(),buf.str(),true,true);
			var_count++;
		}
		if(vd->hasGlobalStorage()){
			glob_loc.push_back(vd);
		}
		return true;	
	}

	int get_type_enc(VarDecl* vd){
	         	const Type* type=(vd->getType()).getTypePtr();
			if (type->isIntegerType()){
				return 0;
			}
			else if(type->isRealFloatingType()){
				return 1;
			}
			else if(type->isConstantArrayType()){
				auto t =  dyn_cast_or_null<ArrayType>(type);
				const Type* arr_type= (t->getElementType()).getTypePtr();
				if (arr_type->isIntegerType()){
					return 2;
				}	
				else if(arr_type->isRealFloatingType()){
					return 3;
				}
			}
			return -99;
	         } 
	
	int get_fl_index(string str){
		int index=0;
		bool match=false;
		for (vector<function_list>::iterator i=func_list.begin();i!=func_list.end() && !match ;i++){
                         if(str.compare(i->name)==0){
                         	match=true;
 	                 }
			 index++;
                }
		return index-1;
	}
	
	bool fl_exists(string str){
		bool ret=false;
		for (vector<function_list>::iterator i=func_list.begin();i!=func_list.end();i++){
			if(str.compare(i->name)==0){
				ret=true;
			}
		}
		return ret;
	}	

	int get_index(string str){
		int index=0;
		bool match=false;
		for (vector<global_storage>::iterator i=glob_stor.begin();i!=glob_stor.end() && !match ;i++){
                         if(str.compare(i->name)==0){
                         	match=true;
 	                 }
			 index++;
                }
		return index-1;
	}
	
	bool var_exists(string str){
		bool ret=true;
		for (vector<global_storage>::iterator i=glob_stor.begin();i!=glob_stor.end();i++){
			if(str.compare(i->name)==0){
				ret=false;
			}
		}
		return ret;
	}
	~FindNamedClassVisitor(){
		SourceLocation main_loc;
		for(vector <func_location>::iterator i=func_loc.begin();i!=func_loc.end();i++){
			stringstream buf;
			buf<<"\nstruct new_added* top=NULL;\nint l_varcount=0;\n"<<i->return_type<<" ret;\n";
			int index=get_fl_index(i->name);
			buf<<"struct ltable ltb["<<func_list[index].lvar_count <<"];\n";
			rewriter.InsertText(i->sl,buf.str(),true,true);
			if(i->name.compare("main")==0)
				main_loc=i->sl;
		}
		for(vector <VarDecl*>::iterator i=glob_loc.begin();i!=glob_loc.end();i++){
			stringstream buf;
			buf<<"if(add_node("<<get_index( (*i)->getQualifiedNameAsString())<<","<<get_type_enc((*i))<<","<<get_size((*i))<<",&ltb[l_varcount])==1)\n";
			buf<<"\ttop=na_push(top,"<<get_index((*i)->getQualifiedNameAsString())<<");\n";
			buf<<"else\n\tl_varcount++;\n";
			rewriter.InsertText(main_loc,buf.str(),true,true);
		}	

	}
private:
  ASTContext *Context;
  int var_count=0;
  struct global_storage{
	string name;
   	void* gen_ptr;
 	int type;
	int size;
  };
  struct function_list{
  	string name;
	string return_type;
	int lvar_count;
  };
  struct func_location{
  	string name;
	SourceLocation sl;
	string return_type;
  };

  vector <VarDecl*> glob_loc;
  vector <struct func_location>func_loc;
  vector <struct function_list>func_list;
  vector <struct global_storage> glob_stor ;
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
  rewriter.getEditBuffer(rewriter.getSourceMgr().getMainFileID()).write(errs());
  return result;
}
