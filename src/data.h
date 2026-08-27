#ifndef FINANCIAL_RECONCILIATION_DATA_H
#define FINANCIAL_RECONCILIATION_DATA_H

#include <stddef.h>

enum {
    MAX_TRANSACTION_ROWS = 499,
    MAX_TAX_ROWS = 19,
    MAX_BUDGET_ROWS = 9
};

typedef struct {
    char *raw;
} Transaction;

typedef struct {
    char *tax_code;
    double rate;
    double withholding_factor;
} TaxRate;

typedef struct {
    char *cost_center;
    double monthly_limit;
    double penalty_factor;
    char *risk_category;
} BudgetCap;

typedef struct {
    Transaction *transactions;
    size_t transaction_count;
    TaxRate *tax_rates;
    size_t tax_rate_count;
    BudgetCap *budget_caps;
    size_t budget_cap_count;
} Dataset;

int dataset_init_demo(Dataset *dataset, char *error, size_t error_size);

int dataset_load_csv(Dataset *dataset,
                     const char *transactions_path,
                     const char *tax_rates_path,
                     const char *budget_caps_path,
                     char *error,
                     size_t error_size);

void dataset_free(Dataset *dataset);

#endif

