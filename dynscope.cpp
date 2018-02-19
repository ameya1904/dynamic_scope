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
#include "clang/AST/RecordLayout.h"
#include "clang/Frontend/ASTConsumers.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/AST/Expr.h"
#include "clang/Basic/LLVM.h"

using namespace std;
using namespace llvm;
using namespace clang;
using namespace clang::tooling;
using namespace clang::driver;

Rewriter rewriter;
static llvm::cl::OptionCategory MyToolCategory("example options");
static cl::extrahelp CommonHelp(CommonOptionsParser::HelpMessage);
static cl::extrahelp MoreHelp("\nMore help text...");
static int rec_type=4;

class FindNamedClassVisitor : public RecursiveASTVisitor<FindNamedClassVisitor> {
	public:
		explicit FindNamedClassVisitor(ASTContext *Context)
			: Context(Context) {
				rewriter.setSourceMgr(Context->getSourceManager(),
						Context->getLangOpts());
			}


		bool VisitRecordDecl(RecordDecl *rd) {
			FullSourceLoc FullLocation = Context->getFullLoc(rd->getLocStart());
			if (FullLocation.isValid()){
				//outs()<<rd->getQualifiedNameAsString()<<"\n";
				const clang::ASTRecordLayout &typeLayout(rd->getASTContext().getASTRecordLayout(rd));
				struct rec_dict object;
				object.name.assign("struct "+rd->getQualifiedNameAsString());
				object.num_mem=0;
				for(clang::RecordDecl::field_iterator fit = rd->field_begin(); fit != rd->field_end(); fit++) {
					object.num_mem++;
					const clang::QualType qualType = fit->getType().getLocalUnqualifiedType().getCanonicalType();
					size_t fieldOffset = typeLayout.getFieldOffset(fit->getFieldIndex());
					//std::cout << "member '"<<fit->getNameAsString() << qualType.getAsString() << "' with " << fieldOffset << "bytes offset\n";
					struct rec_mem mem_object;
					const Type* type=(fit->getType()).getTypePtr();
					mem_object.name.assign(fit->getNameAsString());
					mem_object.type=get_type_enc(type,fit->getNameAsString());
					mem_object.offset=typeLayout.getFieldOffset(fit->getFieldIndex());
					object.members.push_back(mem_object);
				}
				object.type=rec_type;
				record_dict.push_back(object);
				rec_type++;
				//rewriter.InsertTextBefore(Declaration->getLocStart() , "\/* inserted text *\/");
			}                 

			return true;
		}


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
			const Type* type=(vd->getType()).getTypePtr();
			SourceManager &SM = rewriter.getSourceMgr();		
			FullSourceLoc loc = Context->getFullLoc (vd->getLocStart() );
			if(is_new=var_exists(vd->getQualifiedNameAsString()) && (vd->getQualifiedNameAsString()).size()>0 ){
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
				//outs()<<vd->getQualifiedNameAsString()<<"\t"<<get_type_enc(type,QualType::getAsString(vd->getType().split()))<<"\n";
				buf<<"if(add_node("<<get_index(vd->getQualifiedNameAsString())<<","<<get_type_enc(type,QualType::getAsString(vd->getType().split()))<<","<<get_size(vd)<<",&ltb[l_varcount])==1)\n";
				buf<<"\ttop=na_push(top,"<<get_index(vd->getQualifiedNameAsString())<<");\n";
				buf<<"else\n\tl_varcount++;\n";

				/*find out how to get initilaization of variable. handle if initialization is an expr.	*/
				if(vd->hasInit()){
					const Expr* init_expr=vd->getInit();
					ASTContext& ctx=vd->getASTContext();
					init_handler(init_expr,ctx);
					for(vector<struct init_val_rec>::iterator i=init_val.begin();i!=init_val.end();i++){
						cout<<i->type<<"\t";
						if(i->type==0)
							cout<<*(int*)(i->arr)<<"\n";
						if(i->type==1)
							cout<<*(float*)(i->arr)<<"\n";			
					}
				}	
				rewriter.InsertText(vd->getLocStart(),buf.str(),true,true);
				var_count++;
			}
			if(vd->hasGlobalStorage()){
				glob_loc.push_back(vd);
			}
			return true;	
		}

		void init_handler(const Expr* init_expr,ASTContext& ctx){
			//outs()<<"---"<<vd->getQualifiedNameAsString()<<"\n";//<<loc.getSpellingLineNumber()<<":"<<loc.getSpellingColumnNumber()<<"\n";
			//const Expr* init_expr=vd->getInit();
			QualType qtype=init_expr->getType();
			const Type *expr_type=qtype.getTypePtr();
			//cout<<qtype.getAsString()<<"\n";
			struct init_val_rec tmp;
			if(qtype.getAsString().compare("int")==0){//cout<<"int\n";
				APSInt r;
				APSInt &Result=r;
				clang::Expr::SideEffectsKind AllowSideEffects=clang::Expr::SideEffectsKind::SE_NoSideEffects;
				if(init_expr->EvaluateAsInt(Result,ctx, AllowSideEffects)){
					//cout<<"\t"<<Result.toString(10)<<"\n";
					uint64_t Limit =UINT64_MAX;
					int temp=Result.getLimitedValue(Limit);
					memcpy(tmp.arr,&temp,sizeof(int));
					tmp.type=0;
					init_val.push_back(tmp);
				}
			}
			else if(qtype.getAsString().compare("float")==0){//cout<<"float\n";
				APFloat r=APFloat(1.0);
				APFloat &Result=r;
				clang::Expr::SideEffectsKind AllowSideEffects=clang::Expr::SideEffectsKind::SE_NoSideEffects;
				if(init_expr->EvaluateAsFloat(Result,ctx, AllowSideEffects)){
					//cout<<"a\t"<<Result.convertToFloat()<<"\n";
					float temp=Result.convertToFloat();
					memcpy(tmp.arr,&temp,sizeof(float));
					tmp.type=1;			
					init_val.push_back(tmp);
				}
			}
			else if(expr_type->isConstantArrayType()){
				if(auto arr_init_expr=dyn_cast_or_null<InitListExpr>(init_expr)){
					const Type *type=arr_init_expr->getType().getTypePtr();
					ArrayRef<Expr*> array_expr= arr_init_expr->inits();
					if(get_type_enc(type,QualType::getAsString(init_expr->getType().split()))==2){//cout<<"int array\n";	
						for(ArrayRef<Expr*>::iterator i=array_expr.begin();i!=array_expr.end();i++ ){
							APSInt r;
							APSInt &Result=r;
							clang::Expr::SideEffectsKind AllowSideEffects=clang::Expr::SideEffectsKind::SE_NoSideEffects;
							if((*i)->EvaluateAsInt(Result,ctx, AllowSideEffects)){
								//cout<<"\t"<<Result.toString(10)<<"\n";
								uint64_t Limit =UINT64_MAX;
								int temp=Result.getLimitedValue(Limit);
								memcpy(tmp.arr,&temp,sizeof(int));
								tmp.type=0;
								init_val.push_back(tmp);
							}
						}
					}
					else if(get_type_enc(type,QualType::getAsString(init_expr->getType().split()))==3){//cout<<"float array\n";
						for(ArrayRef<Expr*>::iterator i=array_expr.begin();i!=array_expr.end();i++ ){
							APFloat r=APFloat(1.0);
							APFloat &Result=r;
							clang::Expr::SideEffectsKind AllowSideEffects=clang::Expr::SideEffectsKind::SE_NoSideEffects;
							if((*i)->EvaluateAsFloat(Result,ctx, AllowSideEffects)){
								//cout<<"a\t"<<Result.convertToFloat()<<"\n";
								float temp=Result.convertToFloat();
								memcpy(tmp.arr,&temp,sizeof(float));
								tmp.type=1;
								init_val.push_back(tmp);
							}

						}
					}
				}
			}
			else{
				if(auto struct_init_expr=dyn_cast_or_null<InitListExpr>(init_expr)){
					//cout<<"inside struct\n\t";				
					ArrayRef<Expr*> struct_expr= struct_init_expr->inits();	
					for(ArrayRef<Expr*>::iterator i=struct_expr.begin();i!=struct_expr.end();i++ ){
						init_handler((*i),ctx);	
					}
				}
			}

		}

		bool VisitDeclRefExpr(const DeclRefExpr *dre) {
			FullSourceLoc loc = Context->getFullLoc (dre->getLocStart() );

			//llvm::outs()<<" declared at " << loc.getSpellingLineNumber()<<":"<<loc.getSpellingColumnNumber()<<"\n";

			return true;
		}

		int get_type_enc(const Type* type,string type_name){   
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
			else{
				for(vector<rec_dict>::iterator i=record_dict.begin();i!=record_dict.end();i++){
					//outs()<<"a--\t"<<type_name<<"\t"<<i->name<<"\t"<<i->type<<"\t"<<i->num_mem<<"\n";
					/*for(vector<rec_mem>::iterator h=i->members.begin();h!=i->members.end();h++){
					  outs()<<"\t"<<h->name<<"\t"<<h->type<<"\t"<<h->offset<<"\n";
					  }*/
					if(type_name.compare(i->name)==0){
						return i->type;
					}
				}
				return -99;
			}
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
				const Type* type=((*i)->getType()).getTypePtr();
				buf<<"if(add_node("<<get_index( (*i)->getQualifiedNameAsString())<<","<<get_type_enc(type,QualType::getAsString((*i)->getType().split()))<<","<<get_size((*i))<<",&ltb[l_varcount])==1)\n";
				buf<<"\ttop=na_push(top,"<<get_index((*i)->getQualifiedNameAsString())<<");\n";
				buf<<"else\n\tl_varcount++;\n";
				rewriter.InsertText(main_loc,buf.str(),true,true);
			}
			for(vector<global_storage>::iterator i=glob_stor.begin();i!=glob_stor.end();i++){
				//outs()<<(i)->name<<"\t"<<(i)->type<<"\t"<<i->size<<"\n";   

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
		struct rec_mem{
			string name;
			int type;
			int offset;
		};
		struct rec_dict{
			string name;
			int type;
			int num_mem;
			vector<struct rec_mem> members;
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
		struct init_val_rec{
			char arr[4];
			int type;
		};
		vector<struct init_val_rec> init_val;
		vector <VarDecl*> glob_loc;
		vector <struct rec_dict> record_dict;
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
	//rewriter.getEditBuffer(rewriter.getSourceMgr().getMainFileID()).write(errs());
	return result;
}
