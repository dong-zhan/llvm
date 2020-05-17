# llvm

this project is for practice in IR <br>
and hopefully, the functions in it can be handy for someone who actually is creating a dynamic compiler(JIT). <br><br>
for example: <br>
1, how to create global variables? <br>
2, how to access local varaibles? <br>
3, how to call exernal functions? <br>
4, how to use struct? <br>
<br>
All functions are in testllvm.cpp
<br>


KaleidoscopeJIT.h is from llvm repository

but, seems there is a problem
I changed  <br>
[this](const std::string& Name) { return findMangledSymbol(Name); }, <br>
to <br>
[this](const llvm::StringRef& Name) { return findMangledSymbol(Name.str()); }, <br>


