target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"
%opaque = type opaque
@g511 = global [511 x i8] zeroinitializer
@g512 = global [512 x i8] zeroinitializer
@g513 = global [513 x i8] zeroinitializer
@g4096 = global [4096 x i8] zeroinitializer
@gzero = global [0 x i8] zeroinitializer
@gunsized = external global %opaque
@gwide = external global <1073741824 x i64>
declare ptr @extent_good(i64, i64)
declare ptr @extent_product(i64, i64)
declare ptr @extent_missing(i64)
declare ptr @extent_unknown(i64)
declare ptr @extent_malformed(i64)
declare ptr @extent_trailing(i64)
declare ptr @extent_conflict(i64, i64)
declare ptr @extent_badindex(i64)
declare ptr @extent_pointer(ptr)
declare ptr @extent_wide(i128)
declare ptr @extent_zero(i64)
declare ptr @extent_negative(i64)
define i32 @main(i32 %argc, ptr %argv) {
entry:
  %n = sext i32 %argc to i64
  %wide = sext i64 %n to i128
  %stack = alloca [513 x i8]
  %dynamic = alloca i32, i64 %n
  %good = call ptr @extent_good(i64 64, i64 %n)
  %product = call ptr @extent_product(i64 4, i64 %n)
  %missing = call ptr @extent_missing(i64 %n)
  %unknown = call ptr @extent_unknown(i64 %n)
  %malformed = call ptr @extent_malformed(i64 %n)
  %trailing = call ptr @extent_trailing(i64 %n)
  %conflict = call ptr @extent_conflict(i64 64, i64 %n)
  %badindex = call ptr @extent_badindex(i64 %n)
  %pointer = call ptr @extent_pointer(ptr %argv)
  %too_wide = call ptr @extent_wide(i128 %wide)
  %zero = call ptr @extent_zero(i64 0)
  %negative = call ptr @extent_negative(i64 -1)
  ret i32 0
}
