/*
 * Hex Calc - programmer's calculator engine.
 */
#include "engine.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

static const char DIGITS[] = "0123456789ABCDEF";

uint64_t engine_mask(int bits)
{
    if (bits >= 64)
        return UINT64_MAX;
    return (UINT64_C(1) << bits) - 1;
}

int64_t engine_sign_extend(uint64_t v, int bits)
{
    uint64_t m = engine_mask(bits);
    v &= m;
    if (bits < 64 && ((v >> (bits - 1)) & 1))
        v |= ~m;
    return (int64_t)v;
}

const char *engine_op_symbol(BinOp op)
{
    switch (op) {
    case OP_ADD:  return "+";
    case OP_SUB:  return "\xE2\x88\x92"; /* U+2212 minus */
    case OP_MUL:  return "\xC3\x97";     /* U+00D7 times */
    case OP_DIV:  return "\xC3\xB7";     /* U+00F7 divide */
    case OP_MOD:  return "mod";
    case OP_AND:  return "AND";
    case OP_OR:   return "OR";
    case OP_XOR:  return "XOR";
    case OP_NAND: return "NAND";
    case OP_NOR:  return "NOR";
    case OP_SHL:  return "<<";
    case OP_SHR:  return ">>";
    case OP_ROL:  return "ROL";
    case OP_ROR:  return "ROR";
    default:      return "";
    }
}

const char *engine_base_name(Base base)
{
    switch (base) {
    case BASE_BIN: return "BIN";
    case BASE_OCT: return "OCT";
    case BASE_DEC: return "DEC";
    default:       return "HEX";
    }
}

bool engine_digit_valid(Base base, int digit)
{
    return digit >= 0 && digit < (int)base;
}

static int clamp_bits(int bits)
{
    if (bits <= 8)
        return 8;
    if (bits <= 16)
        return 16;
    if (bits <= 32)
        return 32;
    return 64;
}

uint64_t engine_apply(BinOp op, uint64_t a, uint64_t b, int bits, bool is_signed,
                      CalcStatus *status)
{
    uint64_t m = engine_mask(bits);
    uint64_t r = 0;
    uint64_t s;

    a &= m;
    b &= m;
    if (status)
        *status = CALC_OK;

    switch (op) {
    case OP_ADD:
        r = a + b;
        break;
    case OP_SUB:
        r = a - b;
        break;
    case OP_MUL:
        r = a * b; /* low bits are identical for signed and unsigned */
        break;
    case OP_DIV:
    case OP_MOD:
        if (b == 0) {
            if (status)
                *status = CALC_DIV_ZERO;
            return 0;
        }
        if (is_signed) {
            int64_t sa = engine_sign_extend(a, bits);
            int64_t sb = engine_sign_extend(b, bits);
            if (sb == -1)  /* avoids INT64_MIN / -1 overflow; result wraps */
                r = (op == OP_DIV) ? (uint64_t)0 - a : 0;
            else
                r = (op == OP_DIV) ? (uint64_t)(sa / sb) : (uint64_t)(sa % sb);
        } else {
            r = (op == OP_DIV) ? a / b : a % b;
        }
        break;
    case OP_AND:
        r = a & b;
        break;
    case OP_OR:
        r = a | b;
        break;
    case OP_XOR:
        r = a ^ b;
        break;
    case OP_NAND:
        r = ~(a & b);
        break;
    case OP_NOR:
        r = ~(a | b);
        break;
    case OP_SHL:
        r = (b >= (uint64_t)bits) ? 0 : a << b;
        break;
    case OP_SHR: {
        bool negative = is_signed && ((a >> (bits - 1)) & 1);
        if (b >= (uint64_t)bits) {
            r = negative ? m : 0;
        } else {
            r = a >> b;
            if (negative && b > 0)
                r |= m & ~(m >> b); /* arithmetic shift: fill with sign */
        }
        break;
    }
    case OP_ROL:
        s = b % (uint64_t)bits;
        r = s ? (a << s) | (a >> (bits - s)) : a;
        break;
    case OP_ROR:
        s = b % (uint64_t)bits;
        r = s ? (a >> s) | (a << (bits - s)) : a;
        break;
    default:
        r = b;
        break;
    }
    return r & m;
}

