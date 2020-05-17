# llvm

this project is for practice in IR <br>
and hopefully, the functions in it can be handy for someone who actually is creating a dynamic compiler(JIT). <br><br>
for example: <br>
1, how to create global variables? <br>
2, how to access local varaibles? <br>
3, how to call exernal functions? <br>
4, how to use struct? <br>
<br>


KaleidoscopeJIT.h is from llvm repository

but, seems there is a problem
I changed  <br>
[this](const std::string& Name) { return findMangledSymbol(Name); }, <br>
to <br>
[this](const llvm::StringRef& Name) { return findMangledSymbol(Name.str()); }, <br>



some thoughts <br>
1, Making this in C is not a good idea, because, it's "usually" for JIT, C is already too low level.<br>
2, So, it is for a "pure" compiler, or static/dynamic mixed type compiler.<br>
3, the correct way to make a "middelware" of LLVM is to read their structure carefully, have a big picture of that.<br>
   the faster(dangerous) way is to build from small details, let the details jell themselves.<br>
4, I am trying my luck now :)<br>
