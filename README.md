# llvm
llvm

KaleidoscopeJIT.h is from llvm repository

but, seems there is a problem
I changed 
[this](const std::string& Name) { return findMangledSymbol(Name); },
to
[this](const llvm::StringRef& Name) { return findMangledSymbol(Name.str()); },
