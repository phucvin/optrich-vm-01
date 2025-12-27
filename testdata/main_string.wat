(module
  (import "string" "create" (func $create (param i32) (result i32)))
  (import "string" "set" (func $set (param i32 i32 i32)))
  (import "string" "get" (func $get (param i32 i32) (result i32)))
  (import "string" "concat" (func $concat (param i32 i32 i32 i32) (result i32)))
  (import "string" "print" (func $print (param i32 i32)))
  (import "env" "putchar" (func $putchar (param i32)))

  (func $main (result i32)
    (local $s1 i32)
    (local $s2 i32)
    (local $s3 i32)

    ;; Create "Hello"
    (local.set $s1 (call $create (i32.const 5)))
    (call $set (local.get $s1) (i32.const 0) (i32.const 72)) ;; H
    (call $set (local.get $s1) (i32.const 1) (i32.const 101)) ;; e
    (call $set (local.get $s1) (i32.const 2) (i32.const 108)) ;; l
    (call $set (local.get $s1) (i32.const 3) (i32.const 108)) ;; l
    (call $set (local.get $s1) (i32.const 4) (i32.const 111)) ;; o

    ;; Create " World"
    (local.set $s2 (call $create (i32.const 6)))
    (call $set (local.get $s2) (i32.const 0) (i32.const 32)) ;; space
    (call $set (local.get $s2) (i32.const 1) (i32.const 87)) ;; W
    (call $set (local.get $s2) (i32.const 2) (i32.const 111)) ;; o
    (call $set (local.get $s2) (i32.const 3) (i32.const 114)) ;; r
    (call $set (local.get $s2) (i32.const 4) (i32.const 108)) ;; l
    (call $set (local.get $s2) (i32.const 5) (i32.const 100)) ;; d

    ;; Concat
    (local.set $s3 (call $concat (local.get $s1) (i32.const 5) (local.get $s2) (i32.const 6)))

    ;; Print
    (call $print (local.get $s3) (i32.const 11))
    (call $putchar (i32.const 10)) ;; newline

    (i32.const 0)
  )
)
