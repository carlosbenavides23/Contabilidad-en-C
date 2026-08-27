#include "formulas.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#define RAW_RANGE "Transacciones!$A$2:$A$500"
#define TAX_CODE_RANGE "Catalogo_Tasas!$A$2:$A$20"
#define TAX_RATE_RANGE "Catalogo_Tasas!$B$2:$B$20"
#define WITHHOLDING_RANGE "Catalogo_Tasas!$C$2:$C$20"
#define BUDGET_CENTER_RANGE "Topes_Presupuesto!$A$2:$A$10"
#define BUDGET_LIMIT_RANGE "Topes_Presupuesto!$B$2:$B$10"
#define BUDGET_PENALTY_RANGE "Topes_Presupuesto!$C$2:$C$10"

static char *allocate_printf(const char *format, ...)
{
    va_list arguments;
    va_list arguments_copy;
    int required;
    char *result;

    va_start(arguments, format);
    va_copy(arguments_copy, arguments);
    required = vsnprintf(NULL, 0U, format, arguments_copy);
    va_end(arguments_copy);

    if (required < 0) {
        va_end(arguments);
        return NULL;
    }

    result = malloc((size_t)required + 1U);
    if (result != NULL) {
        const int written = vsnprintf(result,
                                      (size_t)required + 1U,
                                      format,
                                      arguments);
        if (written != required) {
            free(result);
            result = NULL;
        }
    }
    va_end(arguments);
    return result;
}

/*
 * The delimiter positions stay in the XLSX formula; C never parses a raw row.
 * p1 = FIND("#", raw)
 * p2 = FIND("#", raw, p1 + 1)
 * p3 = FIND("#", raw, p2 + 1)
 * p4 = FIND("#", raw, p3 + 1)
 */
static char *build_hash_position(const char *raw_reference, unsigned int ordinal)
{
    char *position;
    unsigned int current;

    if (raw_reference == NULL || ordinal == 0U) {
        return NULL;
    }

    position = allocate_printf("FIND(\"#\",%s)", raw_reference);
    for (current = 2U; position != NULL && current <= ordinal; ++current) {
        char *next = allocate_printf("FIND(\"#\",%s,%s+1)",
                                     raw_reference,
                                     position);
        free(position);
        position = next;
    }
    return position;
}

char *build_amount_expression(const char *raw_reference)
{
    char *third = build_hash_position(raw_reference, 3U);
    char *fourth = build_hash_position(raw_reference, 4U);
    char *expression;

    if (third == NULL || fourth == NULL) {
        free(third);
        free(fourth);
        return NULL;
    }

    /* Amount is the text strictly between p3 and p4, converted by VALUE. */
    expression = allocate_printf("IFERROR(VALUE(MID(%s,%s+1,%s-%s-1)),0)",
                                 raw_reference,
                                 third,
                                 fourth,
                                 third);
    free(third);
    free(fourth);
    return expression;
}

char *build_tax_code_expression(const char *raw_reference)
{
    char *fourth = build_hash_position(raw_reference, 4U);
    char *expression;

    if (fourth == NULL) {
        return NULL;
    }

    /* Tax code runs from p4 + 1 through the end of the raw string. */
    expression = allocate_printf("IFERROR(MID(%s,%s+1,LEN(%s)-%s),\"\")",
                                 raw_reference,
                                 fourth,
                                 raw_reference,
                                 fourth);
    free(fourth);
    return expression;
}

char *build_cost_center_expression(const char *raw_reference)
{
    char *second = build_hash_position(raw_reference, 2U);
    char *third = build_hash_position(raw_reference, 3U);
    char *expression;

    if (second == NULL || third == NULL) {
        free(second);
        free(third);
        return NULL;
    }

    expression = allocate_printf("IFERROR(MID(%s,%s+1,%s-%s-1),\"\")",
                                 raw_reference,
                                 second,
                                 third,
                                 second);
    free(second);
    free(third);
    return expression;
}

char *build_date_expression(const char *raw_reference)
{
    char *first = build_hash_position(raw_reference, 1U);
    char *second = build_hash_position(raw_reference, 2U);
    char *expression;

    if (first == NULL || second == NULL) {
        free(first);
        free(second);
        return NULL;
    }

    expression = allocate_printf("IFERROR(MID(%s,%s+1,%s-%s-1),\"\")",
                                 raw_reference,
                                 first,
                                 second,
                                 first);
    free(first);
    free(second);
    return expression;
}

