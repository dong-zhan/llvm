#include "stdafx.h"


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
static AllocaInst* CreateEntryBlockAlloca(Function* TheFunction,
    StringRef VarName) {
    IRBuilder<> TmpB(&TheFunction->getEntryBlock(),
        TheFunction->getEntryBlock().begin());
    return TmpB.CreateAlloca(Type::getDoubleTy(TheContext), nullptr, VarName);
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
//                      test_call_extern
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

void test_call_extern(void)
{
    CFunction function;
    function.pushArg(Type::getInt8Ty(TheContext), "c");
    function.setRetType(Type::getDoubleTy(TheContext));
    Function* f = function.create(*TheModule.get(), Function::ExternalLinkage, "putchard");

    auto H = TheJIT->addModule(std::move(TheModule));

    InitializeModuleAndPassManager();
    auto ExprSymbol = TheJIT->findSymbol("putchard");
    double (*FP)(char) = (double (*)(char))(intptr_t)cantFail(ExprSymbol.getAddress());
    errs() << "putchard" << " not found\n";

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
    //test_toy_memory(M);
    //test_toy_memory_packed(M);
    test_global_variables();
    //test_call_extern();

    return 0;
}
