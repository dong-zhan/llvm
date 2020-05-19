#include "stdafx.h"


#include "toy_constant.h"
#include "toy_module.h"
#include "toy_basicblock.h"
#include "toy_function.h"
#include "toy_instruction.h"
#include "toy_memory.h"


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

CGlobalVariables gVars;

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
    BasicBlock* BB = BasicBlock::Create(TheContext, "entry", f);

    Builder.SetInsertPoint(BB);

    Value* vs[2];
    int idx = 0;
    for (auto& Arg : f->args()) {
        vs[idx++] = &Arg;
    }

    Value* Inc = Builder.CreateFAdd(vs[0], vs[1]);
    Builder.CreateRet(Inc);

    verifyFunction(*f);
    f->print(errs());
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

---------- expect ----------------->

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

    BasicBlock* BB = BasicBlock::Create(TheContext, "entry", f);
    Builder.SetInsertPoint(BB);
    //Builder.saveIP();

    Value* gep = Builder.CreateGEP(gv, indices);
    Value* loadedV = Builder.CreateLoad(gep, "");

    Builder.CreateRet(loadedV);

    verifyFunction(*f);
    f->print(errs());
}

void test_struct(void)
{
    CFunction function;
    function.setRetType(Type::getInt32Ty(TheContext));
    Function* f = function.create(*TheModule.get(), Function::ExternalLinkage, "tf1");

    createBB_struct(*TheModule.get(), f);

    auto H = TheJIT->addModule(std::move(TheModule));

    InitializeModuleAndPassManager();
    auto ExprSymbol = TheJIT->findSymbol("tf1");
    char (*FP)() = (char (*)())(intptr_t)cantFail(ExprSymbol.getAddress());
    char c = FP();
    errs() << "output:" << c << "\n";

    TheJIT->removeModule(H);
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

  --------> expect --------> 

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

void createBB_string(Function* f)
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

    BasicBlock* BB = BasicBlock::Create(TheContext, "entry", f);
    Builder.SetInsertPoint(BB);
    //Builder.saveIP();

    AllocaInst* Alloca = CreateEntryBlockAlloca(Type::getInt8PtrTy(TheContext), f, "");

    Value* gep = Builder.CreateGEP(gv, indices);
    
    Builder.CreateStore(gep, Alloca);

    Value* loadedV = Builder.CreateLoad(Alloca, "");

    Value* gep2 = Builder.CreateGEP(loadedV, ConstantInt::get(Type::getInt32Ty(TheContext), 0));

    Value* loadedV2 = Builder.CreateLoad(gep2, "");

    Builder.CreateRet(loadedV2);

    verifyFunction(*f);
    f->print(errs());
}

void test_string(void)
{
    CFunction function;
    function.setRetType(Type::getInt32Ty(TheContext));
    Function* f = function.create(*TheModule.get(), Function::ExternalLinkage, "tf1");

    createBB_string(f);

    auto H = TheJIT->addModule(std::move(TheModule));

    InitializeModuleAndPassManager();
    auto ExprSymbol = TheJIT->findSymbol("tf1");
    char (*FP)() = (char (*)())(intptr_t)cantFail(ExprSymbol.getAddress());
    char c = FP();
    errs() << "output:" << c << "\n";

    TheJIT->removeModule(H);
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
    //test_GEP(*TheModule.get());
    //test_constant(*TheModule.get());
    //test_toy_memory(M);
    //test_toy_memory_packed(M);
    //test_global_variables();        //basic function calls
    //test_call_external_function();
    //test_string();
    test_struct();

    return 0;
}
