; ModuleID = 'hello.bc'
target datalayout = "e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:32:64-f32:32:32-f64:32:64-v64:64:64-v128:128:128-a0:0:64-f80:32:32"
target triple = "i386-pc-linux-gnu"
	%struct.A = type { [2 x [10 x i32]], double }
	%struct.B = type { %struct.A }

define i32 @main() nounwind {
entry:
	%structure = alloca %struct.B		; <%struct.B*> [#uses=2]
	%"alloca point" = bitcast i32 0 to i32		; <i32> [#uses=0]
	br label %bb1

bb:		; preds = %bb1
	%0 = getelementptr %struct.B* %structure, i32 0, i32 0		; <%struct.A*> [#uses=1]
	%1 = getelementptr %struct.A* %0, i32 0, i32 1		; <double*> [#uses=1]
	store double 2.400000e-03, double* %1, align 4
	%2 = getelementptr %struct.B* %structure, i32 0, i32 0		; <%struct.A*> [#uses=1]
	%3 = getelementptr %struct.A* %2, i32 0, i32 1		; <double*> [#uses=1]
	%4 = load double* %3, align 4		; <double> [#uses=1]
	%5 = add double %4, %c.0		; <double> [#uses=1]
	%6 = add i32 %i.0, 1		; <i32> [#uses=1]
	%7 = add i32 %j.0, 1		; <i32> [#uses=1]
	br label %bb1

bb1:		; preds = %bb, %entry
	%c.0 = phi double [ 0.000000e+00, %entry ], [ %5, %bb ]		; <double> [#uses=1]
	%j.0 = phi i32 [ 0, %entry ], [ %7, %bb ]		; <i32> [#uses=1]
	%i.0 = phi i32 [ 9, %entry ], [ %6, %bb ]		; <i32> [#uses=2]
	%8 = icmp sle i32 %i.0, 9		; <i1> [#uses=1]
	br i1 %8, label %bb, label %bb2

bb2:		; preds = %bb1
	br label %return

return:		; preds = %bb2
	ret i32 0
}