char *build_id_expression(const char *raw_reference)
{
    char *first = build_hash_position(raw_reference, 1U);
    char *expression;

    if (first == NULL) {
        return NULL;
    }

    expression = allocate_printf("IFERROR(MID(%s,1,%s-1),\"\")",
                                 raw_reference,
                                 first);
    free(first);
    return expression;
}

static char *build_year_expression(const char *raw_reference)
{
    char *date = build_date_expression(raw_reference);
    char *expression;

    if (date == NULL) {
        return NULL;
    }
    expression = allocate_printf("IFERROR(VALUE(MID(%s,1,4)),0)", date);
    free(date);
    return expression;
}

static char *build_month_expression(const char *raw_reference)
{
    char *date = build_date_expression(raw_reference);
    char *expression;

    if (date == NULL) {
        return NULL;
    }
    expression = allocate_printf("IFERROR(VALUE(MID(%s,6,2)),0)", date);
    free(date);
    return expression;
}

static char *build_catalog_lookup(const char *tax_code,
                                  const char *result_range)
{
    return allocate_printf("IFERROR(INDEX(%s,MATCH(%s,%s,0)),0)",
                           result_range,
                           tax_code,
                           TAX_CODE_RANGE);
}

char *build_net_amount_formula(void)
{
    char *amount = build_amount_expression(RAW_RANGE);
    char *tax_code = build_tax_code_expression(RAW_RANGE);
    char *rate;
    char *withholding;
    char *formula;

    if (amount == NULL || tax_code == NULL) {
        free(amount);
        free(tax_code);
        return NULL;
    }
    rate = build_catalog_lookup(tax_code, TAX_RATE_RANGE);
    withholding = build_catalog_lookup(tax_code, WITHHOLDING_RANGE);
    if (rate == NULL || withholding == NULL) {
        free(amount);
        free(tax_code);
        free(rate);
        free(withholding);
        return NULL;
    }

    formula = allocate_printf("{=IF(%s<>\"\",%s*(1+%s)-(%s*%s),\"\")}",
                              RAW_RANGE,
                              amount,
                              rate,
                              amount,
                              withholding);
    free(amount);
    free(tax_code);
    free(rate);
    free(withholding);
    return formula;
}

static char *build_monthly_sum_expression(const char *cost_center_reference,
                                          const char *month_reference)
{
    char *amount = build_amount_expression(RAW_RANGE);
    char *center = build_cost_center_expression(RAW_RANGE);
    char *year = build_year_expression(RAW_RANGE);
    char *month = build_month_expression(RAW_RANGE);
    char *expression;

    if (amount == NULL || center == NULL || year == NULL || month == NULL) {
        free(amount);
        free(center);
        free(year);
        free(month);
        return NULL;
    }

    expression = allocate_printf("SUMPRODUCT((%s=$B$1)*(%s=%s)*(%s=%s)*%s)",
                                 year,
                                 month,
                                 month_reference,
                                 center,
                                 cost_center_reference,
                                 amount);
    free(amount);
    free(center);
    free(year);
    free(month);
    return expression;
}

char *build_consolidated_formula(const char *cost_center_reference)
{
    char *amount = build_amount_expression(RAW_RANGE);
    char *center = build_cost_center_expression(RAW_RANGE);
    char *year = build_year_expression(RAW_RANGE);
    char *month = build_month_expression(RAW_RANGE);
    char *formula;

    if (amount == NULL || center == NULL || year == NULL || month == NULL) {
        free(amount);
        free(center);
        free(year);
        free(month);
        return NULL;
    }

    formula = allocate_printf(
        "=SUMPRODUCT((%s=$B$1)*(%s=$B$2)*(%s=%s)*"
        "(%s>(SUMPRODUCT((%s<>\"\")*%s)/SUMPRODUCT(--(%s<>\"\"))))*%s)",
        year,
        month,
        center,
        cost_center_reference,
        amount,
        RAW_RANGE,
        amount,
        RAW_RANGE,
        amount);
    free(amount);
    free(center);
    free(year);
    free(month);
    return formula;
}

