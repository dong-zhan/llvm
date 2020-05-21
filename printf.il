declare i32 @printf(i8* noalias nocapture, ...) nounwind

@.message = internal constant [12 x i8] c"Hello World\00"

define i32 @main(i32 %argc, i8** %argv) nounwind {
	%1 = getelementptr [12 x i8], [12 x i8]* @.message, i32 0, i32 0
	call i32 (i8*, ...) @printf(i8* %1)
	ret i32 0
}

; this il file can be executed by un-commenting (#if0 #endif) some codes in main() in testlvmm.cpp
