//references: 
//1, https://mapping-high-level-constructs-to-llvm-ir.readthedocs.io/en/latest/object-oriented-constructs/single-inheritance.html
//2, 

// pending problems/practices:
// 1, address space cast(pointer cast)
// 2, use getPointerTo




#include "stdafx.h"


#include "toy_constant.h"
#include "toy_module.h"
#include "toy_basicblock.h"
#include "toy_function.h"
#include "toy_instruction.h"
#include "toy_memory.h"
#include "toy_builder.h"


using namespace llvm;
using namespace llvm::orc;

using namespace toyc;

static LLVMContext TheContext;
static IRBuilder<> Builder(TheContext);
static std::unique_ptr<Module> TheModule;
static std::map<std::string, AllocaInst*> NamedValues;
static std::unique_ptr<legacy::FunctionPassManager> TheFPM;
static std::unique_ptr<KaleidoscopeJIT> TheJIT;
//static std::map<std::string, std::unique_ptr<PrototypeAST>> FunctionProtos;

using namespace std;

#define PP(x) errs() << #x":"; x->print(errs()); errs() << "\n";

FunctionCallee declare_malloc(void)
{
    std::vector<Type*> args;
    args.push_back(Type::getInt64Ty(TheContext));
    Type* retType = PointerType::get(Type::getVoidTy(TheContext), 0);
    FunctionType* mallocType = FunctionType::get(retType, args, false);
    FunctionCallee func = TheModule.get()->getOrInsertFunction("malloc", mallocType);
    return func;
}

FunctionCallee declare_free(void)
{
    std::vector<Type*> args;
    args.push_back(PointerType::get(Type::getVoidTy(TheContext), 0));
    Type* retType = Builder.getVoidTy();
    FunctionType* freeType = FunctionType::get(retType, args, false);
    FunctionCallee func = TheModule.get()->getOrInsertFunction("free", freeType);
    return func;
}


static void InitializeModuleAndPassManager() {
    // Open a new module.
    TheModule = std::make_unique<Module>("my cool jit", TheContext);
    TheModule->setDataLayout(TheJIT->getTargetMachine().createDataLayout());

    // Create a new pass manager attached to it.
    TheFPM = std::make_unique<legacy::FunctionPassManager>(TheModule.get());

    // Do simple "peephole" optimizations and bit-twiddling optzns.
    TheFPM->add(createInstructionCombiningPass());
    // Reassociate expressions.
    TheFPM->add(createReassociatePass());
    // Eliminate Common SubExpressions.
    TheFPM->add(createGVNPass());
    // Simplify the control flow graph (deleting unreachable blocks, etc).
    TheFPM->add(createCFGSimplificationPass());

    TheFPM->doInitialization();
}

void create_BB(void(*func)(Module&, Function*), Function* f)
{
    BasicBlock* BB = BasicBlock::Create(TheContext, "entry", f);
    Builder.SetInsertPoint(BB);

    func(*TheModule.get(), f);

    verifyFunction(*f);
    f->print(errs());
}

template<class RETT = int>
void call_function(void(*func)(Module&, Function*), Function* inputF = nullptr)
{
    //if no inputF, create a default one.
    Function* f;
    if (inputF) {
        f = inputF;
    }
    else {
        CFunction function;
        if (sizeof(RETT) == 1) {
            function.setRetType(Type::getInt8Ty(TheContext));
        }
        else if (sizeof(RETT) == 2) {
            function.setRetType(Type::getInt16Ty(TheContext));
        }
        else if (sizeof(RETT) == 4) {
            function.setRetType(Type::getInt32Ty(TheContext));
        }
        else {
            function.setRetType(Type::getInt64Ty(TheContext));
        }
        f = function.create(*TheModule.get(), Function::ExternalLinkage, "tf1");
    }

    // create bb for f
    BasicBlock* BB = BasicBlock::Create(TheContext, "entry", f);
    Builder.SetInsertPoint(BB);

    //
    func(*TheModule.get(), f);

    verifyFunction(*f);
    f->print(errs());

    auto H = TheJIT->addModule(std::move(TheModule));

    InitializeModuleAndPassManager();
    auto ExprSymbol = TheJIT->findSymbol("tf1");
    RETT(*FP)() = (RETT(*)())(intptr_t)cantFail(ExprSymbol.getAddress());
    //printf("function address = %p\n", FP);
    RETT c = FP();
    if (sizeof(RETT) == 1) {
        printf("output: 0x%x'%c'\n", c, c);
    }
    else if (sizeof(RETT) == 4 || sizeof(RETT) == 2) {
        printf("output: 0x%x\n", c);
    }
    else {
        printf("output: 0x%p\n", c);
    }

    printf("result: %f, %f\n", gFloat, gDouble);   //result of createBB_external_variables

    TheJIT->removeModule(H);
}

