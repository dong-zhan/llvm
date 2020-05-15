//
// 
//

#include "stdafx.h"

#include "toy_memory.h"
 
//#include "llvm/support/alignment.h"

using namespace llvm;
using namespace std;

namespace toyc
{

llvm::GlobalVariable* new_global_variable(llvm::Module& M, const char* name, llvm::Type* ty, 
    llvm::GlobalValue::LinkageTypes linkage, int align)
{
    M.getOrInsertGlobal(name, ty);
    llvm::GlobalVariable* g = M.getNamedGlobal(name);
    g->setLinkage(linkage);
    g->setAlignment(MaybeAlign(align));

    return g;
}

void delete_global_variable(llvm::Module& M, llvm::GlobalVariable* g)
{
    if (g) {
        g->replaceAllUsesWith(UndefValue::get(g->getType()));
        g->eraseFromParent();
    }
}

void CStruct::push_struct_pointer(llvm::Module& M, llvm::StructType* stru)
{
    members.push_back(llvm::PointerType::get(stru, 0));
}

void CStruct::setBody(void)
{
    stru->setBody(members);
}

void CStruct::print(void)
{ 
    errs() << stru << "\n"; 
}

CGlobalVariables::CGlobalVariables()
{
    linkage = GlobalValue::CommonLinkage;
    align = 0;
}

void CGlobalVariables::setModule(llvm::Module* M)
{
    this->M = M; 
}
void CGlobalVariables::setLinkage(llvm::GlobalValue::LinkageTypes linkage) 
{ 
    this->linkage = linkage; 
}
void CGlobalVariables::setAlign(int align)
{ 
    this->align = align; 
}

void CGlobalVariables::create(const char* name, llvm::Type* ty)
{
    map[name] = new_global_variable(*M, name, ty, linkage, align);
}

void CGlobalVariables::destroy(const char* name)
{
    delete_global_variable(*M, map[name]);
    map.erase(name);
}

llvm::GlobalVariable* CGlobalVariables::find(const char* name)
{
    return map[name];
}


void test_toy_memory(llvm::Module&M)
{
    const DataLayout& layout = M.getDataLayout();

    CStruct cs;
    cs.push_scalar_pointer<float>(M);
    cs.push_scalar_pointer<int>(M);
    cs.push_scalar_pointer<short>(M);

    StructType* const llvm_S = cs.create(M, "test");
    cs.push_struct_pointer(M, llvm_S);

    cs.push<int>(M, 8);
    cs.push<float>(M);
    cs.push<int>(M, 16);
    cs.push<double>(M);

    cs.setBody();

    errs() << "allocSize = " << layout.getTypeAllocSize(llvm_S) << "\n";
    llvm_S->print(errs());

    errs() << "\n";

/*
result:
allocSize = 56
%test = type { float*, i32*, i16*, %test*, i8, float, i16, double }
*/
}


};
