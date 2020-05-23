/*
TODO: pending problems:
1, bitfield. answer: refer to clang.
2, alignment? seems this is not a problem, but, how to dump dataLayout?
*/


#include "stdafx.h"

#include "toy_module.h"

#include "toy_memory.h"
 
//#include "llvm/support/alignment.h"

using namespace llvm;
using namespace std;

namespace toyc
{

///////////////////////////////////////////////////////////////////////////////////////////////////
//									APIs
//
llvm::GlobalVariable* new_global_variable(llvm::Module& M, const char* name, llvm::Type* ty, 
    llvm::GlobalValue::LinkageTypes linkage, int align, llvm::Constant* initVal)
{
    llvm::GlobalVariable* g = M.getNamedGlobal(name);
    if (g) {
        errs() << "FATAL ERROR(new_global_variable): duplicated names\n";
        abort();
        return nullptr;
    }
    M.getOrInsertGlobal(name, ty);
    g = M.getNamedGlobal(name);
    g->setLinkage(linkage);
    g->setAlignment(MaybeAlign(align));
    g->setInitializer(initVal);
    //g->setConstant(true);

    return g;
}

void delete_global_variable(llvm::Module& M, llvm::GlobalVariable* g)
{
    if (g) {
        g->replaceAllUsesWith(UndefValue::get(g->getType()));
        g->eraseFromParent();
    }
}

uint64_t getElementOffset(llvm::Module&M, llvm::StructType* st, uint32_t index)
{
    return M.getDataLayout().getStructLayout(st)->getElementOffset(index);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//									GlobalVariable
//
CGlobalVariables::CGlobalVariables()
{
    //TODO: print symbol table.
/*
ExternalLinkage 	Externally visible function.
AvailableExternallyLinkage 	Available for inspection, not emission.
LinkOnceAnyLinkage 	Keep one copy of function when linking (inline)
LinkOnceODRLinkage 	Same, but only replaced by something equivalent.
WeakAnyLinkage 	Keep one copy of named function when linking (weak)
WeakODRLinkage 	Same, but only replaced by something equivalent.
AppendingLinkage 	Special purpose, only applies to global arrays.
InternalLinkage 	Rename collisions when linking (static functions).
PrivateLinkage 	Like Internal, but omit from symbol table.
ExternalWeakLinkage 	ExternalWeak linkage description.
CommonLinkage 	Tentative definitions.
*/
    linkage = GlobalValue::InternalLinkage;
    align = 1;
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

//NOTE: create() crashes, if there is any mismatch of data type in ty and initVal.
llvm::GlobalVariable* CGlobalVariables::create(const char* name, llvm::Type* ty, llvm::Constant* initVal)
{
    llvm::GlobalVariable* g = new_global_variable(*M, name, ty, linkage, align, initVal);
    map[name] = g;
    return g;
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

llvm::GlobalVariable* CGlobalVariables::create_struct_array(const char* name, llvm::StructType* stru, int n, llvm::Constant* initVal)
{
    return create(name, llvm::ArrayType::get(stru, n), initVal);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//									CStruct
//
void CStruct::push_struct_array(llvm::Module& M, llvm::StructType* stru, int n)
{
    members.push_back(ArrayType::get(stru, n));
}

void CStruct::push_struct(llvm::Module& M, llvm::StructType* stru)
{
    members.push_back(stru);
}

void CStruct::push_struct_pointer(llvm::Module& M, llvm::StructType* stru)
{
    members.push_back(llvm::PointerType::get(stru, 0));
}

void CStruct::setBody(bool isPacked)
{
    stru->setBody(members, isPacked);
}

void CStruct::print(void)
{
    errs() << stru << "\n";
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//									test
//
void test_toy_memory_packed(llvm::Module& M)
{
    const DataLayout& layout = M.getDataLayout();

    CStruct cs;

    /*
    struct stru{
        unsigned char m:3;
        unsigned char n:5;
        unsigned char u;
    }s;
    */

    cs.push<unsigned char>(M, 3);
    cs.push<unsigned char>(M, 5);
    cs.push<unsigned char>(M, 8);

    StructType* const llvm_S = cs.create(M, "test packed", true);
    //cs.setBody();

    errs() << "allocSize = " << layout.getTypeAllocSize(llvm_S) << "\n";
    llvm_S->print(errs());

    errs() << "\n";

    errs() << "getElementOffset:" << getElementOffset(M, llvm_S, 0) << getElementOffset(M, llvm_S, 1) 
        << getElementOffset(M, llvm_S, 2) << "\n";
}

void test_toy_memory(llvm::Module&M)
{
    const DataLayout& layout = M.getDataLayout();

    CStruct cs;
    StructType* const llvm_S = cs.create(M, "test");

    cs.push_scalar_pointer<float>(M);
    cs.push_scalar_pointer<int>(M);
    cs.push_scalar_pointer<short>(M);

    cs.push_struct_pointer(M, llvm_S);
    cs.push_struct(M, llvm_S);

    cs.push<int>(M, 8);
    cs.push<float>(M);
    cs.push<int>(M, 16);
    cs.push<double>(M);

    cs.push_scalar_array<int>(M, 16, 2);
    cs.push_scalar_array<float>(M, 16, 2);
    cs.push_scalar_array<double>(M, 16, 2);
    cs.push_struct_array(M, llvm_S, 3);

    cs.setBody();

    errs() << "allocSize = " << layout.getTypeAllocSize(llvm_S) << "\n";
    llvm_S->print(errs());

    errs() << "\n";

/*
result:
allocSize = 480
%test = type { float*, i32*, i16*, %test*, %test, i8, float, i16, double, [2 x i16], [2 x float], [2 x double], [3 x %test] }
*/
}

void test_global_variable(CGlobalVariables& gVars, llvm::Module& M)
{
    gVars.setModule(&M);

    gVars.create_array<char>("i8", 16);
    gVars.create<short>("i16");
    gVars.create<int>("i32");
    gVars.create<long long>("i64");

    CStruct cs;
    cs.push<int>(M, 8);
    StructType* stru = cs.create(M, "test abc");
    cs.setBody();

    gVars.create_struct_array("ta", stru, 10);

    errs() << "sint = " << sizeof(int) << "\n";

    dump_module_globals(M);
    errs() << "\n";

/*
result:
[16 x i8]* i8
i16* i16
i32* i32
i64* i64
[10 x %"test abc"]* ta
*/
}


};      //end of namespace

