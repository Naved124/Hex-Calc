/*
 * Unit tests for the calculator engine. Run with `make test`.
 */
#include "engine.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

static int failures, checks;

#define CHECK(cond)                                                          \
    do {                                                                     \
        checks++;                                                            \
        if (!(cond)) {                                                       \
            failures++;                                                      \
            fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, \
                    #cond);                                                  \
        }                                                                    \
    } while (0)

#define CHECK_U64(got, want)                                                  \
    do {                                                                      \
        uint64_t g_ = (got), w_ = (want);                                     \
        checks++;                                                             \
        if (g_ != w_) {                                                       \
            failures++;                                                       \
            fprintf(stderr, "%s:%d: %s = 0x%" PRIX64 ", expected 0x%" PRIX64 \
                    "\n", __FILE__, __LINE__, #got, g_, w_);                 \
        }                                                                     \
    } while (0)

#define CHECK_STR(got, want)                                                  \
    do {                                                                      \
        checks++;                                                             \
        if (strcmp((got), (want)) != 0) {                                     \
            failures++;                                                       \
            fprintf(stderr, "%s:%d: got \"%s\", expected \"%s\"\n", __FILE__, \
                    __LINE__, (got), (want));                                 \
        }                                                                     \
    } while (0)

static void type_digits(Engine *e, const char *s)
{
    for (; *s; s++) {
        int d = (*s <= '9') ? *s - '0' : (*s | 0x20) - 'a' + 10;
        engine_input_digit(e, d);
    }
}

static const char *fmt(uint64_t v, Base b, int bits, bool sign, bool group)
{
    static char buf[ENGINE_FORMAT_MAX];
    engine_format(v, b, bits, sign, group, buf, sizeof buf);
    return buf;
}

static void test_format(void)
{
    CHECK_STR(fmt(0, BASE_HEX, 64, true, true), "0");
    CHECK_STR(fmt(0xDEADBEEF, BASE_HEX, 64, true, true), "DEAD BEEF");
    CHECK_STR(fmt(0xDEADBEEF, BASE_HEX, 64, true, false), "DEADBEEF");
    CHECK_STR(fmt(0x1F, BASE_HEX, 64, true, true), "1F");
    CHECK_STR(fmt(1234567, BASE_DEC, 64, true, true), "1,234,567");
    CHECK_STR(fmt(255, BASE_DEC, 8, true, true), "-1");
    CHECK_STR(fmt(255, BASE_DEC, 8, false, true), "255");
    CHECK_STR(fmt(0x80, BASE_DEC, 8, true, true), "-128");
    CHECK_STR(fmt(UINT64_C(0x8000000000000000), BASE_DEC, 64, true, false),
              "-9223372036854775808");
    CHECK_STR(fmt(UINT64_MAX, BASE_DEC, 64, false, false), "18446744073709551615");
    CHECK_STR(fmt(10, BASE_BIN, 8, false, true), "1010");
    CHECK_STR(fmt(0xA5, BASE_BIN, 8, false, true), "1010 0101");
    CHECK_STR(fmt(0777, BASE_OCT, 16, false, true), "777");
    CHECK_STR(fmt(01234, BASE_OCT, 16, false, true), "1 234");
    CHECK_STR(fmt(UINT64_MAX, BASE_BIN, 64, false, true),
              "1111 1111 1111 1111 1111 1111 1111 1111 "
              "1111 1111 1111 1111 1111 1111 1111 1111");
}

static void test_arithmetic(void)
{
    CalcStatus st;

    CHECK_U64(engine_apply(OP_ADD, 0xFF, 1, 8, false, &st), 0);
    CHECK_U64(engine_apply(OP_SUB, 0, 1, 16, false, &st), 0xFFFF);
    CHECK_U64(engine_apply(OP_MUL, 0x10, 0x10, 8, false, &st), 0);
    CHECK_U64(engine_apply(OP_MUL, 6, 7, 64, true, &st), 42);
    CHECK_U64(engine_apply(OP_DIV, 100, 7, 32, false, &st), 14);
    CHECK_U64(engine_apply(OP_MOD, 100, 7, 32, false, &st), 2);

    /* Signed division truncates toward zero. */
    CHECK_U64(engine_apply(OP_DIV, (uint64_t)-7 & 0xFF, 2, 8, true, &st), (uint64_t)-3 & 0xFF);
    CHECK_U64(engine_apply(OP_MOD, (uint64_t)-7 & 0xFF, 2, 8, true, &st), (uint64_t)-1 & 0xFF);
    /* Unsigned interpretation of the same bits. */
    CHECK_U64(engine_apply(OP_DIV, 0xF9, 2, 8, false, &st), 0x7C);
    /* INT_MIN / -1 wraps instead of trapping. */
    CHECK_U64(engine_apply(OP_DIV, 0x80, 0xFF, 8, true, &st), 0x80);
    CHECK_U64(engine_apply(OP_DIV, UINT64_C(1) << 63, UINT64_MAX, 64, true, &st),
              UINT64_C(1) << 63);
    CHECK_U64(engine_apply(OP_MOD, UINT64_C(1) << 63, UINT64_MAX, 64, true, &st), 0);

    engine_apply(OP_DIV, 5, 0, 64, true, &st);
    CHECK(st == CALC_DIV_ZERO);
    engine_apply(OP_MOD, 5, 0, 64, false, &st);
    CHECK(st == CALC_DIV_ZERO);
}

static void test_bitwise(void)
{
    CHECK_U64(engine_apply(OP_AND, 0xF0F0, 0xFF00, 16, false, NULL), 0xF000);
    CHECK_U64(engine_apply(OP_OR, 0xF0F0, 0x0F00, 16, false, NULL), 0xFFF0);
    CHECK_U64(engine_apply(OP_XOR, 0xFF, 0x0F, 8, false, NULL), 0xF0);
    CHECK_U64(engine_apply(OP_NAND, 0xFF, 0x0F, 8, false, NULL), 0xF0);
    CHECK_U64(engine_apply(OP_NOR, 0xF0, 0x0F, 8, false, NULL), 0x00);
    CHECK_U64(engine_apply_unary(UOP_NOT, 0, 8), 0xFF);
    CHECK_U64(engine_apply_unary(UOP_NOT, 0, 64), UINT64_MAX);
    CHECK_U64(engine_apply_unary(UOP_NEG, 1, 16), 0xFFFF);
    CHECK_U64(engine_apply_unary(UOP_NEG, 0, 16), 0);
}

static void test_shifts(void)
{
    CHECK_U64(engine_apply(OP_SHL, 1, 4, 8, false, NULL), 0x10);
    CHECK_U64(engine_apply(OP_SHL, 1, 8, 8, false, NULL), 0);
    CHECK_U64(engine_apply(OP_SHL, 1, 63, 64, false, NULL), UINT64_C(1) << 63);
    CHECK_U64(engine_apply(OP_SHL, 1, 64, 64, false, NULL), 0);
    CHECK_U64(engine_apply(OP_SHR, 0x80, 7, 8, false, NULL), 1);
    /* Arithmetic shift in signed mode. */
    CHECK_U64(engine_apply(OP_SHR, 0x80, 1, 8, true, NULL), 0xC0);
    CHECK_U64(engine_apply(OP_SHR, 0x80, 9, 8, true, NULL), 0xFF);
    CHECK_U64(engine_apply(OP_SHR, 0x40, 1, 8, true, NULL), 0x20);
    CHECK_U64(engine_apply(OP_SHR, UINT64_MAX, 63, 64, true, NULL), UINT64_MAX);
    /* Rotates. */
    CHECK_U64(engine_apply(OP_ROL, 0x81, 1, 8, false, NULL), 0x03);
    CHECK_U64(engine_apply(OP_ROR, 0x81, 1, 8, false, NULL), 0xC0);
    CHECK_U64(engine_apply(OP_ROL, 0x12, 8, 8, false, NULL), 0x12);
    CHECK_U64(engine_apply(OP_ROL, UINT64_C(0x8000000000000001), 4, 64, false, NULL),
              UINT64_C(0x18));
    CHECK_U64(engine_apply(OP_ROR, 1, 1, 32, false, NULL), 0x80000000);
}

static void test_parse(void)
{
    uint64_t v = 0;

    CHECK(engine_parse("0xFF", BASE_DEC, 64, true, &v) == PARSE_OK);
    CHECK_U64(v, 0xFF);
    CHECK(engine_parse("dead_beef", BASE_HEX, 64, true, &v) == PARSE_OK);
    CHECK_U64(v, 0xDEADBEEF);
    CHECK(engine_parse("0b1010", BASE_DEC, 64, true, &v) == PARSE_OK);
    CHECK_U64(v, 10);
    CHECK(engine_parse("0B12", BASE_HEX, 64, true, &v) == PARSE_OK);
    CHECK_U64(v, 0xB12);
    CHECK(engine_parse("0o17", BASE_HEX, 64, true, &v) == PARSE_OK);
    CHECK_U64(v, 15);
    CHECK(engine_parse("1Fh", BASE_DEC, 64, true, &v) == PARSE_OK);
    CHECK_U64(v, 0x1F);
    CHECK(engine_parse("#ff00ff", BASE_DEC, 32, false, &v) == PARSE_OK);
    CHECK_U64(v, 0xFF00FF);
    CHECK(engine_parse("1,000,000", BASE_DEC, 32, false, &v) == PARSE_OK);
    CHECK_U64(v, 1000000);
    CHECK(engine_parse("  42\n", BASE_DEC, 8, false, &v) == PARSE_OK);
    CHECK_U64(v, 42);
    CHECK(engine_parse("-1", BASE_DEC, 16, true, &v) == PARSE_OK);
    CHECK_U64(v, 0xFFFF);
    CHECK(engine_parse("-128", BASE_DEC, 8, true, &v) == PARSE_OK);
    CHECK_U64(v, 0x80);
    CHECK(engine_parse("-129", BASE_DEC, 8, true, &v) == PARSE_RANGE);
    CHECK(engine_parse("128", BASE_DEC, 8, true, &v) == PARSE_RANGE);
    CHECK(engine_parse("0x80", BASE_DEC, 8, true, &v) == PARSE_OK);
    CHECK(engine_parse("256", BASE_DEC, 8, false, &v) == PARSE_RANGE);
    CHECK(engine_parse("18446744073709551615", BASE_DEC, 64, false, &v) == PARSE_OK);
    CHECK_U64(v, UINT64_MAX);
    CHECK(engine_parse("18446744073709551616", BASE_DEC, 64, false, &v) == PARSE_RANGE);
    CHECK(engine_parse("12G", BASE_HEX, 64, true, &v) == PARSE_INVALID);
    CHECK(engine_parse("102", BASE_BIN, 64, true, &v) == PARSE_INVALID);
    CHECK(engine_parse("   ", BASE_HEX, 64, true, &v) == PARSE_EMPTY);
    CHECK(engine_parse("0x", BASE_HEX, 64, true, &v) == PARSE_INVALID);
}

static void test_digit_entry(void)
{
    Engine e;
    engine_init(&e);

    engine_set_base(&e, BASE_HEX);
    type_digits(&e, "dead");
    CHECK_U64(e.value, 0xDEAD);

    /* Digits that don't exist in the current base are rejected. */
    engine_clear(&e);
    engine_set_base(&e, BASE_OCT);
    CHECK(!engine_input_digit(&e, 8));
    CHECK(engine_input_digit(&e, 7));
    CHECK_U64(e.value, 7);

    /* Entry stops at the word size. */
    engine_clear(&e);
    engine_set_bits(&e, 8);
    engine_set_base(&e, BASE_HEX);
    type_digits(&e, "fff");
    CHECK_U64(e.value, 0xFF);

    engine_clear(&e);
    engine_set_signed(&e, true);
    engine_set_base(&e, BASE_DEC);
    type_digits(&e, "1279");
    CHECK_U64(e.value, 127);

    engine_clear(&e);
    engine_set_signed(&e, false);
    engine_set_base(&e, BASE_DEC);
    type_digits(&e, "2559");
    CHECK_U64(e.value, 255);

    /* Negative decimal entry. */
    engine_clear(&e);
    engine_set_signed(&e, true);
    engine_set_base(&e, BASE_DEC);
    engine_input_digit(&e, 1);
    engine_unop(&e, UOP_NEG);
    type_digits(&e, "28");
    CHECK_U64(e.value, 0x80); /* -128 */
    CHECK(!engine_input_digit(&e, 0));
    engine_backspace(&e);
    CHECK_U64(e.value, (uint64_t)-12 & 0xFF);

    engine_set_bits(&e, 64);
    engine_clear(&e);
    engine_set_base(&e, BASE_HEX);
    type_digits(&e, "ffffffffffffffff");
    CHECK_U64(e.value, UINT64_MAX);
    CHECK(!engine_input_digit(&e, 0));
    engine_backspace(&e);
    CHECK_U64(e.value, UINT64_C(0x0FFFFFFFFFFFFFFF));
}

static void test_state_machine(void)
{
    Engine e;
    EngineResult r;
    char expr[ENGINE_EXPR_MAX];

    engine_init(&e);
    engine_set_base(&e, BASE_DEC);

    type_digits(&e, "5");
    engine_binop(&e, OP_ADD);
    engine_expression(&e, true, expr, sizeof expr);
    CHECK_STR(expr, "5 +");
    type_digits(&e, "3");
    CHECK(engine_equals(&e, true, &r));
    CHECK_U64(r.result, 8);
    CHECK_STR(r.expr, "5 + 3");
    engine_expression(&e, true, expr, sizeof expr);
    CHECK_STR(expr, "5 + 3 =");

    /* Repeated '=' repeats the last operation. */
    CHECK(engine_equals(&e, true, &r));
    CHECK_U64(e.value, 11);

    /* Chaining evaluates left to right. */
    engine_clear(&e);
    type_digits(&e, "2");
    engine_binop(&e, OP_ADD);
    type_digits(&e, "3");
    engine_binop(&e, OP_MUL);
    CHECK_U64(e.value, 5);
    type_digits(&e, "4");
    engine_equals(&e, true, &r);
    CHECK_U64(e.value, 20);

    /* Changing the operator before typing the operand. */
    engine_clear(&e);
    type_digits(&e, "9");
    engine_binop(&e, OP_ADD);
    engine_binop(&e, OP_SUB);
    type_digits(&e, "4");
    engine_equals(&e, true, &r);
    CHECK_U64(e.value, 5);

    /* '=' with nothing to do. */
    engine_clear(&e);
    CHECK(!engine_equals(&e, true, &r));

    /* Division by zero puts the engine in an error state. */
    engine_clear(&e);
    type_digits(&e, "7");
    engine_binop(&e, OP_DIV);
    type_digits(&e, "0");
    CHECK(!engine_equals(&e, true, &r));
    CHECK(e.error);
    CHECK(strlen(e.message) > 0);
    engine_binop(&e, OP_ADD); /* ignored while in error */
    CHECK(e.error);
    type_digits(&e, "3"); /* typing recovers */
    CHECK(!e.error);
    CHECK_U64(e.value, 3);

    /* Unary ops apply to the current value. */
    engine_clear(&e);
    engine_set_base(&e, BASE_HEX);
    engine_set_bits(&e, 8);
    type_digits(&e, "f0");
    engine_unop(&e, UOP_NOT);
    CHECK_U64(e.value, 0x0F);

    /* Shrinking the word size truncates. */
    engine_set_bits(&e, 64);
    engine_set_value(&e, 0x12345);
    engine_set_bits(&e, 16);
    CHECK_U64(e.value, 0x2345);

    /* Bit toggling. */
    engine_set_value(&e, 0);
    engine_toggle_bit(&e, 15);
    CHECK_U64(e.value, 0x8000);
    engine_toggle_bit(&e, 16); /* outside the word, ignored */
    CHECK_U64(e.value, 0x8000);

    /* CE keeps the pending operation. */
    engine_clear(&e);
    engine_set_base(&e, BASE_DEC);
    type_digits(&e, "10");
    engine_binop(&e, OP_SUB);
    type_digits(&e, "99");
    engine_clear_entry(&e);
    type_digits(&e, "4");
    engine_equals(&e, true, &r);
    CHECK_U64(e.value, 6);
}

static void test_memory(void)
{
    Engine e;
    engine_init(&e);
    engine_set_base(&e, BASE_DEC);

    type_digits(&e, "10");
    engine_mem_store(&e);
    CHECK(e.has_memory);
    type_digits(&e, "5");
    engine_mem_add(&e);
    CHECK_U64(e.memory, 15);
    type_digits(&e, "20");
    engine_mem_sub(&e);
    CHECK_U64(e.memory, (uint64_t)-5);
    engine_clear(&e);
    engine_mem_recall(&e);
    CHECK_U64(e.value, (uint64_t)-5);
    engine_mem_clear(&e);
    CHECK(!e.has_memory);
}

int main(void)
{
    test_format();
    test_arithmetic();
    test_bitwise();
    test_shifts();
    test_parse();
    test_digit_entry();
    test_state_machine();
    test_memory();

    if (failures) {
        fprintf(stderr, "%d of %d checks failed\n", failures, checks);
        return 1;
    }
    printf("All %d checks passed\n", checks);
    return 0;
}