uint64_t engine_apply_unary(UnOp op, uint64_t a, int bits)
{
    uint64_t m = engine_mask(bits);
    switch (op) {
    case UOP_NOT:
        return ~a & m;
    case UOP_NEG:
        return ((uint64_t)0 - a) & m;
    default:
        return a & m;
    }
}

size_t engine_format(uint64_t v, Base base, int bits, bool is_signed, bool group,
                     char *buf, size_t n)
{
    char digits[72];
    char out[ENGINE_FORMAT_MAX];
    int nd = 0, o = 0;
    bool negative = false;
    uint64_t mag = v & engine_mask(bits);
    int group_size = (base == BASE_DEC || base == BASE_OCT) ? 3 : 4;
    char sep = (base == BASE_DEC) ? ',' : ' ';

    if (n == 0)
        return 0;

    if (base == BASE_DEC && is_signed) {
        int64_t sv = engine_sign_extend(mag, bits);
        if (sv < 0) {
            negative = true;
            mag = (uint64_t)0 - (uint64_t)sv;
        }
    }

    do {
        digits[nd++] = DIGITS[mag % (uint64_t)base];
        mag /= (uint64_t)base;
    } while (mag);

    if (negative)
        out[o++] = '-';
    for (int i = nd - 1; i >= 0; i--) {
        out[o++] = digits[i];
        if (group && i > 0 && i % group_size == 0)
            out[o++] = sep;
    }
    out[o] = '\0';

    snprintf(buf, n, "%s", out);
    return strlen(buf);
}

static int digit_value(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    return -1;
}

/* Parses an unsigned magnitude. Returns PARSE_RANGE on 64-bit overflow. */
static ParseStatus parse_digits(const char *s, Base base, uint64_t *out)
{
    uint64_t v = 0;
    if (!*s)
        return PARSE_INVALID;
    for (; *s; s++) {
        int d = digit_value(*s);
        if (!engine_digit_valid(base, d))
            return PARSE_INVALID;
        if (v > (UINT64_MAX - (uint64_t)d) / (uint64_t)base)
            return PARSE_RANGE;
        v = v * (uint64_t)base + (uint64_t)d;
    }
    *out = v;
    return PARSE_OK;
}

ParseStatus engine_parse(const char *s, Base base, int bits, bool is_signed,
                         uint64_t *out)
{
    char clean[200];
    size_t len = 0;
    const char *p;
    bool negative = false;
    uint64_t mag = 0, m = engine_mask(bits);
    Base used = base;
    ParseStatus st;

    if (!s)
        return PARSE_EMPTY;

    for (p = s; *p; p++) {
        unsigned char c = (unsigned char)*p;
        if (isspace(c) || c == '_' || c == ',' || c == '\'')
            continue;
        if (len + 1 >= sizeof clean)
            return PARSE_RANGE;
        clean[len++] = (char)c;
    }
    clean[len] = '\0';
    if (len == 0)
        return PARSE_EMPTY;

    p = clean;
    if (*p == '-' || *p == '+') {
        negative = (*p == '-');
        p++;
    }

    if (p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) {
        used = BASE_HEX;
        p += 2;
    } else if (p[0] == '0' && (p[1] == 'o' || p[1] == 'O')) {
        used = BASE_OCT;
        p += 2;
    } else if (p[0] == '#' || p[0] == '$') {
        used = BASE_HEX;
        p += 1;
    } else if (p[0] == '0' && (p[1] == 'b' || p[1] == 'B')) {
        /* In hex mode "0B12" is a plain hex number, so only treat this as a
         * binary prefix when the rest really is binary. */
        uint64_t tmp;
        if (parse_digits(p + 2, BASE_BIN, &tmp) != PARSE_INVALID) {
            used = BASE_BIN;
            p += 2;
        }
    } else {
        size_t pl = strlen(p);
        if (pl > 1 && (p[pl - 1] == 'h' || p[pl - 1] == 'H')) {
            clean[(size_t)(p - clean) + pl - 1] = '\0';
            used = BASE_HEX;
        }
    }

    st = parse_digits(p, used, &mag);
    if (st != PARSE_OK)
        return st;

    if (negative) {
        /* Allow down to the most negative value of the word size. */
        uint64_t limit = is_signed ? (m >> 1) + 1 : m;
        if (mag > limit)
            return PARSE_RANGE;
        *out = ((uint64_t)0 - mag) & m;
    } else {
        uint64_t limit = (is_signed && used == BASE_DEC) ? (m >> 1) : m;
        if (mag > limit)
            return PARSE_RANGE;
        *out = mag;
    }
    return PARSE_OK;
}

