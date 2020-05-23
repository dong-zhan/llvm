#ifndef __INCLUDE_TOY_MEMORY__
#define __INCLUDE_TOY_MEMORY__

#include <map>
#include <vector>

#include "llvm/IR/Value.h"
#include "llvm/IR/GlobalValue.h"

namespace toyc
{

///////////////////////////////////////////////////////////////////////////////////////////////////
//									CStruct
//
class CStruct
{
protected:
	std::vector<llvm::Type*> members;
	llvm::StructType* stru;

public:		//debug
	void print(void);

public:
	void push_struct_array(llvm::Module& M, llvm::StructType* stru, int n);

	template<typename T>
	void push_scalar_array(llvm::Module& M, int bits, int n) {
		llvm::Type* ty = llvm::Type::getScalarTy<T>(M.getContext());
		switch (ty->getTypeID()) {
		case llvm::Type::IntegerTyID:
			members.push_back(llvm::ArrayType::get(llvm::IntegerType::get(M.getContext(), bits), n));
			break;
		case llvm::Type::FloatTyID:
			members.push_back(llvm::ArrayType::get(llvm::Type::getFloatTy(M.getContext()), n));
			break;
		case llvm::Type::DoubleTyID:
			members.push_back(llvm::ArrayType::get(llvm::Type::getDoubleTy(M.getContext()), n));
			break;
		default:
			assert(0 && "not implemented");
			break;
		}
	}

	void push_struct_pointer(llvm::Module& M, llvm::StructType* stru);
	void push_struct(llvm::Module& M, llvm::StructType* stru);

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

	llvm::StructType* create(llvm::Module& M, const char* name, bool isPacked = false) {
		if (isPacked) {
			stru = llvm::StructType::create(M.getContext(), members, name, isPacked);
		}
		else {
			stru = llvm::StructType::create(M.getContext(), name);
		}
		return stru;
	}

	void setBody(bool isPacked = false);
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//									APIs
//
llvm::GlobalVariable* new_global_variable(llvm::Module& M, const char* name, llvm::Type* ty, 
	llvm::GlobalValue::LinkageTypes linkage = llvm::GlobalValue::CommonLinkage, int align = 0, llvm::Constant* initVal = nullptr);
void delete_global_variable(llvm::Module& M, llvm::GlobalVariable*g);

uint64_t getElementOffset(llvm::Module&mod, llvm::StructType* st, uint32_t index);

///////////////////////////////////////////////////////////////////////////////////////////////////
//									CGlobalVariables
//
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
	static llvm::GlobalVariable* create_string(llvm::Module&M, const char* name, const char* str,
		llvm::GlobalValue::LinkageTypes linkage = llvm::GlobalValue::PrivateLinkage, llvm::Constant* initVal = nullptr) {
		llvm::Constant* cs = llvm::ConstantDataArray::getString(M.getContext(), str, true);
		llvm::GlobalVariable* g = new_global_variable(M, name,
			llvm::ArrayType::get(llvm::Type::getScalarTy<char>(M.getContext()), strlen(str)+1),
			linkage, 1, cs);
		return g;
	}
	//struct is not created with create<struct_type>(), but, with CStruct above first, then use the created type.
	template<class T>
	llvm::GlobalVariable* create(const char* name, llvm::Constant* initVal = nullptr){
		return create(name, llvm::Type::getScalarTy<T>(M->getContext()), initVal);
	}

	llvm::GlobalVariable* create(const char* name, llvm::Type* ty, llvm::Constant* initVal = nullptr);

	template<class T>
	llvm::GlobalVariable* create_array(const char* name, int n, llvm::Constant* initVal = nullptr) {
		return create(name, llvm::ArrayType::get(llvm::Type::getScalarTy<T>(M->getContext()), n), initVal);
	}

	llvm::GlobalVariable* create_struct_array(const char* name, llvm::StructType* stru, int n, llvm::Constant* initVal = nullptr);


	void destroy(const char* name);
	llvm::GlobalVariable* find(const char* name);
};


void test_toy_memory_packed(llvm::Module& M);
void test_toy_memory(llvm::Module& M);
void test_global_variable(CGlobalVariables& gVars, llvm::Module& M);

};

#endif