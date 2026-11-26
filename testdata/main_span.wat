(module
  (import "env" "alloc" (func $alloc (param i32) (result i32)))
  (import "env" "make_span" (func $make_span (param i32 i32 i32) (result i32)))
  (import "env" "write_i32" (func $write_i32 (param i32 i32 i32)))
  (import "env" "read_i32" (func $read_i32 (param i32 i32) (result i32)))
  (import "env" "read_u8" (func $read_u8 (param i32 i32) (result i32)))
  (import "env" "putchar" (func $putchar (param i32)))

  (string $greeting "Hello World")

  ;; Print `len` raw bytes from a handle (e.g. a span into a string)
  (func $print_bytes (param $handle i32) (param $len i32)
    (local $i i32)
    (local.set $i (i32.const 0))
    (block $break
      (loop $loop
        (br_if $break (i32.ge_s (local.get $i) (local.get $len)))
        (call $putchar (call $read_u8 (local.get $handle) (local.get $i)))
        (local.set $i (i32.add (local.get $i) (i32.const 1)))
        (br $loop)
      )
    )
  )

  ;; Sum i32 values across `byte_len` bytes of a handle
  (func $sum_i32 (param $handle i32) (param $byte_len i32) (result i32)
    (local $offset i32)
    (local $sum i32)
    (local.set $offset (i32.const 0))
    (local.set $sum (i32.const 0))
    (block $break
      (loop $loop
        (br_if $break (i32.ge_s (local.get $offset) (local.get $byte_len)))
        (local.set $sum (i32.add (local.get $sum) (call $read_i32 (local.get $handle) (local.get $offset))))
        (local.set $offset (i32.add (local.get $offset) (i32.const 4)))
        (br $loop)
      )
    )
    (local.get $sum)
  )

  (func $main (result i32)
    (local $str_span i32)
    (local $arr i32)
    (local $span1 i32)
    (local $span2 i32)
    (local $total i32)

    ;; 1. Span over read-only string constant ("Hello World")
    ;; String layout: 4-byte length prefix, then ASCII bytes.
    ;; "World" starts at offset 4 + 6 = 10, length 5.
    (local.set $str_span (call $make_span (string.const $greeting) (i32.const 10) (i32.const 5)))
    (call $print_bytes (local.get $str_span) (i32.const 5))
    (call $putchar (i32.const 10)) ;; newline

    ;; 2. Mutable i32 array with overlapping and chained spans
    ;; Allocate 5 i32 elements (20 bytes): [10, 20, 30, 40, 50]
    (local.set $arr (call $alloc (i32.const 20)))
    (call $write_i32 (local.get $arr) (i32.const 0) (i32.const 10))
    (call $write_i32 (local.get $arr) (i32.const 4) (i32.const 20))
    (call $write_i32 (local.get $arr) (i32.const 8) (i32.const 30))
    (call $write_i32 (local.get $arr) (i32.const 12) (i32.const 40))
    (call $write_i32 (local.get $arr) (i32.const 16) (i32.const 50))

    ;; span1 covers arr[1..3] (offset 4, size 12)
    (local.set $span1 (call $make_span (local.get $arr) (i32.const 4) (i32.const 12)))

    ;; span2 chains from span1 and covers arr[2..3] (offset 4 of span1, size 8)
    (local.set $span2 (call $make_span (local.get $span1) (i32.const 4) (i32.const 8)))

    ;; Mutate through spans: arr[1] = 99 via span1[0], arr[3] = 77 via span2[1]
    (call $write_i32 (local.get $span1) (i32.const 0) (i32.const 99))
    (call $write_i32 (local.get $span2) (i32.const 4) (i32.const 77))

    ;; arr is now [10, 99, 30, 77, 50] -> sum = 266
    ;; span1 is [99, 30, 77] -> sum = 206
    ;; span2 is [30, 77] -> sum = 107
    ;; total = 266 + 206 + 107 = 579
    (local.set $total (call $sum_i32 (local.get $arr) (i32.const 20)))
    (local.set $total (i32.add (local.get $total) (call $sum_i32 (local.get $span1) (i32.const 12))))
    (local.set $total (i32.add (local.get $total) (call $sum_i32 (local.get $span2) (i32.const 8))))

    (local.get $total)
  )
)
