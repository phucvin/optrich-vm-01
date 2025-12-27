(module
  (import "env" "alloc" (func $alloc (param i32) (result i32)))
  (import "env" "write_u8" (func $write_u8 (param i32 i32 i32)))
  (import "env" "read_u8" (func $read_u8 (param i32 i32) (result i32)))
  (import "env" "putchar" (func $putchar (param i32)))

  (func $create (param $size i32) (result i32)
    (call $alloc (local.get $size))
  )

  (func $set (param $handle i32) (param $index i32) (param $val i32)
    (call $write_u8 (local.get $handle) (local.get $index) (local.get $val))
  )

  (func $get (param $handle i32) (param $index i32) (result i32)
    (call $read_u8 (local.get $handle) (local.get $index))
  )

  (func $concat (param $h1 i32) (param $len1 i32) (param $h2 i32) (param $len2 i32) (result i32)
    (local $h3 i32)
    (local $i i32)
    ;; h3 = alloc(len1 + len2)
    (local.set $h3 (call $alloc (i32.add (local.get $len1) (local.get $len2))))

    ;; Copy string 1
    (local.set $i (i32.const 0))
    block $break1
      loop $loop1
        local.get $i
        local.get $len1
        i32.ge_s
        br_if $break1

        ;; write_u8(h3, i, read_u8(h1, i))
        local.get $h3
        local.get $i
        local.get $h1
        local.get $i
        call $read_u8
        call $write_u8

        ;; i++
        local.get $i
        i32.const 1
        i32.add
        local.set $i

        br $loop1
      end
    end

    ;; Copy string 2
    (local.set $i (i32.const 0))
    block $break2
      loop $loop2
        local.get $i
        local.get $len2
        i32.ge_s
        br_if $break2

        ;; write_u8(h3, len1 + i, read_u8(h2, i))
        local.get $h3
        local.get $len1
        local.get $i
        i32.add
        local.get $h2
        local.get $i
        call $read_u8
        call $write_u8

        ;; i++
        local.get $i
        i32.const 1
        i32.add
        local.set $i

        br $loop2
      end
    end

    (local.get $h3)
  )

  (func $print (param $handle i32) (param $len i32)
    (local $i i32)
    (local.set $i (i32.const 0))
    block $break
      loop $loop
        local.get $i
        local.get $len
        i32.ge_s
        br_if $break

        local.get $handle
        local.get $i
        call $read_u8
        call $putchar

        local.get $i
        i32.const 1
        i32.add
        local.set $i

        br $loop
      end
    end
  )
)
