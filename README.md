# llvm
llvm

KaleidoscopeJIT.h is from llvm repository

but, seems there is a problem
I changed  <br>
[this](const std::string& Name) { return findMangledSymbol(Name); }, <br>
to <br>
[this](const llvm::StringRef& Name) { return findMangledSymbol(Name.str()); }, <br>



1, Making this in C is not a good idea, because, it's "usually" for JIT, C is already too low level.
2, So, it is for a "pure" compiler, or static/dynamic mixed type compiler.
3, the correct way to make a "middelware" of LLVM is to read their structure carefully, have a big picture of that.
   the faster(dangerous) way is to build from small details, let the details jell themselves.
4, I am trying my luck now :)