llvm::GlobalVariable* create_global_variable(llvm::Module& M, const char* name, llvm::Type* ty, 
    llvm::GlobalValue::LinkageTypes linkage = GlobalValue::CommonLinkage, int align = 0)  //CommonLinkage ExternalLinkage
{
    //test_toy_memory(M);
    //test_toy_memory_packed(M);

    CGlobalVariables gVars;
    gVars.setModule(&M);
    return gVars.create<double>(name);
}

/// CreateEntryBlockAlloca - Create an alloca instruction in the entry block of
/// the function.  This is used for mutable variables etc.
static AllocaInst* CreateEntryBlockAlloca(Type* ty,  Function* TheFunction, StringRef VarName) {
    BasicBlock& eb = TheFunction->getEntryBlock();
    //use a new builder in order not to change the old insert point? Builder.saveIp() not a good choice?
    IRBuilder<> TmpB(&eb, eb.begin());
    return TmpB.CreateAlloca(ty, nullptr, VarName);
}

llvm::GlobalVariable* g;


void createAddBB(Function* f)
{
    Value* vs[2];
    int idx = 0;
    for (auto& Arg : f->args()) {
        vs[idx++] = &Arg;
    }

    Value* Inc = Builder.CreateFAdd(vs[0], vs[1]);
    Builder.CreateRet(Inc);
}

//////////////////////////////////////////////////////////////////////////
//                      test_global_variables
// 
void test_global_variables(void)
{
    g = create_global_variable(*TheModule.get(), "g", Type::getDoubleTy(TheContext));

    CFunction function;
    function.pushArg(Type::getDoubleTy(TheContext), "x");
    function.pushArg(Type::getDoubleTy(TheContext), "y");
    function.setRetType(Type::getDoubleTy(TheContext));
    Function* f = function.create(*TheModule.get(), Function::ExternalLinkage, "tf1");

    createAddBB(f);

    auto H = TheJIT->addModule(std::move(TheModule));
    InitializeModuleAndPassManager();
    auto ExprSymbol = TheJIT->findSymbol("tf1");
    double (*FP)(double, double) = (double (*)(double, double))(intptr_t)cantFail(ExprSymbol.getAddress());
    fprintf(stderr, "Evaluated to %f\n", FP(1234, 5678));
    TheJIT->removeModule(H);
}

//////////////////////////////////////////////////////////////////////////
//                      test_call_external_function
// 
#ifdef _WIN32
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT
#endif

extern "C" DLLEXPORT double putchard(char c) {
    fputc(c, stderr);
    return 0;
}

void test_call_external_function(void)
{
    CFunction function;
    function.pushArg(Type::getInt8Ty(TheContext), "c");
    function.setRetType(Type::getDoubleTy(TheContext));
    /*Function* f = */function.create(*TheModule.get(), Function::ExternalLinkage, "putchard");

    auto H = TheJIT->addModule(std::move(TheModule));

    InitializeModuleAndPassManager();
    auto ExprSymbol = TheJIT->findSymbol("putchard");
    double (*FP)(char) = (double (*)(char))(intptr_t)cantFail(ExprSymbol.getAddress());
    for(int i=0;i<26;i++)FP('a'+i);
    errs() << "\n";

    TheJIT->removeModule(H);
}

//////////////////////////////////////////////////////////////////////////
//                      create_SingleInheritance 
// 
class SIBase {
public:
    int a;
public:
    void set(int a) { this->a = a; }
};

class SIDerived : public SIBase {
public:
    int a;
public:
    void setAll(int a) { 
        SIBase::set(a);
        this->a = a; 
    }
};

SIDerived siDerived;
StructType* sty_SIBase, * sty_SIDerived;

/*
define void @tc_sib_constructor(%SIBase* %this) {
entry:
  %0 = getelementptr %SIBase, %SIBase* %this, i32 0, i32 0
  store i32 999, i32* %0, align 4
  ret void
}
*/
void createBB_tc_sib_constructor(Module& M, Function* f)
{
    Argument* pArgThis = f->getArg(0);

    std::vector<Value*>indices;
    indices.push_back(ConstantInt::get(Type::getInt32Ty(TheContext), 0));
    indices.push_back(ConstantInt::get(Type::getInt32Ty(TheContext), 0));
    Value* gep = Builder.CreateGEP(pArgThis->getType()->getPointerElementType(), pArgThis, indices);        //crashes solve: forgot to setbody()    

    Builder.CreateStore(CConstant::getInt32(M.getContext(), 111), gep);

    Builder.CreateRetVoid();
}

/*
define void @tc_sid_constructor(%SIDerived* %this) {
entry:
  %0 = getelementptr %SIDerived, %SIDerived* %this, i32 0, i32 1
  store i32 999, i32* %0, align 4
  ret void
}*/
void createBB_tc_sid_constructor(Module& M, Function* f)
{
    Argument* pArgThis = f->getArg(0);

    std::vector<Value*>indices;
    indices.push_back(ConstantInt::get(Type::getInt32Ty(TheContext), 0));
    indices.push_back(ConstantInt::get(Type::getInt32Ty(TheContext), 1));
    Value* gep = Builder.CreateGEP(pArgThis->getType()->getPointerElementType(), pArgThis, indices);        //crashes solve: forgot to setbody()    

    Builder.CreateStore(CConstant::getInt32(M.getContext(), 222), gep);

    Builder.CreateRetVoid();
}

