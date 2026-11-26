# Memory Model & Prior Art

Optrich VM explores replacing WebAssembly's flat linear memory with **bounds-checked, permission-tagged byte blocks accessed via handles** (`alloc`, `alloc_readonly`) and **zero-copy sub-views (`make_span`)** that inherit `readOnly` permissions and can be chained across modules.

## Where This Fits in the Design Space

Most mainstream VMs choose one of two extremes:

1. **Flat linear memory** (Wasm MVP, native): Fast (hardware MMU guard pages) and easy for C/Rust/LLVM to target, but poorly suited for safe, zero-copy cross-module sharing.
2. **Typed GC object graphs** (JVM, CLR, WasmGC): Safe cross-module sharing, but requires a heavier runtime/type system and gives less control over raw byte layouts.

Optrich VM targets the middle ground: **untyped byte layouts combined with capability-like handles and spans**.

## Related Systems & Industry Precedents

### Closest Direct Siblings
* **MS-Wasm (Memory-Safe WebAssembly, POPL 2023):** Extends WebAssembly with segment memory and unforgeable handles/capabilities (base, bounds, permissions) to achieve spatial/temporal safety and zero-copy cross-module sharing without forcing languages into WasmGC.
* **Java `MemorySegment` (Project Panama / Foreign Function & Memory API, JDK 22+):** Discrete byte allocations with typed offset reads/writes, `.asReadOnly()`, and zero-copy `.asSlice(offset, size)` (analogous to `make_span`), paired with explicit lifetime scopes (`Arena`).
* **CHERI / Arm Morello:** Hardware-level capability pointers carrying `(base, length, permissions)` that allow deriving narrowed, read-only sub-capabilities to pass across trust boundaries without copying. `MemoryStore` is conceptually a software, handle-indexed version of CHERI capabilities.

### Related VM & Runtime Models
* **WasmGC:** Replaces linear memory with discrete, bounds-checked heap objects, but relies on a static nominal/structural type system and tracing GC rather than untyped byte-offset views/sub-slices.
* **Wasm Component Model:** Enables multi-module interop, but defaults to copying across module boundaries because linear memories cannot be safely sliced and shared.
* **eBPF (`bpf_dynptr`):** Uses opaque handles and bounds/read-only-checked slices so sandboxed bytecode can safely read and write host buffers without raw pointer arithmetic.
* **JavaScript `ArrayBuffer` / `DataView`:** Untyped byte blocks with zero-copy sub-views, though without built-in read-only views (currently explored via the TC39 immutable `ArrayBuffer` proposal).

## Key Tradeoffs & Open Challenges

Pushing this model beyond a prototype involves three core tradeoffs:

1. **Performance:** Handle-table lookups plus bounds and permission checks on every read/write are costly in an interpreter. Achieving near-native speed requires either a JIT that hoists and eliminates redundant checks (like HotSpot for `MemorySegment`) or hardware capability support (like CHERI).
2. **Lifetimes & Deallocation (`free`):** Once allocations can be freed, `make_span` introduces aliased views that must remain temporally safe. Solving this requires choosing a reclamation strategy such as reference counting, arenas/regions (like Panama's `Arena`), generational handles, or tracing GC.
3. **Isolation & Unforgeability:** While this prototype represents handles as plain `i32` values for simplicity, true cross-module isolation requires opaque, unforgeable handle types (similar to `externref` or per-module handle tables).
4. **Compiler Targets:** Standard LLVM backends assume pointers are plain integers in a flat address space, so targeting a pure handle+offset VM typically requires either a custom language frontend or a capability-aware compiler backend (such as CHERI-LLVM).

## Performance: Interpreter & JIT

With the right internal representation, this model can perform well in both an interpreter and a JIT/AOT compiler (as demonstrated by Java's `MemorySegment` and Rust slices), and can outperform standard Wasm Component Model ABIs on multi-module zero-copy workloads.

### Making It Fast in an Interpreter
* **Native memory opcodes:** Make `read`/`write`/`span` first-class VM opcodes rather than host function calls (avoiding per-access `std::function` dispatch and argument-vector allocations).
* **Fat handles on the value stack:** Carry `{ uint8_t* ptr; uint32_t size; uint32_t flags; }` directly on the operand stack so accesses perform a single bounds check and direct load instead of a handle-table lookup.
* **Static read-only types:** Encoding `ro_handle` vs. `rw_handle` in the type system eliminates runtime write-permission checks.
* **Bulk opcodes & pre-resolution:** Adding `mem.copy`/`mem.fill`/`mem.cmp` amortizes dispatch and bounds checks; pre-resolving local/function/branch targets plus threaded dispatch keeps interpreter overhead low.

### Making It Fast in a JIT / AOT Compiler
* **Scalar replacement (unboxing):** Keep `(ptr, size)` in CPU registers rather than in a handle table or memory struct.
* **Loop-Invariant Code Motion (LICM) & Range Check Elimination (RCE):** For loops over `handle[0..n]`, check `n <= handle.size` (and write permission) once before the loop and emit unchecked native loads/stores inside the loop.
* **Zero-copy cross-module calls:** Passing a span across module boundaries passes `(ptr, size)` in registers without copying.

### Where Flat Linear Memory Still Has an Edge
* **Random / data-dependent accesses:** Standard 64-bit Wasm JITs use 4–8 GiB MMU guard pages for 0-instruction bounds checks even on unpredictable offsets. Discrete objects/spans cannot use per-object guard pages, so random accesses pay ~1–2 instructions (compare + branch), though branch prediction keeps the overhead modest.
* **Handles stored in memory:** Storing 16-byte fat pointers inside structs reduces cache density, while storing compact 32-bit handle IDs requires a table lookup on dereference.
* **Temporal safety (`free` + `make_span`):** Arena-scoped lifetimes (like Panama's `Arena`) or tracing GC allow a JIT to safely hoist checks across loops. Arbitrary manual `free()` while spans exist forces liveness/generation re-checks across calls and inhibits aggressive optimization.
