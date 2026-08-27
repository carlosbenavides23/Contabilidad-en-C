#ifndef FINANCIAL_RECONCILIATION_WORKBOOK_H
#define FINANCIAL_RECONCILIATION_WORKBOOK_H

#include <stddef.h>

#include "data.h"

int generate_financial_workbook(const char *output_path,
                                const Dataset *dataset,
                                char *error,
                                size_t error_size);

#endif
