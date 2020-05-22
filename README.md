# llvm

this project is for practice in IR <br>
and hopefully, the functions in it can be handy for someone who actually is creating either a dynamic compiler(JIT) or just a compiler<br><br>
for example: <br>
1, how to create global variables? <br>
2, how to access local varaibles? <br>
3, how to call exernal functions? <br>
4, how to use struct? <br>
5, example of SSA and Phi node  <br>
6, mutable variable  <br>
7, cast <br>
8, object-oriented construct (c++ class, this, constructor, member functions) <br>
9, virtual functions <br>
10, class inheritance <br>
11, typeinfo and dynamic_cast <br>
<br>
All functions are in testllvm.cpp
<br>


KaleidoscopeJIT.h is from llvm repository

but, seems there is a problem
I changed  <br>
[this](const std::string& Name) { return findMangledSymbol(Name); }, <br>
to <br>
[this](const llvm::StringRef& Name) { return findMangledSymbol(Name.str()); }, <br>