static char *build_budget_lookup(const char *cost_center_reference,
                                 const char *result_range)
{
    return allocate_printf("IFERROR(INDEX(%s,MATCH(%s,%s,0)),0)",
                           result_range,
                           cost_center_reference,
                           BUDGET_CENTER_RANGE);
}

char *build_budget_formula(const char *cost_center_reference,
                           const char *month_reference)
{
    char *monthly_sum = build_monthly_sum_expression(cost_center_reference,
                                                      month_reference);
    char *limit = build_budget_lookup(cost_center_reference, BUDGET_LIMIT_RANGE);
    char *penalty = build_budget_lookup(cost_center_reference,
                                        BUDGET_PENALTY_RANGE);
    char *formula;

    if (monthly_sum == NULL || limit == NULL || penalty == NULL) {
        free(monthly_sum);
        free(limit);
        free(penalty);
        return NULL;
    }

    formula = allocate_printf(
        "=IF(%s<=%s,TEXT((%s-%s)/%s,\"0.00%%\"),"
        "(%s-%s)*(1+%s)^2+(((%s-%s)^2/%s)*LN(1+%s)))",
        monthly_sum,
        limit,
        limit,
        monthly_sum,
        limit,
        monthly_sum,
        limit,
        penalty,
        monthly_sum,
        limit,
        limit,
        penalty);
    free(monthly_sum);
    free(limit);
    free(penalty);
    return formula;
}

static char *build_top_output_expression(TopField field)
{
    switch (field) {
        case TOP_FIELD_ID:
            return build_id_expression(RAW_RANGE);
        case TOP_FIELD_AMOUNT:
            return build_amount_expression(RAW_RANGE);
        case TOP_FIELD_DATE:
            return build_date_expression(RAW_RANGE);
        case TOP_FIELD_COST_CENTER:
            return build_cost_center_expression(RAW_RANGE);
        case TOP_FIELD_TAX_CODE:
            return build_tax_code_expression(RAW_RANGE);
        default:
            return NULL;
    }
}

char *build_top5_formula(TopField field)
{
    char *amount = build_amount_expression(RAW_RANGE);
    char *output = build_top_output_expression(field);
    char *rank_vector;
    char *position;
    char *formula;

    if (amount == NULL || output == NULL) {
        free(amount);
        free(output);
        return NULL;
    }

    rank_vector = allocate_printf(
        "%s+(ROW(%s)-ROW(Transacciones!$A$2)+1)/1000000",
        amount,
        RAW_RANGE);
    if (rank_vector == NULL) {
        free(amount);
        free(output);
        return NULL;
    }

    position = allocate_printf("MATCH(LARGE(%s,ROW()-1),%s,0)",
                               rank_vector,
                               rank_vector);
    if (position == NULL) {
        free(amount);
        free(output);
        free(rank_vector);
        return NULL;
    }

    formula = allocate_printf("{=INDEX(%s,%s)}", output, position);
    free(amount);
    free(output);
    free(rank_vector);
    free(position);
    return formula;
}

char *build_determinant_formula(void)
{
    return allocate_printf(
        "=IF(ABS(MDETERM($A$1:$D$4))<1E-12,"
        "\"MATRIZ_SINGULAR\",MDETERM($A$1:$D$4))");
}

char *build_annual_deviation_formula(size_t consolidated_excel_row)
{
    return allocate_printf("=SUM(Consolidado!B%zu:M%zu)",
                           consolidated_excel_row,
                           consolidated_excel_row);
}

char *build_linear_solution_formula(void)
{
    return allocate_printf(
        "{=IF(ABS(MDETERM($A$1:$D$4))<1E-12,\"MATRIZ_SINGULAR\","
        "IFERROR(MMULT(MINVERSE($A$1:$D$4),$E$1:$E$4),"
        "\"MATRIZ_SINGULAR\"))}");
}

/*
 * This intentionally addresses fixed offsets inside the date field, but the
 * parsing still happens in the formula. IFERROR maps corrupt dates to "".
 */
