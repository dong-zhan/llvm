#include "stdafx.h"

#include "toy_function.h"

using namespace llvm;
using namespace std;

namespace toyc
{
///////////////////////////////////////////////////////////////////////////////////////////////////
//									CFunction
//
void CFunction::pushArg(llvm::Type* ty, const std::string& name)
{
    argTypes.push_back(ty);
    argNames.push_back(name);
}

void CFunction::setRetType(llvm::Type* ty)
{
    retType = ty;
}

Function* CFunction::create(llvm::Module& M, llvm::GlobalValue::LinkageTypes linkage, const char* fn)
{
    FunctionType* FT = FunctionType::get(retType, argTypes, false);

    Function* F = Function::Create(FT, linkage, fn, M);

    unsigned Idx = 0;
    for (auto& Arg : F->args())
        Arg.setName(argNames[Idx++]);

    return F;
}

llvm::Function* CFunction::find(llvm::Module& M, const char* fn)
{
    return M.getFunction(fn);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//									APIs
//
llvm::Function* create_function(llvm::Module& M, llvm::GlobalValue::LinkageTypes linkage,
    const char* fn, llvm::Type* retType, std::vector<llvm::Type*>& argTypes, std::string* argNames)
{
    FunctionType* FT = FunctionType::get(retType, argTypes, false);

    Function* F = Function::Create(FT, linkage, fn, M);

    if (argNames) {
        unsigned Idx = 0;
        for (auto& Arg : F->args())
            Arg.setName(argNames[Idx++]);
    }

    return F;
}



};   //end of namespace
