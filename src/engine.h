/*
 * Hex Calc - programmer's calculator engine.
 *
 * Pure C, no GUI dependencies. All values are stored as raw bit patterns
 * in a uint64_t and masked to the active word size; signedness only
 * changes how a pattern is interpreted (two's complement).
 */
#ifndef HEXCALC_ENGINE_H
#define HEXCALC_ENGINE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BASE_BIN = 2,
    BASE_OCT = 8,
    BASE_DEC = 10,
    BASE_HEX = 16
} Base;

typedef enum {
    OP_NONE = 0,
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_MOD,
    OP_AND,
    OP_OR,
    OP_XOR,
    OP_NAND,
    OP_NOR,
    OP_SHL,
    OP_SHR,
    OP_ROL,
    OP_ROR,
    OP_COUNT
} BinOp;

typedef enum {
    UOP_NOT = 0,
    UOP_NEG,
    UOP_COUNT
} UnOp;

typedef enum {
    CALC_OK = 0,
    CALC_DIV_ZERO
} CalcStatus;

typedef enum {
    PARSE_OK = 0,
    PARSE_EMPTY,
    PARSE_INVALID,
    PARSE_RANGE
} ParseStatus;

#define ENGINE_EXPR_MAX 256
#define ENGINE_FORMAT_MAX 96 /* longest: 64 binary digits + 15 separators */

typedef struct {
    uint64_t value;        /* number currently shown */
    uint64_t acc;          /* left operand of a pending operation */
    BinOp pending;         /* operation waiting for its right operand */
    bool entering;         /* user is typing digits into value */
    bool entry_negative;   /* signed decimal entry has a leading minus */

    BinOp last_op;         /* for repeated '=' */
    uint64_t last_operand;

    bool have_eq;          /* last action was '=' (expression line shows it) */
    BinOp eq_op;
    uint64_t eq_a, eq_b;

    uint64_t memory;
    bool has_memory;

    int bits;              /* 8, 16, 32 or 64 */
    bool is_signed;
    Base base;

    bool error;
    char message[64];
} Engine;

typedef struct {
    char expr[ENGINE_EXPR_MAX];
    uint64_t result;
} EngineResult;

/* Helpers */
uint64_t engine_mask(int bits);
int64_t engine_sign_extend(uint64_t v, int bits);
const char *engine_op_symbol(BinOp op);
const char *engine_base_name(Base base);
bool engine_digit_valid(Base base, int digit);

uint64_t engine_apply(BinOp op, uint64_t a, uint64_t b, int bits, bool is_signed,
                      CalcStatus *status);
uint64_t engine_apply_unary(UnOp op, uint64_t a, int bits);

/* Formats v into buf. Returns the number of characters written. */
size_t engine_format(uint64_t v, Base base, int bits, bool is_signed, bool group,
                     char *buf, size_t n);

/* Parses user/clipboard text. Accepts 0x/0b/0o/# prefixes, an 'h' suffix,
 * a leading sign and digit separators (space, '_', ',', '\''). */
ParseStatus engine_parse(const char *s, Base base, int bits, bool is_signed,
                         uint64_t *out);

/* State machine */
void engine_init(Engine *e);
bool engine_input_digit(Engine *e, int digit);
void engine_backspace(Engine *e);
void engine_binop(Engine *e, BinOp op);
void engine_unop(Engine *e, UnOp op);
bool engine_equals(Engine *e, bool group, EngineResult *out);
void engine_clear(Engine *e);
void engine_clear_entry(Engine *e);
void engine_set_value(Engine *e, uint64_t v);
void engine_toggle_bit(Engine *e, int bit);
void engine_set_bits(Engine *e, int bits);
void engine_set_signed(Engine *e, bool is_signed);
void engine_set_base(Engine *e, Base base);

void engine_mem_store(Engine *e);
void engine_mem_recall(Engine *e);
void engine_mem_add(Engine *e);
void engine_mem_sub(Engine *e);
void engine_mem_clear(Engine *e);

/* Text for the small expression line above the main display. */
void engine_expression(const Engine *e, bool group, char *buf, size_t n);

#ifdef __cplusplus
}
#endif

#endif