/* ------------------------------------------------------------------ */

void engine_init(Engine *e)
{
    memset(e, 0, sizeof *e);
    e->bits = 64;
    e->is_signed = true;
    e->base = BASE_HEX;
}

static void set_error(Engine *e, const char *msg)
{
    e->error = true;
    snprintf(e->message, sizeof e->message, "%s", msg);
    e->pending = OP_NONE;
    e->entering = false;
    e->have_eq = false;
}

bool engine_input_digit(Engine *e, int digit)
{
    uint64_t m = engine_mask(e->bits);

    if (e->error)
        engine_clear(e);
    if (!engine_digit_valid(e->base, digit))
        return false;

    if (!e->entering) {
        e->value = 0;
        e->entering = true;
        e->entry_negative = false;
        e->have_eq = false;
    }

    if (e->base == BASE_DEC && e->is_signed) {
        uint64_t mag = e->entry_negative ? ((uint64_t)0 - e->value) & m : e->value;
        uint64_t limit = (m >> 1) + (e->entry_negative ? 1 : 0);
        if (mag > (limit - (uint64_t)digit) / 10)
            return false;
        mag = mag * 10 + (uint64_t)digit;
        e->value = e->entry_negative ? ((uint64_t)0 - mag) & m : mag;
    } else {
        if (e->value > (m - (uint64_t)digit) / (uint64_t)e->base)
            return false;
        e->value = e->value * (uint64_t)e->base + (uint64_t)digit;
    }
    return true;
}

void engine_backspace(Engine *e)
{
    uint64_t m = engine_mask(e->bits);

    if (e->error) {
        engine_clear(e);
        return;
    }
    if (!e->entering)
        return;

    if (e->base == BASE_DEC && e->is_signed && e->entry_negative) {
        uint64_t mag = ((uint64_t)0 - e->value) & m;
        mag /= 10;
        e->value = ((uint64_t)0 - mag) & m;
        if (mag == 0)
            e->entry_negative = false;
    } else {
        e->value /= (uint64_t)e->base;
    }
}

void engine_binop(Engine *e, BinOp op)
{
    if (e->error || op <= OP_NONE || op >= OP_COUNT)
        return;

    if (e->pending != OP_NONE) {
        if (!e->entering) {
            /* User changed their mind about the operator. */
            e->pending = op;
            e->have_eq = false;
            return;
        }
        CalcStatus st;
        uint64_t r = engine_apply(e->pending, e->acc, e->value, e->bits,
                                  e->is_signed, &st);
        if (st == CALC_DIV_ZERO) {
            set_error(e, "Cannot divide by zero");
            return;
        }
        e->value = r;
    }

    e->acc = e->value;
    e->pending = op;
    e->entering = false;
    e->entry_negative = false;
    e->have_eq = false;
}

void engine_unop(Engine *e, UnOp op)
{
    if (e->error)
        return;

    e->value = engine_apply_unary(op, e->value, e->bits);
    e->have_eq = false;

    if (op == UOP_NEG && e->entering && e->base == BASE_DEC && e->is_signed) {
        /* Keep typing: "5 ± 3" becomes -53. */
        e->entry_negative = !e->entry_negative;
        return;
    }
    e->entering = false;
    e->entry_negative = false;
}

