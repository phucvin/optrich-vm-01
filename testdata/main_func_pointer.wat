(module
  (type $t_ii (func (param i32 i32) (result i32)))
  (type $t_void (func))

  (table 2 funcref)
  (elem (i32.const 0) $add $sub)

  (func $add (param $a i32) (param $b i32) (result i32)
    (i32.add (local.get $a) (local.get $b))
  )

  (func $sub (param $a i32) (param $b i32) (result i32)
    (i32.sub (local.get $a) (local.get $b))
  )

  (func $main (result i32)
    ;; Call add(10, 20) via table index 0
    (call_indirect (type $t_ii) (i32.const 10) (i32.const 20) (i32.const 0))

    ;; Result should be 30.
    ;; Now call sub(30, 5) via table index 1. 30 is on stack.
    (i32.const 5)
    (i32.const 1)
    (call_indirect (type $t_ii))

    ;; Result should be 25.
  )

  (export "main" (func $main))
)
