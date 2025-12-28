(module
  (import "string" "create" (func $create (param i32) (result i32)))
  (import "string" "set" (func $set (param i32 i32 i32)))
  (import "string" "get" (func $get (param i32 i32) (result i32)))
  (import "string" "concat" (func $concat (param i32 i32) (result i32)))
  (import "string" "print" (func $print (param i32)))
  (import "env" "putchar" (func $putchar (param i32)))

  (string $hello "Hello")
  (string $world "World")

  (func $main (result i32)
    (local $s1 i32)
    (local $s2 i32)
    (local $s3 i32)
    (local $space i32)

    (local.set $s1 (string.const $hello))
    (local.set $s2 (string.const $world))

    (call $print (local.get $s1))
    (call $putchar (i32.const 32)) ;; space
    (call $print (local.get $s2))
    (call $putchar (i32.const 10)) ;; newline

    ;; Concat test
    (local.set $space (call $create (i32.const 1)))
    (call $set (local.get $space) (i32.const 0) (i32.const 32))

    (local.set $s3 (call $concat (local.get $s1) (local.get $space)))
    (local.set $s3 (call $concat (local.get $s3) (local.get $s2)))

    (call $print (local.get $s3))
    (call $putchar (i32.const 10)) ;; newline

    (i32.const 0)
  )
)
