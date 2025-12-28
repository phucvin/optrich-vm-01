(module
  (import "env" "alloc" (func $alloc (param i32) (result i32)))
  (import "env" "write_u8" (func $write_u8 (param i32 i32 i32)))
  (import "env" "read_u8" (func $read_u8 (param i32 i32) (result i32)))
  (import "env" "write_i32" (func $write_i32 (param i32 i32 i32)))
  (import "env" "read_i32" (func $read_i32 (param i32 i32) (result i32)))
  (import "env" "putchar" (func $putchar (param i32)))

  (func $create (param $size i32) (result i32)
    (local $h i32)
    (local.set $h (call $alloc (i32.add (local.get $size) (i32.const 4))))
    (call $write_i32 (local.get $h) (i32.const 0) (local.get $size))
    (local.get $h)
  )

  (func $length (param $handle i32) (result i32)
    (call $read_i32 (local.get $handle) (i32.const 0))
  )

  (func $set (param $handle i32) (param $index i32) (param $val i32)
    (call $write_u8 (local.get $handle) (i32.add (local.get $index) (i32.const 4)) (local.get $val))
  )

  (func $get (param $handle i32) (param $index i32) (result i32)
    (call $read_u8 (local.get $handle) (i32.add (local.get $index) (i32.const 4)))
  )

  (func $concat (param $h1 i32) (param $h2 i32) (result i32)
    (local $len1 i32)
    (local $len2 i32)
    (local $h3 i32)
    (local $i i32)

    (local.set $len1 (call $length (local.get $h1)))
    (local.set $len2 (call $length (local.get $h2)))
    (local.set $h3 (call $create (i32.add (local.get $len1) (local.get $len2))))

    ;; Copy string 1
    (local.set $i (i32.const 0))
    (block $break1
      (loop $loop1
        (br_if $break1 (i32.ge_s (local.get $i) (local.get $len1)))

        (call $set (local.get $h3) (local.get $i) (call $get (local.get $h1) (local.get $i)))

        (local.set $i (i32.add (local.get $i) (i32.const 1)))

        (br $loop1)
      )
    )

    ;; Copy string 2
    (local.set $i (i32.const 0))
    (block $break2
      (loop $loop2
        (br_if $break2 (i32.ge_s (local.get $i) (local.get $len2)))

        (call $set (local.get $h3) (i32.add (local.get $len1) (local.get $i)) (call $get (local.get $h2) (local.get $i)))

        (local.set $i (i32.add (local.get $i) (i32.const 1)))

        (br $loop2)
      )
    )

    (local.get $h3)
  )

  (func $print (param $handle i32)
    (local $len i32)
    (local $i i32)

    (local.set $len (call $length (local.get $handle)))
    (local.set $i (i32.const 0))

    (block $break
      (loop $loop
        (br_if $break (i32.ge_s (local.get $i) (local.get $len)))

        (call $putchar (call $get (local.get $handle) (local.get $i)))

        (local.set $i (i32.add (local.get $i) (i32.const 1)))

        (br $loop)
      )
    )
  )
)
