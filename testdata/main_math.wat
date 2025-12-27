(module
  (import "math" "add" (func $math.add (param i32 i32) (result i32)))
  (func $main (result i32)
    (call $math.add (i32.const 10) (i32.const 32))
  )
)
