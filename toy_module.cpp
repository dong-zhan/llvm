#include "stdafx.h"

#include "toy_module.h"

using namespace llvm;
using namespace std;


void dump_module_structs(llvm::Module& m)
{
	errs() << "Module:" << m.getName() << "'s structs:\n";

	vector <StructType*> list = m.getIdentifiedStructTypes();

	for (vector <StructType*> ::iterator it = list.begin(); it != list.end(); ++it) {
		(*it)->print(errs()); 
		errs() << "\n";
	}
}

void dump_module_globals(llvm::Module& m, bool llvmPrint)
{
	errs() << "Module:" << m.getName() << "'s globals:\n";

	llvm::Module::GlobalListType& gl = m.getGlobalList();
	for (GlobalVariable& g : gl) {

		if (llvmPrint) {
			g.print(errs());
			errs() << "\n";
		}
		else {
			string type_str;
			raw_string_ostream rso(type_str);
			PointerType* ty = g.getType();
			ty->print(rso);

			errs() << rso.str() << " " << g.getName() << "\n";
		}
	}
}

void dump_module_functions(llvm::Module& m)
{
	errs() << "Module:" << m.getName() << "'s functions:\n";

	llvm::Module::FunctionListType& gl = m.getFunctionList();
	for (auto& g : gl) {
		string type_str;
		raw_string_ostream rso(type_str);
		g.getType()->print(rso);

		errs() << rso.str() << " " << g.getName() << "\n";
	}
}