/*
define void @tc_sib_set(%SIBase* %this, i32 %a) {
entry:
  %0 = getelementptr %SIBase, %SIBase* %this, i32 0, i32 0
  store %SIBase* %this, i32* %0, align 8
  ret void
}
*/
void createBB_tc_f_tc_sib_set(Module& M, Function* f)
{
    Argument* pArgThis = f->getArg(0);
    Argument* pArgAge = f->getArg(1);

    std::vector<Value*>indices;
    indices.push_back(ConstantInt::get(Type::getInt32Ty(TheContext), 0));
    indices.push_back(ConstantInt::get(Type::getInt32Ty(TheContext), 0));
    Value* gep = Builder.CreateGEP(pArgThis->getType()->getPointerElementType(), pArgThis, indices);        //crashes solve: forgot to setbody()    

    Builder.CreateStore(pArgAge, gep);

    Builder.CreateRetVoid();
}

/*
define void @tc_sid_setAll(%SIDerived* %this, i32 %a) {
entry:
  %0 = bitcast %SIDerived* %this to %SIBase*
  call void @tc_sib_set(%SIBase* %0, i32 %a)
  %1 = getelementptr %SIDerived, %SIDerived* %this, i32 0, i32 1
  store i32 %a, i32* %1, align 4
  ret void
}
*/
Function* f_tc_sib_set;

void createBB_tc_f_tc_sid_setAll(Module& M, Function* f)
{
    Argument* pArgThis = f->getArg(0);
    Argument* pArgAge = f->getArg(1);

    Value* baseClass = Builder.CreateBitCast(pArgThis, sty_SIBase->getPointerTo(0));
    std::vector<Value*> args;
    args.push_back(baseClass);
    args.push_back(pArgAge);
    Builder.CreateCall(f_tc_sib_set, args);

    std::vector<Value*>indices;
    indices.push_back(ConstantInt::get(Type::getInt32Ty(TheContext), 0));
    indices.push_back(ConstantInt::get(Type::getInt32Ty(TheContext), 1));
    Value* gep = Builder.CreateGEP(pArgThis->getType()->getPointerElementType(), pArgThis, indices);        //crashes solve: forgot to setbody()    

    Builder.CreateStore(pArgAge, gep);

    Builder.CreateRetVoid();
}

void create_SingleInheritance(Module& M)
{
    //
    // create struct sty_SIBase, sty_SIDerived
    //
    CStruct cs_SIBase;
    cs_SIBase.push<int>(M, 32);
    sty_SIBase = cs_SIBase.create(M, "SIBase");
    cs_SIBase.setBody();

    CStruct cs_SIDerived;
    cs_SIDerived.push<int>(M, 32);
    cs_SIDerived.push<int>(M, 32);
    sty_SIDerived = cs_SIDerived.create(M, "SIDerived");
    cs_SIDerived.setBody();

    //
    //create 4 function declarations.
    //
    CFunction tc_sib_constructor;
    tc_sib_constructor.pushArg(PointerType::get(sty_SIBase, 0), "this");
    tc_sib_constructor.setRetType(Type::getVoidTy(M.getContext()));
    Function* f_tc_sib_constructor = tc_sib_constructor.create(*TheModule.get(), Function::ExternalLinkage, "tc_sib_constructor");

    CFunction tc_sib_set;
    tc_sib_set.pushArg(PointerType::get(sty_SIBase, 0), "this");
    tc_sib_set.pushArg(Type::getInt32Ty(TheContext), "a");
    tc_sib_set.setRetType(Type::getVoidTy(M.getContext()));
    f_tc_sib_set = tc_sib_set.create(*TheModule.get(), Function::ExternalLinkage, "tc_sib_set");

    CFunction tc_sid_constructor;
    tc_sid_constructor.pushArg(PointerType::get(sty_SIDerived, 0), "this");
    tc_sid_constructor.setRetType(Type::getVoidTy(M.getContext()));
    Function* f_tc_sid_constructor = tc_sid_constructor.create(*TheModule.get(), Function::ExternalLinkage, "tc_sid_constructor");

    CFunction tc_sid_setAll;
    tc_sid_setAll.pushArg(PointerType::get(sty_SIDerived, 0), "this");
    tc_sid_setAll.pushArg(Type::getInt32Ty(TheContext), "a");
    tc_sid_setAll.setRetType(Type::getVoidTy(M.getContext()));
    Function* f_tc_sid_setAll = tc_sid_setAll.create(*TheModule.get(), Function::ExternalLinkage, "tc_sid_setAll");

    //
    // create basic block for each function
    // 
    create_BB(createBB_tc_sib_constructor, f_tc_sib_constructor);
    create_BB(createBB_tc_f_tc_sib_set, f_tc_sib_set);
    create_BB(createBB_tc_sid_constructor, f_tc_sid_constructor);
    create_BB(createBB_tc_f_tc_sid_setAll, f_tc_sid_setAll);

    //
    // call set functions
    //
    auto H = TheJIT->addModule(std::move(TheModule));

    InitializeModuleAndPassManager();

    // construct
    auto ExprSymbol = TheJIT->findSymbol("tc_sid_constructor");
    void(*FP_tc_sid_constructor)(SIDerived*) = (void(*)(SIDerived*))(intptr_t)cantFail(ExprSymbol.getAddress());

    FP_tc_sid_constructor(&siDerived);

    SIBase* base = static_cast<SIBase*>(&siDerived);
    printf("after construction: base a = %d, derived a = %d\n", base->a, siDerived.a);

    //base seta
    auto ExprSymbol1 = TheJIT->findSymbol("tc_sib_set");
    void(*FP_tc_sib_set)(SIBase*, int) = (void(*)(SIBase*, int))(intptr_t)cantFail(ExprSymbol1.getAddress());

    FP_tc_sib_set(base, 1);
    printf("after base::set : base a = %d, derived a = %d\n", base->a, siDerived.a);

    //derived setAll
    auto ExprSymbol2 = TheJIT->findSymbol("tc_sid_setAll");
    void(*FP_tc_sid_setAll)(SIDerived*, int) = (void(*)(SIDerived*, int))(intptr_t)cantFail(ExprSymbol2.getAddress());

    FP_tc_sid_setAll(&siDerived, 2);

    printf("after setAll: base a = %d, derived a = %d\n", base->a, siDerived.a);
    //
    TheJIT->removeModule(H);
}

