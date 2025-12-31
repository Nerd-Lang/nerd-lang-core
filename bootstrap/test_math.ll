; Test harness for math.nerd
; Compile with: cat math.ll test_math.ll > combined.ll && clang combined.ll -o test

@fmt_add = private constant [16 x i8] c"add(5, 3) = %g\0A\00"
@fmt_sub = private constant [17 x i8] c"sub(10, 4) = %g\0A\00"
@fmt_mul = private constant [16 x i8] c"mul(6, 7) = %g\0A\00"
@fmt_div = private constant [17 x i8] c"div(20, 4) = %g\0A\00"

declare i32 @printf(i8*, ...)

define i32 @main() {
entry:
  ; Test add(5, 3) = 8
  %r1 = call double @add(double 5.0, double 3.0)
  %p1 = getelementptr [16 x i8], [16 x i8]* @fmt_add, i32 0, i32 0
  call i32 (i8*, ...) @printf(i8* %p1, double %r1)

  ; Test sub(10, 4) = 6
  %r2 = call double @sub(double 10.0, double 4.0)
  %p2 = getelementptr [17 x i8], [17 x i8]* @fmt_sub, i32 0, i32 0
  call i32 (i8*, ...) @printf(i8* %p2, double %r2)

  ; Test mul(6, 7) = 42
  %r3 = call double @mul(double 6.0, double 7.0)
  %p3 = getelementptr [16 x i8], [16 x i8]* @fmt_mul, i32 0, i32 0
  call i32 (i8*, ...) @printf(i8* %p3, double %r3)

  ; Test div(20, 4) = 5
  %r4 = call double @div(double 20.0, double 4.0)
  %p4 = getelementptr [17 x i8], [17 x i8]* @fmt_div, i32 0, i32 0
  call i32 (i8*, ...) @printf(i8* %p4, double %r4)

  ret i32 0
}