bool engine_equals(Engine *e, bool group, EngineResult *out)
{
    BinOp op;
    uint64_t a, b, r;
    CalcStatus st;
    char fa[ENGINE_FORMAT_MAX], fb[ENGINE_FORMAT_MAX];

    if (e->error)
        return false;

    if (e->pending != OP_NONE) {
        op = e->pending;
        a = e->acc;
        b = e->value;
    } else if (e->last_op != OP_NONE) {
        op = e->last_op;
        a = e->value;
        b = e->last_operand;
    } else {
        return false;
    }

    r = engine_apply(op, a, b, e->bits, e->is_signed, &st);
    if (st == CALC_DIV_ZERO) {
        set_error(e, "Cannot divide by zero");
        return false;
    }

    if (out) {
        engine_format(a, e->base, e->bits, e->is_signed, group, fa, sizeof fa);
        engine_format(b, e->base, e->bits, e->is_signed, group, fb, sizeof fb);
        snprintf(out->expr, sizeof out->expr, "%s %s %s", fa, engine_op_symbol(op), fb);
        out->result = r;
    }

    e->eq_a = a;
    e->eq_b = b;
    e->eq_op = op;
    e->have_eq = true;
    e->last_op = op;
    e->last_operand = b;
    e->pending = OP_NONE;
    e->acc = r;
    e->value = r;
    e->entering = false;
    e->entry_negative = false;
    return true;
}

void engine_clear(Engine *e)
{
    e->value = 0;
    e->acc = 0;
    e->pending = OP_NONE;
    e->entering = false;
    e->entry_negative = false;
    e->last_op = OP_NONE;
    e->last_operand = 0;
    e->have_eq = false;
    e->error = false;
    e->message[0] = '\0';
}

void engine_clear_entry(Engine *e)
{
    if (e->error) {
        engine_clear(e);
        return;
    }
    e->value = 0;
    e->entering = false;
    e->entry_negative = false;
    e->have_eq = false;
}

void engine_set_value(Engine *e, uint64_t v)
{
    if (e->error)
        engine_clear(e);
    e->value = v & engine_mask(e->bits);
    e->entering = false;
    e->entry_negative = false;
    e->have_eq = false;
}

void engine_toggle_bit(Engine *e, int bit)
{
    if (bit < 0 || bit >= e->bits)
        return;
    if (e->error)
        engine_clear(e);
    e->value ^= UINT64_C(1) << bit;
    e->entering = false;
    e->entry_negative = false;
    e->have_eq = false;
}

void engine_set_bits(Engine *e, int bits)
{
    uint64_t m;

    e->bits = clamp_bits(bits);
    m = engine_mask(e->bits);
    e->value &= m;
    e->acc &= m;
    e->memory &= m;
    e->last_operand &= m;
    e->eq_a &= m;
    e->eq_b &= m;
    e->entering = false;
    e->entry_negative = false;
}

void engine_set_signed(Engine *e, bool is_signed)
{
    e->is_signed = is_signed;
    e->entering = false;
    e->entry_negative = false;
}

void engine_set_base(Engine *e, Base base)
{
    e->base = base;
    e->entering = false;
    e->entry_negative = false;
}

void engine_mem_store(Engine *e)
{
    if (e->error)
        return;
    e->memory = e->value;
    e->has_memory = true;
    e->entering = false;
}

void engine_mem_recall(Engine *e)
{
    if (!e->has_memory)
        return;
    engine_set_value(e, e->memory);
}

void engine_mem_add(Engine *e)
{
    if (e->error)
        return;
    e->memory = engine_apply(OP_ADD, e->memory, e->value, e->bits, e->is_signed, NULL);
    e->has_memory = true;
    e->entering = false;
}

void engine_mem_sub(Engine *e)
{
    if (e->error)
        return;
    e->memory = engine_apply(OP_SUB, e->memory, e->value, e->bits, e->is_signed, NULL);
    e->has_memory = true;
    e->entering = false;
}

void engine_mem_clear(Engine *e)
{
    e->memory = 0;
    e->has_memory = false;
}

void engine_expression(const Engine *e, bool group, char *buf, size_t n)
{
    char fa[ENGINE_FORMAT_MAX], fb[ENGINE_FORMAT_MAX];

    if (n == 0)
        return;
    buf[0] = '\0';
    if (e->error)
        return;

    if (e->pending != OP_NONE) {
        engine_format(e->acc, e->base, e->bits, e->is_signed, group, fa, sizeof fa);
        snprintf(buf, n, "%s %s", fa, engine_op_symbol(e->pending));
    } else if (e->have_eq) {
        engine_format(e->eq_a, e->base, e->bits, e->is_signed, group, fa, sizeof fa);
        engine_format(e->eq_b, e->base, e->bits, e->is_signed, group, fb, sizeof fb);
        snprintf(buf, n, "%s %s %s =", fa, engine_op_symbol(e->eq_op), fb);
    }
}