//////////////////////////////////////////////////////////////////////////
//                      createBB_class 
// 
class testClass {
protected:
    char name[32];
    int age;
public:
    testClass() { age = 0; }
    void setAge(int age) { this->age = age; }
    int getAge(void) { return age; }
};
testClass testclass;
StructType* sty_testClass;

/*
define void @tc_constructor(%testClass* %this) {
entry:
  %0 = getelementptr %testClass, %testClass* %this, i32 0, i32 1
  store i32 999, i32* %0, align 4
  ret void
}
*/
void createBB_tc_constructor(Module& M, Function* f)
{
    Argument* pArgThis = f->getArg(0);
   
    std::vector<Value*>indices;
    indices.push_back(ConstantInt::get(Type::getInt32Ty(TheContext), 0));
    indices.push_back(ConstantInt::get(Type::getInt32Ty(TheContext), 1));
    Value* gep = Builder.CreateGEP(pArgThis->getType()->getPointerElementType(), pArgThis, indices);        //crashes solve: forgot to setbody()    

    Builder.CreateStore(CConstant::getInt32(M.getContext(), 999), gep);

    Builder.CreateRetVoid();
}

/*
define void @tc_setAge(%testClass* %this, i32 %age) {
entry:
  %0 = getelementptr %testClass, %testClass* %this, i32 0, i32 1
  store i32 %age, i32* %0, align 4
  ret void
}
*/
void createBB_tc_setAge(Module& M, Function* f)
{
    Argument* pArgThis = f->getArg(0);
    Argument* pArgAge = f->getArg(1);

    std::vector<Value*>indices;
    indices.push_back(ConstantInt::get(Type::getInt32Ty(TheContext), 0));
    indices.push_back(ConstantInt::get(Type::getInt32Ty(TheContext), 1));
    Value* gep = Builder.CreateGEP(pArgThis->getType()->getPointerElementType(), pArgThis, indices);        //crashes solve: forgot to setbody()    

    Builder.CreateStore(pArgAge, gep);

    Builder.CreateRetVoid();
}

void createBB_tc_getAge(Module& M, Function* f)
{
    Argument* pArgThis = f->getArg(0);

    std::vector<Value*>indices;
    indices.push_back(ConstantInt::get(Type::getInt32Ty(TheContext), 0));
    indices.push_back(ConstantInt::get(Type::getInt32Ty(TheContext), 1));
    Value* gep = Builder.CreateGEP(pArgThis->getType()->getPointerElementType(), pArgThis, indices);        //crashes solve: forgot to setbody()    

    Builder.CreateRet(Builder.CreateLoad(gep));
}


