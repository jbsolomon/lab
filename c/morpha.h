#ifndef MPH_MORPHA_H
#define MPH_MORPHA_H

#include <stdint.h>
#include <string.h>

/*

morpha.h (prefix MPH) defines the core Morpha runtime, a tiny rewrite
system which follows the Morpha ruleset.  This is the boostrap version,
which implements only the simplest system possible to fulfill the Morpha
contract.  It is intended to be hand-verifiable, and to be suitable for
implementing things such as a regex engine, a native compiler, and a
higher-level language for proof systems.

The goal of this library is not to provide a full or efficient
implementation of Morpha, or a language/syntax implementation, but
instead to provide a portable and verifiable bootstrap runtime for
Morpha which can be defined using simple rules describing state
machines.  A Morpha CLI or higher-level library will use this library to
define a working stream consumer.

The core pieces of this library are MPH_runtime, MPH_morph, and
MPH_rule, along with the functions that use them.  Please see the
documentation of those structs for more information about their use.

In general, Morpha functions may be applied to a MPH_runtime in order to
construct a runnable Morpha composition.  Running the composition
requires the use of this library.

*/

/* Assume page size of 4k */
#define MPH_DEFAULT_BLOCK 0x00000200

/*
MPH_raw is the type of the underlying raw memory which Morpha uses to
define and run compositions.
*/
typedef uint64_t MPH_raw;

/* MPH_result_kind defines the possible types of MPH_result. */
typedef enum {
    /* MPH_OK indicates expected state, with optional function-specific
       state. */
    MPH_OK,
    /* MPH_HALT indicates that the runtime has completed operation.  The
       data value is the size_t offset of any return value.  All
       possible compositions eventually halt. */
    MPH_HALT,
    /* MPH_TOTALITY_FAULT indicates that a morph or rule would result in
       a non-total condition and cannot be used, with size_t offset of
       the problematic morph. */
    MPH_TOTALITY_FAULT,
    /* MPH_MEM_LOW indicates insufficient memory, with size_t value of
       required additional memory. */
    MPH_MEM_LOW,
} MPH_result_kind;

typedef struct {
    MPH_result_kind t;
    void*           data;
} MPH_result;

/*
MPH_runtime is a container for the core Morpha runtime state.  Functions
over this type are prefixed with MPH_rt.  To use MPH_runtime, you may
first initialize it with MPH_rt_init.

Functions which create values in MPH_runtime take an argument of
*MPH_result, and on success, will set its "data" member to the offset of
the new runtime value as a size_t value.  Offsets are used throughout
the runtime library to refer to runtime values.

Use MPH_morph and MPH_rule to define the control flow of a Morpha
composition (or "comp".)  Morphs define mutations or functions; rules
define control flow.  To evaluate a comp, use MPH_rt_step with the
offset of the morph you want to evaluate.  After evaluation begins,
MPH_runtime.pos will be updated with the offset of the runtime state.

Functionality such as naming and FFI (for heap allocation, for example)
may be defined in a helper "layer" or wrapper over MPH_runtime.

TODO:
- Define "helpers" / "layers"
- Define auditing / step logging
- Define name allocator layer
- Define JMP chaining (Rules)
- Define C FFI
- Define FFI allocators
- Define offsets, arg push, peek

Example:

MPH_raw mem[MPH_DEFAULT_BLOCK];
MPH_runtime rt;
MPH_result ret;

// Initialize the runtime.
MPH_rt_init(&rt, mem, MPH_DEFAULT_BLOCK);

// Create a new morph: add two numbers.  On success, ret.data will be
the offset of the new morph.
MPH_rt_morph(&rt, &MPH_morph{ .op = MPH_OP_ADD }, &ret);
assert(ret.t == MPH_OK);

// ret.data is now the offset of the new morph.  Set the runtime
// position to the new offset.
rt.pos = ret.data;

// Pass control to the runtime.
while(MPH_rt_step(&rt, &ret)->t != MPH_HALT) {
    switch(ret.t) {
    case MPH_MEM_LOW:
        // Reallocate / clone / etc.
        break;
    // Check for other cases?
    }
}
*/
typedef struct {
    MPH_raw* raw;
    size_t   raw_size;
    size_t   pos;
} MPH_runtime;

