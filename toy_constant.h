#ifndef __INCLUDE_TOY_CONSTANT__
#define __INCLUDE_TOY_CONSTANT__

#include <map>
#include <vector>

#include "llvm/IR/Value.h"
#include "llvm/IR/GlobalValue.h"
//#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Constants.h"

namespace toyc
{

///////////////////////////////////////////////////////////////////////////////////////////////////
//									CConstant
//
class CConstant
{

public:
	static inline llvm::Constant* getInt8(llvm::LLVMContext& Context, char c) {
		return llvm::Constant::getIntegerValue(llvm::Type::getInt8Ty(Context), llvm::APInt(8, c));
	}
	static inline llvm::Constant* getInt16(llvm::LLVMContext& Context, short c) {
		return llvm::Constant::getIntegerValue(llvm::Type::getInt16Ty(Context), llvm::APInt(16, c));
	}
	static inline llvm::Constant* getInt32(llvm::LLVMContext& Context, int c) {
		return llvm::Constant::getIntegerValue(llvm::Type::getInt32Ty(Context), llvm::APInt(32, c));
	}
	static inline llvm::Constant* getInt64(llvm::LLVMContext& Context, long long c) {
		return llvm::Constant::getIntegerValue(llvm::Type::getInt64Ty(Context), llvm::APInt(64, c));
	}

	static inline llvm::Constant* get_string(llvm::Module& M, const char* str, bool addNull = true) {
		return llvm::ConstantDataArray::getString(M.getContext(), str, addNull);
	}


	///////////////////////////////////////////////////////////////////////
	//				bool
	//
	static inline llvm::Constant* getBool(llvm::LLVMContext& Context, bool b) {
		return llvm::Constant::getIntegerValue(llvm::Type::getInt1Ty(Context), llvm::APInt(1, b));
	}


	///////////////////////////////////////////////////////////////////////
	//				float point
	//
	static inline llvm::Constant* getFloat(llvm::LLVMContext& Context, float f) {
		return llvm::ConstantFP::get(llvm::Type::getFloatTy(Context), llvm::APFloat(f));
	}
	static inline llvm::Constant* getDouble(llvm::LLVMContext& Context, double f) {
		return llvm::ConstantFP::get(llvm::Type::getDoubleTy(Context), llvm::APFloat(f));
	}

	///////////////////////////////////////////////////////////////////////
	//				pointer
	//
	static inline llvm::ConstantInt* getPointer(llvm::LLVMContext& Context, void* p) {
		return llvm::ConstantInt::get(Context, llvm::APInt(64, reinterpret_cast<uint64_t>(p)));
	}


	//static inline llvm::ConstantPointerNullVal();

/*
	static inline llvm::PointerType* getInt32Ptr(llvm::LLVMContext& Context, int *c) {
		return llvm::PointerType::get(llvm::Type::getInt32Ty(Context), 0);
	}
*/

	//static inline llvm::Constant* getFloatPtr(llvm::LLVMContext& Context, float* fp) {
	//	return llvm::ConstantFP::get(llvm::Type::getInt64Ty(Context), llvm::APInt(64, c));
	//}
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//									CConstantStructValue
//
class CConstantStructValue
{
protected:
	std::vector<llvm::Constant*> members;
	llvm::Constant* stru;

public:
	//push executable space variable, so, push_pointer() can be used only in JIT, use push(GlobalVariable) to push module space variable.
	void push_pointer(llvm::Module& M, void* p) {
		members.push_back(CConstant::getPointer(M.getContext(), p));
	}
	//push module space variable.
	void push(llvm::Module& M, llvm::GlobalVariable* g) {
		members.push_back(g);
	}
	//push string
	void pushExpr(llvm::Module& M, llvm::Type* ty, std::vector<llvm::Value*>&indices, llvm::GlobalVariable* g) {
		members.push_back(llvm::ConstantExpr::getGetElementPtr(ty, g, indices));
	}
	llvm::Constant* get_string(llvm::Module& M, const char* str, bool addNull = true) {
		return llvm::ConstantDataArray::getString(M.getContext(), str, addNull);
	}
	llvm::Constant* get_null_initializer(llvm::Type* ty) {
		return llvm::Constant::getNullValue(ty);
	}
	void push_null_pointer(llvm::Module& M, llvm::Type*ty) {
		members.push_back(llvm::ConstantPointerNull::get(llvm::PointerType::get(ty, 0)));
	}
	template<typename T>
	void push(llvm::Module& M, const T& value, int bits = 8) {
		llvm::Type* ty = llvm::Type::getScalarTy<T>(M.getContext());
		switch (ty->getTypeID()) {
		case llvm::Type::IntegerTyID:
			members.push_back(llvm::ConstantInt::get(ty, llvm::APInt(bits, value)));
				break;
		case llvm::Type::FloatTyID:
			members.push_back(llvm::ConstantFP::get(M.getContext(), llvm::APFloat((float)value)));
			break;
		case llvm::Type::DoubleTyID:
			members.push_back(llvm::ConstantFP::get(M.getContext(), llvm::APFloat((double)value)));
			break;
		default:
			assert(0 && "not implemented");
			break;
		}
		//members.push_back(Type::getScalarTy<T>(M.getContext(), bits));
	}

	llvm::Constant* create(llvm::StructType* st) {
		stru = llvm::ConstantStruct::get(st, members);
		return stru;
	}
};


}; //end of namespace


#endif