void create_testClass(Module& M)
{
    //
    // create struct testClass
    //
    CStruct cs_testClass;
    cs_testClass.push_scalar_array<char>(M, 8, 32);
    cs_testClass.push<int>(M, 32);
    sty_testClass = cs_testClass.create(M, "testClass");
    cs_testClass.setBody();

    //
    //create 3 function declarations.
    //
    CFunction tc_constructor;
    tc_constructor.pushArg(PointerType::get(sty_testClass, 0), "this");
    tc_constructor.setRetType(Type::getVoidTy(M.getContext()));
    Function* f_tc_constructor = tc_constructor.create(*TheModule.get(), Function::ExternalLinkage, "tc_constructor");

    CFunction tc_setAge;
    tc_setAge.pushArg(PointerType::get(sty_testClass, 0), "this");
    tc_setAge.pushArg(Type::getInt32Ty(TheContext), "age");
    tc_setAge.setRetType(Type::getVoidTy(M.getContext()));
    Function* f_tc_setAge = tc_setAge.create(*TheModule.get(), Function::ExternalLinkage, "tc_setAge");

    CFunction tc_getAge;
    tc_getAge.pushArg(PointerType::get(sty_testClass, 0), "this");
    tc_getAge.setRetType(Type::getInt32Ty(TheContext));
    Function* f_tc_getAge = tc_getAge.create(*TheModule.get(), Function::ExternalLinkage, "tc_getAge");

    //
    // create basic block for each function
    // 
    create_BB(createBB_tc_constructor, f_tc_constructor);
    printf("after construction, age = %d\n", testclass.getAge());
    create_BB(createBB_tc_setAge, f_tc_setAge);
    create_BB(createBB_tc_getAge, f_tc_getAge);

    //
    // call those functions
    //
    auto H = TheJIT->addModule(std::move(TheModule));

    InitializeModuleAndPassManager();

    // construct
    auto ExprSymbol = TheJIT->findSymbol("tc_constructor");
    void(*FP_tc_constructor)(testClass*) = (void(*)(testClass*))(intptr_t)cantFail(ExprSymbol.getAddress());
    
    FP_tc_constructor(&testclass);
    printf("FP_tc_constructor: age is %d\n", testclass.getAge());

    //setAge
    auto ExprSymbol1 = TheJIT->findSymbol("tc_setAge");
    void(*FP_tc_setAge)(testClass*, int) = (void(*)(testClass*, int))(intptr_t)cantFail(ExprSymbol1.getAddress());

    FP_tc_setAge(&testclass, 1234);
    printf("FP_tc_setAge: age is %d\n", testclass.getAge());

    //getAge
    auto ExprSymbol2 = TheJIT->findSymbol("tc_getAge");
    int(*FP_tc_getAge)(testClass*) = (int(*)(testClass*))(intptr_t)cantFail(ExprSymbol2.getAddress());

    int age = FP_tc_getAge(&testclass);
    printf("FP_tc_getAge: %d\n", age);

    TheJIT->removeModule(H);

    //
    // 
    //
}

//////////////////////////////////////////////////////////////////////////
//                      createBB_SSA_PHI 
// 
/*
result:
define i8 @tf1() {
entry:
  %0 = load i8, i8* inttoptr (i64 140701517574144 to i8*), align 1
  %1 = load i8, i8* inttoptr (i64 140701517574145 to i8*), align 1
  %2 = icmp sgt i8 %0, %1
  br i1 %2, label %then, label %else

then:                                             ; preds = %entry
  br label %ifcont

else:                                             ; preds = %entry
  br label %ifcont

ifcont:                                           ; preds = %else, %then
  %iftmp = phi i8 [ %0, %then ], [ %1, %else ]
  ret i8 %iftmp
}
*/
char a = 'a';
char b = '0';
void createBB_SSA_PHI(Module& M, Function* f)
{
    Value* pa = CBuilder::CreateIntToPtr<char>(Builder, TheContext, &a);
    Value* pb = CBuilder::CreateIntToPtr<char>(Builder, TheContext, &b);

    Value* va = Builder.CreateLoad(pa);
    Value* vb = Builder.CreateLoad(pb);

    Value* CondV = Builder.CreateICmpSGT(va, vb);

    BasicBlock* ThenBB = BasicBlock::Create(TheContext, "then", f);
    BasicBlock* ElseBB = BasicBlock::Create(TheContext, "else", f);
    BasicBlock* MergeBB = BasicBlock::Create(TheContext, "ifcont", f);

    Builder.CreateCondBr(CondV, ThenBB, ElseBB);

    // then BB
    Builder.SetInsertPoint(ThenBB);
    Builder.CreateBr(MergeBB);

    // Emit else block.
    Builder.SetInsertPoint(ElseBB);
    Builder.CreateBr(MergeBB);

    // Emit merge block.
    Builder.SetInsertPoint(MergeBB);
    PHINode* PN = Builder.CreatePHI(Type::getInt8Ty(TheContext), 2, "iftmp");
    PN->addIncoming(va, ThenBB);
    PN->addIncoming(vb, ElseBB);

    Builder.CreateRet(PN);
}

//////////////////////////////////////////////////////////////////////////
//                      createBB_external_variables 
// 
float gFloat = 111.f;
double gDouble = 222.;

