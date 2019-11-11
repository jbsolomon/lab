#include <assert.h>
#include <malloc.h>
#include <stdbool.h>
#include <stdio.h>

#include "morpha.h"

// Default logging buffer is 65K.
#define TEST_BUF_MEM (1 << 16)

// Default max number of tests is 100.
#define TEST_MAX 100

typedef enum {
    TEST_OK,
    TEST_FAILED,
    TEST_FATAL,
    TEST_UNIMPLEMENTED,
    TEST_OOM
} test_rt;

// test is a helper for test logging.
typedef struct test {
    // buf is logged to using test_log.
    char*       buf;
    char* const buf_max;

    // cursors is a list of logging buffer offsets.
    char* cursors[TEST_MAX];
    int   cursor;
} test;

void flush_test_logs(FILE* const to, test* t, int i) {
    assert(i < t->cursor);
    fprintf(to, "%s\n", t->cursors[i]);
}

#define test_log(t, ...)                                               \
    t->buf += snprintf(t->buf, t->buf_max - t->buf, __VA_ARGS__);      \
    if (t->buf >= t->buf_max - 1) {                                    \
	test_finish(t);                                                \
	return TEST_OOM;                                               \
    }

#define test_init(t)                                                   \
    t->cursors[t->cursor] = t->buf;                                    \
    test_log(t, "  Initializing test\n");

#define test_finish(t)                                                 \
    t->buf++;                                                          \
    t->cursor++;

#define test_check(t, rv, expr, ...)                                   \
    if (!(expr)) {                                                     \
	test_log(t, "  Check failed: %s\n", __VA_ARGS__);              \
	rv = TEST_FAILED;                                              \
    }

#define test_assert(t, expr, ...)                                      \
    if (!(expr)) {                                                     \
	test_log(t, "  Assertion failed: %s\n", __VA_ARGS__);          \
	test_finish(t);                                                \
	return TEST_FATAL;                                             \
    }

test_rt test_rt_init(test* t, MPH_runtime* rt, MPH_raw* raw,
                     size_t raw_size);
test_rt test_rt_cell(test* t, MPH_runtime* rt);
test_rt test_rt_morph(test* t, MPH_runtime* rt, MPH_result* ret);
test_rt test_rt_rule(test* t, MPH_runtime* rt, MPH_result* ret);
test_rt test_rt_step_morph(test* t, MPH_runtime* rt, size_t m_offs);
test_rt test_rt_step_rule(test* t, MPH_runtime* rt, size_t r_offs);

int main(int argc, char** argv) {
    MPH_raw     mem[MPH_DEFAULT_BLOCK];
    MPH_runtime r;
    MPH_result  ret;
    memset(&ret, 0, sizeof(MPH_result));

    char*       buf     = malloc(TEST_BUF_MEM);
    char* const buf_max = buf + TEST_BUF_MEM;

    char* cursors[TEST_MAX];

    test t = (test){ .buf = buf, .buf_max = buf_max, .cursors = cursors, .cursor = 0 };

    int total_failed = 0;
    int total_passed = 0;
    int total        = 0;

    struct {
	test_rt     result;
	const char* description;
	int         done;
    } tests[TEST_MAX] = {
	{ test_rt_init(&t, &r, mem, MPH_DEFAULT_BLOCK),
	  "Initialize runtime", false },
	{ test_rt_cell(&t, &r), "Define a new cell", false },
	{ test_rt_morph(&t, &r, &ret), "Create a new morph", false },
	{ test_rt_step_morph(&t, &r, (size_t)ret.data),
	  "Step through a morph", false },
	{ test_rt_rule(&t, &r, &ret), "Create a new rule", false },
	{ test_rt_step_rule(&t, &r, (size_t)ret.data),
	  "Step through a rule", false },
	{ TEST_OK, NULL, true }
    };

    int i = 0;
    for (i = 0; !tests[i].done; i++) {
	total++;

	char* result;

	switch (tests[i].result) {
	case TEST_OK:
	    result = "passed";
	    total_passed++;
	    break;
	case TEST_UNIMPLEMENTED:
	    result = "unimplemented";
	    break;
	case TEST_FAILED:
	    result = "failed";
	    total_failed++;
	    break;
	case TEST_FATAL:
	    result = "failed fatally";
	    break;
	case TEST_OOM:
	    result = "failed with logger OOM";
	    break;
	}

	fprintf(stderr, "Test %d %s: %s\n", i + 1, result,
	        tests[i].description);
	// Flush the test's logging buffer.
	flush_test_logs(stderr, &t, i);

	switch (tests[i].result) {
	case TEST_OOM:
	case TEST_FATAL:
	    return 1;
	}
    }

    fprintf(stderr, "%d passed, %d failed, %d total\n", total_passed,
            total_failed, total);

    free(buf);

    return total_failed == 0;
}

int test_rt_init(test* t, MPH_runtime* rt, MPH_raw* raw,
                 size_t raw_size) {
    test_init(t);

    MPH_rt_init(rt, raw, raw_size);

    test_rt rv = TEST_OK;

    test_check(t, rv, rt->pos == 0, "Position initialized to 0");
    test_check(t, rv, rt->raw == raw, "Memory initialized to raw");
    test_check(t, rv, rt->raw_size == raw_size,
               "Size initialized to raw_size");

    test_finish(t);
    return rv;
}

int test_rt_cell(test* t, MPH_runtime* rt) {
    test_init(t);

    test_finish(t);
    return TEST_UNIMPLEMENTED;
}

int test_rt_morph(test* t, MPH_runtime* rt, MPH_result* ret) {
    test_init(t);

    // Applying a morph to a full runtime results in an unchanged
    // runtime and appropriately-sized MPH_MEM_LOW.

    // Applying a morph to a runtime results in an otherwise unchanged
    // runtime.

    // A morph is a cell which first specifies the parameters it takes
    // as a list of sizes (size of list followed by each size.)

    test_finish(t);
    return TEST_UNIMPLEMENTED;
}
int test_rt_step_morph(test* t, MPH_runtime* rt, size_t m_offs) {
    test_init(t);

    test_finish(t);
    return TEST_UNIMPLEMENTED;
}

int test_rt_rule(test* t, MPH_runtime* rt, MPH_result* ret) {
    test_init(t);

    MPH_rule add = (MPH_rule){ (MPH_raw[]){ MPH_OP_ADD },
	                       (MPH_raw[]){ MPH_OP_ADD }, 1 };

    test_rt rv = TEST_OK;

    test_check(t, rv, MPH_rt_rule(rt, &add, ret)->t == MPH_OK,
               "MPH_rt_rule succeeds");

    test_finish(t);
    return rv;
}

int test_rt_step_rule(test* t, MPH_runtime* rt, size_t r_offs) {
    test_init(t);

    test_finish(t);
    return TEST_UNIMPLEMENTED;
}
