; ModuleID = 'hello.bc'
target datalayout = "e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:32:64-f32:32:32-f64:32:64-v64:64:64-v128:128:128-a0:0:64-f80:32:32"
target triple = "i386-pc-linux-gnu"
	%struct.A = type { [2 x [10 x i32]], double }
	%struct.B = type { %struct.A }

define i32 @main() nounwind {
entry:
	%d = alloca [10 x i32]		; <[10 x i32]*> [#uses=6]
	%"alloca point" = bitcast i32 0 to i32		; <i32> [#uses=0]
	%0 = getelementptr [10 x i32]* %d, i32 0, i32 0		; <i32*> [#uses=1]
	store i32 3, i32* %0, align 4
	%1 = getelementptr [10 x i32]* %d, i32 0, i32 2		; <i32*> [#uses=1]
	store i32 13, i32* %1, align 4
	br label %bb4

bb:		; preds = %bb4
	%2 = getelementptr [10 x i32]* %d, i32 0, i32 %i.0		; <i32*> [#uses=1]
	%3 = load i32* %2, align 4		; <i32> [#uses=1]
	%4 = add i32 %3, 10		; <i32> [#uses=1]
	%5 = getelementptr [10 x i32]* %d, i32 0, i32 %i.0		; <i32*> [#uses=1]
	store i32 %4, i32* %5, align 4
	%6 = icmp sle i32 %i.0, 7		; <i1> [#uses=1]
	br i1 %6, label %bb1, label %bb2

bb1:		; preds = %bb
	%7 = getelementptr [10 x i32]* %d, i32 0, i32 1		; <i32*> [#uses=1]
	store i32 10, i32* %7, align 4
	br label %bb3

bb2:		; preds = %bb
	%8 = getelementptr [10 x i32]* %d, i32 0, i32 9		; <i32*> [#uses=1]
	store i32 9, i32* %8, align 4
	br label %bb3

bb3:		; preds = %bb2, %bb1
	%9 = add i32 %i.0, 1		; <i32> [#uses=1]
	%10 = add i32 %j.0, 1		; <i32> [#uses=1]
	br label %bb4

bb4:		; preds = %bb3, %entry
	%j.0 = phi i32 [ 0, %entry ], [ %10, %bb3 ]		; <i32> [#uses=1]
	%i.0 = phi i32 [ 9, %entry ], [ %9, %bb3 ]		; <i32> [#uses=5]
	%11 = icmp sle i32 %i.0, 9		; <i1> [#uses=1]
	br i1 %11, label %bb, label %bb5

bb5:		; preds = %bb4
	br label %return

return:		; preds = %bb5
	ret i32 0
}
