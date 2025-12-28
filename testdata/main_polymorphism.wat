(module
  (import "env" "alloc" (func $alloc (param i32) (result i32)))
  (import "env" "write_i32" (func $write_i32 (param i32 i32 i32)))
  (import "env" "read_i32" (func $read_i32 (param i32 i32) (result i32)))
  (import "env" "putchar" (func $putchar (param i32)))

  ;; Define Type for methods: (param $inst i32) -> void
  (type $method_t (func (param i32)))

  ;; Define Table for dynamic dispatch
  ;; Size 10 is arbitrary but enough for our indices (we'll use small indices)
  (table 10 funcref)

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

  ;; Register methods in table
  ;; Using indices 1 and 2 for simplicity
  (elem (i32.const 1) $Car_start $Bike_start)

  ;; Move function taking interface (handle containing itab + data)
  ;; Interface Layout:
  ;; Offset 0: num_methods (i32) - Not strictly used here but requested
  ;; Offset 4: method_index (i32) - Index in table
  ;; Offset 8: data_handle (i32)
  (func $move (param $iface i32)
    (local $method_idx i32)
    (local $data i32)
    ;; Unpack method index (offset 4)
    (local.set $method_idx (call $read_i32 (local.get $iface) (i32.const 4)))
    ;; Unpack data (offset 8)
    (local.set $data (call $read_i32 (local.get $iface) (i32.const 8)))

    ;; Dynamic Call
    ;; call_indirect (type $t) (arg1) ... (index)
    (call_indirect (type $method_t) (local.get $data) (local.get $method_idx))
  )

  (func $main (result i32)
    (local $car_inst i32)
    (local $bike_inst i32)
    (local $car_iface i32)
    (local $bike_iface i32)

    ;; Create Car, v1=1
    (local.set $car_inst (call $alloc (i32.const 4)))
    (call $car_set_v1 (local.get $car_inst) (i32.const 1))

    ;; Create Car Interface
    ;; Layout: [num_methods=1, method_idx=1 (Car_start), data=inst]
    ;; Size: 12 bytes
    (local.set $car_iface (call $alloc (i32.const 12)))
    (call $write_i32 (local.get $car_iface) (i32.const 0) (i32.const 1)) ;; num_methods
    (call $write_i32 (local.get $car_iface) (i32.const 4) (i32.const 1)) ;; method index for start (Car)
    (call $write_i32 (local.get $car_iface) (i32.const 8) (local.get $car_inst)) ;; data

    ;; Move Car
    (call $move (local.get $car_iface))

    ;; Create Bike, v2=2
    (local.set $bike_inst (call $alloc (i32.const 4)))
    (call $bike_set_v2 (local.get $bike_inst) (i32.const 2))

    ;; Create Bike Interface
    ;; Layout: [num_methods=1, method_idx=2 (Bike_start), data=inst]
    (local.set $bike_iface (call $alloc (i32.const 12)))
    (call $write_i32 (local.get $bike_iface) (i32.const 0) (i32.const 1)) ;; num_methods
    (call $write_i32 (local.get $bike_iface) (i32.const 4) (i32.const 2)) ;; method index for start (Bike)
    (call $write_i32 (local.get $bike_iface) (i32.const 8) (local.get $bike_inst)) ;; data

    ;; Move Bike
    (call $move (local.get $bike_iface))

    (call $putchar (i32.const 10)) ;; newline

    (i32.const 0)
  )
)