static char *build_date_serial_expression(const char *raw_reference)
{
    return allocate_printf(
        "IFERROR(DATE(VALUE(MID(%s,FIND(\"#\",%s)+1,4)),"
        "VALUE(MID(%s,FIND(\"#\",%s)+6,2)),"
        "VALUE(MID(%s,FIND(\"#\",%s)+9,2))),\"\")",
        raw_reference,
        raw_reference,
        raw_reference,
        raw_reference,
        raw_reference,
        raw_reference);
}

static char *build_date_valid_expression(const char *raw_reference,
                                         const char *date_serial)
{
    return allocate_printf(
        "IFERROR((MID(%s,FIND(\"#\",%s)+5,1)=\"-\")*"
        "(MID(%s,FIND(\"#\",%s)+8,1)=\"-\")*"
        "(MID(%s,FIND(\"#\",%s)+11,1)=\"#\")*ISNUMBER(%s)*"
        "(TEXT(%s,\"yyyy-mm-dd\")=MID(%s,FIND(\"#\",%s)+1,10)),FALSE)",
        raw_reference,
        raw_reference,
        raw_reference,
        raw_reference,
        raw_reference,
        raw_reference,
        date_serial,
        date_serial,
        raw_reference,
        raw_reference);
}

static char *build_previous_valid_position(const char *valid_date_vector)
{
    return allocate_printf(
        "MATCH(2,1/((%s)*(ROW(%s)<ROW())))",
        valid_date_vector,
        RAW_RANGE);
}

static char *build_next_valid_position(const char *valid_date_vector)
{
    return allocate_printf(
        "MATCH(1,(%s)*(ROW(%s)>ROW()),0)",
        valid_date_vector,
        RAW_RANGE);
}

char *build_temporal_reconstruction_formula(size_t excel_row)
{
    char *current_raw = allocate_printf("Transacciones!A%zu", excel_row);
    char *current_serial;
    char *serial_vector = build_date_serial_expression(RAW_RANGE);
    char *current_valid;
    char *valid_vector;
    char *previous_position;
    char *next_position;
    char *previous_date;
    char *next_date;
    char *formula;

    if (current_raw == NULL || serial_vector == NULL) {
        free(current_raw);
        free(serial_vector);
        return NULL;
    }
    current_serial = build_date_serial_expression(current_raw);
    if (current_serial == NULL) {
        free(current_raw);
        free(serial_vector);
        return NULL;
    }
    current_valid = build_date_valid_expression(current_raw, current_serial);
    valid_vector = build_date_valid_expression(RAW_RANGE, serial_vector);
    if (current_valid == NULL || valid_vector == NULL) {
        free(current_raw);
        free(current_serial);
        free(serial_vector);
        free(current_valid);
        free(valid_vector);
        return NULL;
    }
    previous_position = build_previous_valid_position(valid_vector);
    next_position = build_next_valid_position(valid_vector);
    if (previous_position == NULL || next_position == NULL) {
        free(current_raw);
        free(current_serial);
        free(serial_vector);
        free(current_valid);
        free(valid_vector);
        free(previous_position);
        free(next_position);
        return NULL;
    }

    previous_date = allocate_printf("INDEX(%s,%s)",
                                    serial_vector,
                                    previous_position);
    next_date = allocate_printf("INDEX(%s,%s)",
                                serial_vector,
                                next_position);
    if (previous_date == NULL || next_date == NULL) {
        free(current_raw);
        free(current_serial);
        free(serial_vector);
        free(current_valid);
        free(valid_vector);
        free(previous_position);
        free(next_position);
        free(previous_date);
        free(next_date);
        return NULL;
    }

    formula = allocate_printf(
        "{=IF(%s=\"\",\"\",IF(%s,%s,IFERROR("
        "%s+(ROW()-ROW(Transacciones!$A$2)+1-%s)*"
        "((%s-%s)/(%s-%s)),\"SIN_VECINOS_VALIDOS\")))}",
        current_raw,
        current_valid,
        current_serial,
        previous_date,
        previous_position,
        next_date,
        previous_date,
        next_position,
        previous_position);

    free(current_raw);
    free(current_serial);
    free(serial_vector);
    free(current_valid);
    free(valid_vector);
    free(previous_position);
    free(next_position);
    free(previous_date);
    free(next_date);
    return formula;
}
