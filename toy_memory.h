#ifndef __INCLUDE_TOY_MEMORY__
#define __INCLUDE_TOY_MEMORY__

#include <map>
#include <vector>

#include "llvm/IR/Value.h"

namespace toyc
{

//TODO: how to create packed struct in IR?
class CStruct
{
protected:
	std::vector<llvm::Type*> members;
	llvm::StructType* stru;

public:		//debug
	void print(void);

public:
	void push_struct_pointer(llvm::Module& M, llvm::StructType* stru);
	template<typename T>
	void push_scalar_pointer(llvm::Module& M) {
		members.push_back(llvm::PointerType::get(llvm::Type::getScalarTy<T>(M.getContext()), 0));
	}
	template<typename T>
	void push(llvm::Module& M, int bits = 8) {
		llvm::Type* ty = llvm::Type::getScalarTy<T>(M.getContext());
		switch (ty->getTypeID()) {
		case llvm::Type::IntegerTyID:
			members.push_back(llvm::IntegerType::get(M.getContext(), bits));
			break;
		case llvm::Type::FloatTyID:
			members.push_back(llvm::Type::getFloatTy(M.getContext()));
			break;
		case llvm::Type::DoubleTyID:
			members.push_back(llvm::Type::getDoubleTy(M.getContext()));
			break;
		default:
			assert(0 && "not implemented");
			break;
		}
		//members.push_back(Type::getScalarTy<T>(M.getContext(), bits));
	}
	llvm::StructType* create(llvm::Module& M, const char* name) {
		stru = llvm::StructType::create(M.getContext(), name);
		return stru;
	}
	void setBody(void); 
};


llvm::GlobalVariable* new_global_variable(llvm::Module& M, const char* name, llvm::Type* ty, 
	llvm::GlobalValue::LinkageTypes linkage = llvm::GlobalValue::CommonLinkage, int align = 0);
void delete_global_variable(llvm::Module& M, llvm::GlobalVariable*g);

class CGlobalVariables
{
protected:
	llvm::Module* M;
	llvm::GlobalValue::LinkageTypes linkage;
	std::map<std::string, llvm::GlobalVariable*> map;
	int align;

public:
	CGlobalVariables();
	void setModule(llvm::Module* M);
	void setLinkage(llvm::GlobalValue::LinkageTypes linkage);
	void setAlign(int align);

public:
	template<class T>void create(const char* name){create(name, Type::getScalarTy<T>(M->getContext()));}
	void create(const char* name, llvm::Type* ty);
	void destroy(const char* name);
	llvm::GlobalVariable* find(const char* name);
};


void test_toy_memory(llvm::Module& M);

};

#endif
