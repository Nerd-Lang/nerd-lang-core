; Test harness for functions.nerd
; Compile with: cat functions.ll test_functions.ll > combined.ll && clang combined.ll -o test

@fmt_square = private constant [16 x i8] c"square(5) = %g\0A\00"
@fmt_cube = private constant [14 x i8] c"cube(3) = %g\0A\00"
@fmt_add = private constant [17 x i8] c"add(10, 7) = %g\0A\00"
@fmt_double = private constant [16 x i8] c"double(8) = %g\0A\00"
@fmt_hyp = private constant [23 x i8] c"hypotenuse(3, 4) = %g\0A\00"

declare i32 @printf(i8*, ...)

define i32 @main() {
entry:
  ; Test square(5) = 25
  %r1 = call double @square(double 5.0)
  %p1 = getelementptr [16 x i8], [16 x i8]* @fmt_square, i32 0, i32 0
  call i32 (i8*, ...) @printf(i8* %p1, double %r1)

  ; Test cube(3) = 27
  %r2 = call double @cube(double 3.0)
  %p2 = getelementptr [14 x i8], [14 x i8]* @fmt_cube, i32 0, i32 0
  call i32 (i8*, ...) @printf(i8* %p2, double %r2)

  ; Test add(10, 7) = 17
  %r3 = call double @add(double 10.0, double 7.0)
  %p3 = getelementptr [17 x i8], [17 x i8]* @fmt_add, i32 0, i32 0
  call i32 (i8*, ...) @printf(i8* %p3, double %r3)

  ; Test double(8) = 16
  %r4 = call double @double(double 8.0)
  %p4 = getelementptr [16 x i8], [16 x i8]* @fmt_double, i32 0, i32 0
  call i32 (i8*, ...) @printf(i8* %p4, double %r4)

  ; Test hypotenuse(3, 4) = 5
  %r5 = call double @hypotenuse(double 3.0, double 4.0)
  %p5 = getelementptr [23 x i8], [23 x i8]* @fmt_hyp, i32 0, i32 0
  call i32 (i8*, ...) @printf(i8* %p5, double %r5)

  ret i32 0
}