void createBB_external_variables(Module& M, Function* f)
{
    Value* fp = CBuilder::CreateIntToPtr<float>(Builder, TheContext, &gFloat);
    Value* dp = CBuilder::CreateIntToPtr<double>(Builder, TheContext, &gDouble);

    Value* vfp = Builder.CreateLoad(fp);
    Value* vdp = Builder.CreateLoad(dp);
    
//#define TEST_MULTIPLICATION
#ifdef TEST_MULTIPLICATION
    Value* vmfp = Builder.CreateFMul(vfp, CConstant::getFloat(TheContext, 2.f));
    Value* vmdp = Builder.CreateFMul(vdp, CConstant::getDouble(TheContext, 2.));

    Builder.CreateStore(vmfp, fp);
    Builder.CreateStore(vmdp, dp);
#endif

//#define FP_EXTEND
#ifdef FP_EXTEND
    Value* extended = Builder.CreateFPExt(vfp, Type::getDoubleTy(TheContext));
    Builder.CreateStore(extended, dp);
#endif

#define FP_TRUNC
#ifdef FP_TRUNC
    Value* extended = Builder.CreateFPTrunc(vdp, Type::getFloatTy(TheContext));
    Builder.CreateStore(extended, fp);
#endif

    Builder.CreateRet(CConstant::getInt8(TheContext, 'X'));
}

//////////////////////////////////////////////////////////////////////////
//                      createBB_truncate 
// 
/*
char f(void)
{
  int x = 0x12345678;
  char c = (char)x;
  return c;
}
define signext i8 @f() #0 {
  %1 = alloca i32, align 4
  %2 = alloca i8, align 1
  store i32 305419896, i32* %1, align 4
  %3 = load i32, i32* %1, align 4
  %4 = trunc i32 %3 to i8
  store i8 %4, i8* %2, align 1
  %5 = load i8, i8* %2, align 1
  ret i8 %5
}
*/
void createBB_truncate(Module& M, Function* f)
{
    AllocaInst* a32 = CreateEntryBlockAlloca(Type::getInt32Ty(TheContext), f, "");
    AllocaInst* a8 = CreateEntryBlockAlloca(Type::getInt8Ty(TheContext), f, "");
    
    Builder.CreateStore(CConstant::getInt32(TheContext, 0x12345678), a32);
    Value* v32 = Builder.CreateLoad(a32);
    Value* truncated = Builder.CreateTrunc(v32, Type::getInt8Ty(TheContext));
    Builder.CreateStore(truncated, a8);

    Builder.CreateRet(Builder.CreateLoad(a8));
}

//////////////////////////////////////////////////////////////////////////
//                      createBB_sext_zext 
// 
/*
int f(void)
{
  char a = 'Y';
  return (int)a;
}

define i32 @f() #0 {
  %1 = alloca i8, align 1
  store i8 89, i8* %1, align 1
  %2 = load i8, i8* %1, align 1
  %3 = sext i8 %2 to i32
  ret i32 %3
}
*/
void createBB_sext_zext(Module& M, Function* f)
{
    //NOTE: a8 is a pointer, that points to a stack location, to use its value, load first.
    //this address a8 is not the runtime address?? (it's very different from function address)
    AllocaInst* a8 = CreateEntryBlockAlloca(Type::getInt8Ty(TheContext), f, "");
    Builder.CreateStore(CConstant::getInt8(TheContext, -1), a8);        
    Value* loaded = Builder.CreateLoad(a8);
    Value* a8Ext = Builder.CreateSExt(loaded, Type::getInt32Ty(TheContext));  
    //sext: 0xff -> 0xffffffff
    //zext: 0xff -> 0x000000ff
    Builder.CreateRet(a8Ext);
}


//////////////////////////////////////////////////////////////////////////
//                      createBB_cast 
// 
/*
result:
define i32 @tf1() {
entry:
  %0 = call void* @malloc(i32 4)
  %1 = bitcast void* %0 to %stru*
  %2 = getelementptr %stru, %stru* %1, i32 0, i32 0
  store i32 1234, i32* %2, align 4
  %3 = load i32, i32* %2, align 4
  call void @free(void* %0)
  ret i32 %3
}
*/
void createBB_cast(Module& M, Function* f)
{
    // declare malloc and free
    FunctionCallee fmalloc = declare_malloc();
    FunctionCallee ffree = declare_free();

    // create struct
    CStruct stru;
    stru.push<int>(M, 32);
    StructType* sty = stru.create(M, "stru");
    stru.setBody();
    PP(sty);

    // create pointer struct (not used)
    PointerType* psty = PointerType::get(sty, 0);
    PP(psty);

    //malloc 4 bytes, the sizeof the struct
    std::vector<Value*>args;
    args.push_back(CConstant::getInt32(TheContext, 4));
    CallInst* inst = Builder.CreateCall(fmalloc, args);
    //PP(inst);

    // cast (void*) to (struct*)
    Value* casted = Builder.CreateBitCast(inst, psty);
    PP(casted);

    // store 0x12345678 to struct.int
    std::vector<Value*>indices;
    indices.push_back(ConstantInt::get(Type::getInt32Ty(TheContext), 0));
    indices.push_back(ConstantInt::get(Type::getInt32Ty(TheContext), 0));
    Value* gep = Builder.CreateGEP(casted, indices);        //crashes solve: forgot to setbody()

    //
    Builder.CreateStore(ConstantInt::get(Type::getInt32Ty(TheContext), 0x12345678), gep);

    //

    // do a load from the stored address (for returning)
    Value* loadedV2 = Builder.CreateLoad(gep);
    //* loadedV2 = Builder.CreateLoad(a8Ext);

    // free allocated
    args.clear();
    args.push_back(inst);
    Builder.CreateCall(ffree, args);

    Builder.CreateRet(loadedV2);
}

