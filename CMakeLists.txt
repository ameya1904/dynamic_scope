set(LLVM_LINK_COMPONENTS support) 
set(LLVM_USED_LIBS clangTooling clangBasic clangAST)


add_clang_executable(scope dynscope.cpp)
target_link_libraries(scope clangTooling clangBasic clangASTMatchers clangFrontend 
			clangSerialization clangDriver clangTooling clangParse clangSema
           clangAnalysis clangRewriteFrontend 
	   clangEdit clangAST clangLex clangBasic)
