#include "morpha.h"

void MPH_rt_init(MPH_runtime* rt, MPH_raw* raw, size_t raw_size) {
    memset(rt, 0, sizeof(MPH_runtime));

    rt->raw      = raw;
    rt->raw_size = raw_size;
}

MPH_result* MPH_rt_cell(MPH_runtime* rt, MPH_cell* c, MPH_result* ret) {
    ret->t = MPH_TOTALITY_FAULT;
    return ret;
}

MPH_result* MPH_rt_morph(MPH_runtime* rt, MPH_morph* m,
                         MPH_result* ret) {
    // TODO: Define control flow / execution / argument passing
    //
    // Ideas:
    // - Morphs dictate their own control flow; no morph, no control
    // flow.
    //
    // - Mutable "continuation object"?
    //
    // - What about pattern-matching?  Essentially, everything should
    // just be a cell, with some "eval" type operation.  Users don't
    // control execution, they just build struct values and trigger
    // evaluation.
    ret->t = MPH_TOTALITY_FAULT;
    return ret;
}

MPH_result* MPH_rt_rule(MPH_runtime* rt, MPH_rule* rule,
                        MPH_result* ret) {

    ret->data = (void*)rt->pos;
    ret->t    = MPH_OK;

    /* For each choice, we must define its arguments and the morph to be
       used.  When the rule is evaluated, each morph will be applied in
       sequence until one results in true.

       Example:

       {
          gt(a, 0) => f(a)
          _ => g(a)
       }

       becomes

       pick    # Consumes morphs until one is true or it reaches default
       gt      # Consumes following 2 arguments
       a
       0
       f       # If gt evaluated to true, evaluate f, consuming a
       a
       default # If nothing has evaluated to true yet, eval g, consume a
       g
       a

       // TODO: Define control flow / execution / argument passing
     */

    int i = 0;
    for (i = 0; i < rule->len; i++) {
	rt->pos += 0;
    }

    ret->t = MPH_TOTALITY_FAULT;
    return ret;
}

MPH_result* MPH_rt_step(MPH_runtime* rt, MPH_result* ret) {
    ret->t = MPH_HALT;
    return ret;
}

MPH_result* MPH_rt_exec(MPH_runtime* rt, MPH_result* ret) {
    ret->t = MPH_HALT;
    return ret;
}