//////////////////////////////////////////////////////////////////////////
//                      test_create_JIT 
// 
void test_create_JIT(Module& M)
{
 
}

//////////////////////////////////////////////////////////////////////////
//                      test_datalayout 
// 
void test_datalayout(Module& M)
{
    errs() << M.getDataLayoutStr() << "\n";

    DataLayout layout = M.getDataLayout();

    errs() << "integer bitwidth 37 is " << (layout.isLegalInteger(37) ? "legal" : "illegal") << "\n";
    errs() << "integer bitwidth 64 is " << (layout.isLegalInteger(64) ? "legal" : "illegal") << "\n";

    errs() << (layout.isBigEndian() ? "big endian" : "little endian") << "\n";

    errs() << (layout.isNonIntegralAddressSpace(0) ? "NonIntegralAddressSpace" : "IntegralAddressSpace") << "\n";
}

//////////////////////////////////////////////////////////////////////////
//                      test_conversion 
// 
void test_conversion(Module&M)
{
    CGlobalVariables gVars;
    gVars.setModule(&M);
    Value* gv = gVars.create<char>("gv", CConstant::getInt8(M.getContext(), 'X'));
    errs() << "gv:"; gv->print(errs()); errs() << "\n";

    Value* ex = Builder.CreateZExt(gv, Type::getInt32Ty(M.getContext()));
    errs() << "ex:"; ex->print(errs()); errs() << "\n";
}

//////////////////////////////////////////////////////////////////////////
//                      test_call_external_variadic 
// 
void test_call_external_variadic(void)
{
#if 0
    Type* args = PointerType::get(Type::getInt8Ty(TheContext), 0);
    Type* retType = IntegerType::getInt8PtrTy(TheContext);
#else
    std::vector<Type*> args;
    args.push_back(Type::getInt8PtrTy(TheContext));
    Type* retType = Builder.getInt32Ty();
#endif

    FunctionType* printfType = FunctionType::get(retType, args, true);
    TheModule.get()->getOrInsertFunction("printf", printfType);

    auto H = TheJIT->addModule(std::move(TheModule));

    InitializeModuleAndPassManager();
    auto ExprSymbol = TheJIT->findSymbol("printf");
    void (*FP)(...) = (void (*)(...))(intptr_t)cantFail(ExprSymbol.getAddress());
    FP("%s = %d\n", "hero", 100);
    errs() << "\n";

    TheJIT->removeModule(H);
}

//////////////////////////////////////////////////////////////////////////
//                      test_constant
// 
Constant* test_constant(Module&M)
{
    // create struct
    CStruct cs;
    cs.push<int>(M);
    cs.push<int>(M);
    StructType* struct_type = cs.create(*TheModule.get(), "cs");

    errs() << "struct_type:"; struct_type->print(errs()); errs() << "\n";

    // create constant value for the struct
    CConstantStructValue csv;
    csv.push<int>(*TheModule.get(), 111, 32);
    csv.push<int>(*TheModule.get(), 222, 32);

    Constant* ccc = csv.create(struct_type);
    errs() << "ccc:"; ccc->print(errs()); errs() << "\n";

    return ccc;
}


//////////////////////////////////////////////////////////////////////////
//                      test_struct
// 
struct munger_struct {
    int i;
    char c;
    float f;
    double d;
};

/*
char f(void)
{
    munger_struct stru = { 0, 'X', 0.f, 0. };
    return stru.c;
}

---------- expect(from clang generated IR)  ----------------->

@stru = global %struct.munger_struct { i32 0, i8 88, float 0.000000e+00, double 0.000000e+00 }, align 8

define signext i8 @f() #0 {
  %1 = load i8, i8* getelementptr inbounds (%struct.munger_struct, %struct.munger_struct* @stru, i32 0, i32 1), align 4
  ret i8 %1
}
*/

void createBB_struct(Module&M, Function* f)
{
    // create struct
    CStruct cs;
    StructType* struct_type = cs.create(*TheModule.get(), "cs");
    //cs.push_struct_pointer(M, struct_type);
    cs.push<int>(M, 32);
    cs.push<char>(M);
    cs.push<float>(M);
    cs.push<double>(M);
    cs.setBody();

    errs() << "struct_type:"; struct_type->print(errs()); errs() << "\n";

    // create constant value for the struct
    CConstantStructValue csv;
    //csv.push()
    csv.push<int>(*TheModule.get(), 12345, 32);
    csv.push<char>(*TheModule.get(), 'X', 8);
    csv.push<float>(*TheModule.get(), 111.f, 32);
    csv.push<double>(*TheModule.get(), 222., 32);

    Constant* ccc = csv.create(struct_type);
    errs() << "ccc:"; ccc->print(errs()); errs() << "\n";

    // create variable(sturct)
    CGlobalVariables g;
    g.setAlign(1);
    g.setLinkage(GlobalValue::PrivateLinkage);
    g.setModule(TheModule.get());
    llvm::GlobalVariable* gv = g.create("gv", struct_type, ccc);

    errs() << "gv:"; gv->print(errs()); errs() << "\n";

    std::vector<Value*>indices;
    indices.push_back(ConstantInt::get(Type::getInt32Ty(TheContext), 0));
    indices.push_back(ConstantInt::get(Type::getInt32Ty(TheContext), 1));

    //gv:@gv = private global %cs { i32 12345, i8 88, float 1.110000e+02, double 2.220000e+02 }, align 1
    Value* gep = Builder.CreateGEP(gv, indices);        //crashes solve: forgot to setbody()
    Value* loadedV = Builder.CreateLoad(gep, "");

    Builder.CreateRet(loadedV);
}

