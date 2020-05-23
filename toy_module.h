#ifndef __INCLUDE_TOY_MODULE__
#define __INCLUDE_TOY_MODULE__

#include "llvm/IR/Module.h"

void dump_module_structs(llvm::Module& m);
void dump_module_globals(llvm::Module& m, bool llvmPrint = false);
void dump_module_functions(llvm::Module& m);

#endif