#ifndef __INCLUDE_TOY_CONSTANT__
#define __INCLUDE_TOY_CONSTANT__

#include <map>
#include <vector>

#include "llvm/IR/Value.h"
#include "llvm/IR/GlobalValue.h"
//#include "llvm/IR/IRBuilder.h"

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
