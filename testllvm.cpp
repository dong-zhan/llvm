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

Function* createFunction(llvm::Module& m, const char* fn)
{
    CFunction function;
    function.pushArg(Type::getDoubleTy(TheContext), "x");
    function.pushArg(Type::getDoubleTy(TheContext), "y");
    function.setRetType(Type::getDoubleTy(TheContext));
    return function.create(m, Function::ExternalLinkage, fn);
}

llvm::GlobalVariable* create_global_variable(llvm::Module& M, const char* name, llvm::Type* ty, 
    llvm::GlobalValue::LinkageTypes linkage = GlobalValue::CommonLinkage, int align = 0)  //CommonLinkage ExternalLinkage
{
    test_toy_memory(M);
    test_toy_memory_packed(M);

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

void createTestBB(const char* fn)
{
    Function* TheFunction = TheModule->getFunction(fn);
    if (!TheFunction) {
        errs() << fn << " not found\n";
        return;
    }

    //
    // create BB and set insert point
    //
    BasicBlock* BB = BasicBlock::Create(TheContext, "entry", TheFunction);

    Builder.SetInsertPoint(BB);

    //
    // function parameters.
    //
    Value* vs[2];
    int idx = 0;
    for (auto& Arg : TheFunction->args()) {
        vs[idx++] = &Arg;
    }

#if 1
    //AllocaInst* Alloca = CreateEntryBlockAlloca(TheFunction, "what");
    //Builder.CreateStore(vs[0], g);
    //LoadInst* Load0 = Builder.CreateLoad(Type::getDoubleTy(TheContext), g);
    LoadInst* Load0 = Builder.CreateLoad(g);        
    //Builder.CreateStore(vs[1], g);

    LoadInst* Load1 = Builder.CreateLoad(g);
    Value* Inc = Builder.CreateFAdd(vs[0], vs[1]); 
    Builder.CreateRet(Inc);
#endif

    verifyFunction(*TheFunction);
    TheFunction->print(errs());
}

void test_call(const char* fn)
{
    auto H = TheJIT->addModule(std::move(TheModule));

    InitializeModuleAndPassManager();

    auto ExprSymbol = TheJIT->findSymbol(fn);
    assert(ExprSymbol && "Function not found");

    double (*FP)(double, double) = (double (*)(double,double))(intptr_t)cantFail(ExprSymbol.getAddress());
    if (FP) {
        fprintf(stderr, "Evaluated to %f\n", FP(1234, 5678));
    }
    else {
        errs() << fn << " not found\n";
    }

    TheJIT->removeModule(H);
}

int main()
{
    InitializeNativeTarget();
    InitializeNativeTargetAsmPrinter();
    InitializeNativeTargetAsmParser();

    //KaleidoscopeJIT is copied from LVMM test project.
    TheJIT = std::make_unique<KaleidoscopeJIT>();
    InitializeModuleAndPassManager();

    g = create_global_variable(*TheModule.get(), "g", Type::getDoubleTy(TheContext));      

    createFunction(*TheModule.get(), "tf1");
    createTestBB("tf1");

    test_call("tf1");

    return 0;
}