/*
MPH_op is a fundamental Morpha operator.  All morphs are either MPH_ops,
compositions of MPH_ops, or compositions of morphs.

Operators consume values following the operator in runtime memory.  For
example, MPH_OP_OFFSET consumes only the argument immediately following;
MPH_OP_ADD consumes the following two.
*/
typedef enum {
    MPH_OP_SUB,
    /* MPH_OP_ADD evaluates to the sum of the following two values.  It
       may be specialized by parameter type. */
    MPH_OP_ADD,
    /* MPH_OP_CMP implements comparison. */
    MPH_OP_CMP,
    /* MPH_OP_CMP implements jump. */
    MPH_OP_JMP,
    /* MPH_OP_OFFSET recalls the value stored at the following value,
       interpreted as an offset from zero. */
    MPH_OP_OFFSET,
} MPH_op;

/*
MPH_rt_init clears any existing state from the runtime and initializes
its fields with default values.  It uses the given block of MPH_raw
memory with size raw_size for storing Morpha runtime state.
*/
void MPH_rt_init(MPH_runtime* rt, MPH_raw* raw, size_t raw_size);

/*
MPH_cell defines Morpha values.  All values, including morphs and rules,
are defined using cells.  Cells may be accessed at runtime if desired,
but are only needed for defining the offsets of internal values.  In
other words, a cell is a sequence of offsets.
*/
typedef struct {
    size_t    size;
    MPH_raw** offsets;
} MPH_cell;

/*
MPH_rt_cell applies the given cell definition to the given runtime.
*/
MPH_result* MPH_rt_cell(MPH_runtime* rt, MPH_cell* c, MPH_result* ret);

/*
MPH_morph is the basic unit of execution of a Morpha composition.  It
defines how some memory may be changed, or some side-effect.

For example:

MPH_morph add = MPH_morph { .op = MPH_OP_ADD };

*/
typedef struct MPH_morph {
    // TODO: Figure out opcodes and how to combine them.
    MPH_op op;
    // TODO: Figure out how to pass arguments to morphs.
    size_t args;
} MPH_morph;

/*
MPH_rt_morph adds the given MPH_morph to the given MPH_runtime, with
MPH_result having .data set to the offset of the new morph, MPH_MEM_LOW
on memory exhaustion, or MPH_TOTALITY_FAILURE on an invalid morph.
*/
MPH_result* MPH_rt_morph(MPH_runtime* rt, MPH_morph* m,
                         MPH_result* ret);

/*
MPH_rule is the basic unit of control flow of a Morpha composition.  It
defines a choice of morphs under some conditions.  Each condition is a
morph, and conditions are evaluated in order until one is true.  When a
condition is true, the given morph is operated.

If none of the morphs evaluate to true, the default choice, given by _,
is operated.

To apply it to the runtime, see MPH_rt_rule.

An example rule, expressed in pseudocode, could be:
{
  gt{a, 0} => f1(a)
  and{ gt{a, 0}, lt{b, 0} } => f2(a, b)
  _ => g(a)
}

This could be expressed using this library as:

MPH_rule exm_rule = MPH_morph{
    .conds = { gt, and_gt_lt, MPH_NOOP },
    .morphs = { f1_a, f2_a_b, g_a },
};

assert(MPH_rt_rule(&rt, &exm_rule, &ret)->t == MPH_OK);

*/
typedef struct {
    MPH_raw* conds;
    MPH_raw* morphs;
    size_t   len;
} MPH_rule;

/*
MPH_rt_rule adds the given MPH_rule to the given runtime.  The resulting
MPH_result.data value is the offset of the new rule.

If there is insufficient runtime memory available, this will not mutate
the runtime, and instead will return MPH_LOW_MEM with the additional
memory size required (size_t) set as the MPH_result.data value.
*/
MPH_result* MPH_rt_rule(MPH_runtime* rt, MPH_rule* rule,
                        MPH_result* ret);

/*
MPH_rt_step steps the runtime execution forward by one unit.  On MPH_OK,
MPH_runtime.pos will be set to the next offset.  If the runtime requires
more memory, the state will be unchanged and MPH_result.t will be
MPH_MEM_LOW.  When the runtime is finished, MPH_result.t will be
MPH_HALT.
*/
MPH_result* MPH_rt_step(MPH_runtime* rt, MPH_result* ret);

/*
MPH_rt_exec iterates over MPH_rt_step until it reaches MPH_MEM_LOW or
MPH_HALT.
*/
MPH_result* MPH_rt_exec(MPH_runtime* rt, MPH_result* ret);

#endif // MPH_MORPHA_H
