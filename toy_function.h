#ifndef __INCLUDE_TOY_FUNCTION__
#define __INCLUDE_TOY_FUNCTION__

#include <map>
#include <vector>

#include "llvm/IR/Value.h"
#include "llvm/IR/GlobalValue.h"

namespace toyc
{

class CFunction
{
protected:
	llvm::Type* retType;
	std::vector<llvm::Type*> argTypes;
	std::vector<std::string> argNames;

public:
	void setRetType(llvm::Type* ty);
	void pushArg(llvm::Type*, const std::string& name);
	llvm::Function* create(llvm::Module& m, llvm::GlobalValue::LinkageTypes linkage, const char* fn);

public:
	llvm::Function* find(llvm::Module& M, const char* fn);
};

llvm::Function* create_function(llvm::Module& m, llvm::GlobalValue::LinkageTypes linkage, const char* fn,
	llvm::Type* retType, std::vector<llvm::Type*>& argTypes, std::string* argNames);

}; //end of namespace

#endif
