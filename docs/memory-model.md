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
