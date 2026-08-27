#ifndef FINANCIAL_RECONCILIATION_FORMULAS_H
#define FINANCIAL_RECONCILIATION_FORMULAS_H

#include <stddef.h>

typedef enum {
    TOP_FIELD_ID,
    TOP_FIELD_AMOUNT,
    TOP_FIELD_DATE,
    TOP_FIELD_COST_CENTER,
    TOP_FIELD_TAX_CODE
} TopField;

char *build_amount_expression(const char *raw_reference);
char *build_tax_code_expression(const char *raw_reference);
char *build_cost_center_expression(const char *raw_reference);
char *build_date_expression(const char *raw_reference);
char *build_id_expression(const char *raw_reference);

char *build_net_amount_formula(void);
char *build_consolidated_formula(const char *cost_center_reference);
char *build_budget_formula(const char *cost_center_reference,
                           const char *month_reference);
char *build_top5_formula(TopField field);
char *build_determinant_formula(void);
char *build_annual_deviation_formula(size_t consolidated_excel_row);
char *build_linear_solution_formula(void);
char *build_temporal_reconstruction_formula(size_t excel_row);

#endif
