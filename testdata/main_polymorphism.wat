(module
  (import "env" "alloc" (func $alloc (param i32) (result i32)))
  (import "env" "write_i32" (func $write_i32 (param i32 i32 i32)))
  (import "env" "read_i32" (func $read_i32 (param i32 i32) (result i32)))
  (import "env" "putchar" (func $putchar (param i32)))

  ;; Car Implementation
  (func $car_set_v1 (param $inst i32) (param $val i32)
    (call $write_i32 (local.get $inst) (i32.const 0) (local.get $val))
  )

  (func $Car_start (param $inst i32)
    (local $val i32)
    ;; Print 'C'
    (call $putchar (i32.const 67))
    ;; Read v1
    (local.set $val (call $read_i32 (local.get $inst) (i32.const 0)))
    ;; Print v1 + '0'
    (call $putchar (i32.add (local.get $val) (i32.const 48)))
  )

  ;; Bike Implementation
  (func $bike_set_v2 (param $inst i32) (param $val i32)
    (call $write_i32 (local.get $inst) (i32.const 0) (local.get $val))
  )

  (func $Bike_start (param $inst i32)
    (local $val i32)
    ;; Print 'B'
    (call $putchar (i32.const 66))
    ;; Read v2
    (local.set $val (call $read_i32 (local.get $inst) (i32.const 0)))
    ;; Print v2 + '0'
    (call $putchar (i32.add (local.get $val) (i32.const 48)))
  )

  ;; Dispatcher
  ;; Simulates calling interface method `run`
  (func $dispatch_run (param $itab i32) (param $data i32)
    (local $func_id i32)
    ;; Read function ID from itab (offset 0)
    (local.set $func_id (call $read_i32 (local.get $itab) (i32.const 0)))

    ;; Check for Car_start (ID 100)
    (block $end_dispatch
      (block $try_bike
        (br_if $try_bike (i32.ne (local.get $func_id) (i32.const 100)))
        (call $Car_start (local.get $data))
        (br $end_dispatch)
      )
      ;; Check for Bike_start (ID 200)
      (block $bad_dispatch
        (br_if $bad_dispatch (i32.ne (local.get $func_id) (i32.const 200)))
        (call $Bike_start (local.get $data))
        (br $end_dispatch)
      )
    )
  )

  ;; Move function taking interface (itab, data)
  (func $move (param $itab i32) (param $data i32)
    (call $dispatch_run (local.get $itab) (local.get $data))
  )

  (func $main (result i32)
    (local $car_itab i32)
    (local $bike_itab i32)
    (local $car_inst i32)
    (local $bike_inst i32)

    ;; Setup Car VTable (ID 100)
    (local.set $car_itab (call $alloc (i32.const 4)))
    (call $write_i32 (local.get $car_itab) (i32.const 0) (i32.const 100))

    ;; Setup Bike VTable (ID 200)
    (local.set $bike_itab (call $alloc (i32.const 4)))
    (call $write_i32 (local.get $bike_itab) (i32.const 0) (i32.const 200))

    ;; Create Car, v1=1
    (local.set $car_inst (call $alloc (i32.const 4)))
    (call $car_set_v1 (local.get $car_inst) (i32.const 1))

    ;; Move Car
    (call $move (local.get $car_itab) (local.get $car_inst))

    ;; Create Bike, v2=2
    (local.set $bike_inst (call $alloc (i32.const 4)))
    (call $bike_set_v2 (local.get $bike_inst) (i32.const 2))

    ;; Move Bike
    (call $move (local.get $bike_itab) (local.get $bike_inst))

    (call $putchar (i32.const 10)) ;; newline

    (i32.const 0)
  )
)
