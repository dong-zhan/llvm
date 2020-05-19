#ifndef __INCLUDE_TOY_BUILDER__
#define __INCLUDE_TOY_BUILDER__

#include <map>
#include <vector>

#include "llvm/IR/Value.h"
#include "llvm/IR/IRBuilder.h"

#include "toy_basicblock.h"
#include "toy_constant.h"

namespace toyc {

class CBuilder
{
public:
	inline static llvm::Value* CreateIntToPtr(llvm::IRBuilder<>& Builder, llvm::LLVMContext& context, const float* p) {
		return Builder.CreateIntToPtr(CConstant::getPointer(context, (void*)p), llvm::Type::getFloatPtrTy(context));
	}
	inline static llvm::Value* CreateIntToPtr(llvm::IRBuilder<>& Builder, llvm::LLVMContext& context, const double* p) {
		return Builder.CreateIntToPtr(CConstant::getPointer(context, (void*)p), llvm::Type::getDoublePtrTy(context));
	}
};


}; //end of namespace

#endif