//////////////////////////////////////////////////////////////////////////
//                      createBB_mutable
// 
/*
char f(void)
{
  char a;
  a = 'Y';
  return a;
}

expect(from clang generated IR):
define signext i8 @f() #0 {
  %1 = alloca i8, align 1
  store i8 89, i8* %1, align 1
  %2 = load i8, i8* %1, align 1
  ret i8 %2
}
*/
void createBB_mutable(Module& M, Function* f)
{
    AllocaInst* Alloca = CreateEntryBlockAlloca(Type::getInt8Ty(TheContext), f, "");

    Builder.CreateStore(CConstant::getInt8(TheContext, 'Y'), Alloca);

    Builder.CreateRet(Builder.CreateLoad(Alloca));
}


//////////////////////////////////////////////////////////////////////////
//                      test_string
// 
/*
char f()
{
    char* s = "test";
    return s[0];
}

  --------> expect(from clang generated IR) --------> 

 @.str = private unnamed_addr constant [5 x i8] c"test\00", align 1
define signext i8 @f() #0 {
  %1 = alloca i8*, align 8
  store i8* getelementptr inbounds ([5 x i8], [5 x i8]* @.str, i32 0, i32 0), i8** %1, align 8
  %2 = load i8*, i8** %1, align 8
  %3 = getelementptr inbounds i8, i8* %2, i64 0
  %4 = load i8, i8* %3, align 1
  ret i8 %4
}
*/

void createBB_string(Module& M, Function* f)
{
    // create string
    CGlobalVariables g;
    g.setAlign(1);
    g.setLinkage(GlobalValue::PrivateLinkage);
    g.setModule(TheModule.get());
    Constant* cs = ConstantDataArray::getString(TheContext, "test", true);
    //Constant* cs = Builder.CreateGlobalStringPtr("test", "str");
    llvm::GlobalVariable* gv = g.create_array<char>("str", 5, cs);

    std::vector<Value*>indices;
    indices.push_back(ConstantInt::get(Type::getInt32Ty(TheContext), 0));
    indices.push_back(ConstantInt::get(Type::getInt32Ty(TheContext), 2));

    AllocaInst* Alloca = CreateEntryBlockAlloca(Type::getInt8PtrTy(TheContext), f, "");

    Value* gep = Builder.CreateGEP(gv, indices);        //crashes solve: forgot to setbody()
    //PP(gep);
    
    Builder.CreateStore(gep, Alloca);

    Value* loadedV = Builder.CreateLoad(Alloca, "");

    Value* gep2 = Builder.CreateGEP(loadedV, ConstantInt::get(Type::getInt32Ty(TheContext), 0));        //crashes solve: forgot to setbody()

    Value* loadedV2 = Builder.CreateLoad(gep2, "");

    Builder.CreateRet(loadedV2);
}


int main()
{
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

    InitializeNativeTarget();
    InitializeNativeTargetAsmPrinter();
    InitializeNativeTargetAsmParser();

    //KaleidoscopeJIT is copied from LLVM test project.
    TheJIT = std::make_unique<KaleidoscopeJIT>();
    InitializeModuleAndPassManager();

    //
    // tests
    //
    //test_constant(*TheModule.get());
    //test_toy_memory(M);
    //test_toy_memory_packed(M);
    //test_global_variables();        //basic function calls
    //test_call_external_function();
    //call_function<char>(createBB_string);
    //call_function<char>(createBB_struct);
    //test_call_external_variadic();
    //test_conversion(*TheModule.get());
    //test_datalayout(*TheModule.get());

    //call_function<char>(createBB_mutable);

    //test_create_JIT(*TheModule.get());

    //call_function<int>(createBB_cast);	
    //call_function<int>(createBB_sext_zext);
    //call_function<char>(createBB_truncate);
    //call_function<char>(createBB_external_variables);         //IntToPtr, fpext, fptrunc.
    //call_function<char>(createBB_SSA_PHI);                    //if control block.  
    
    //create_testClass(*TheModule.get());                       //this pointer. class construct, get/set/constrctor
    create_SingleInheritance(*TheModule.get());                 //access base class member functions.


    return 0;
}