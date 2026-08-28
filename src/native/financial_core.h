#ifndef FINANCIAL_RECONCILIATION_FINANCIAL_CORE_H
#define FINANCIAL_RECONCILIATION_FINANCIAL_CORE_H

void financial_core_calculate_net_amounts(int n,
                                          const double *amounts,
                                          const double *tax_rates,
                                          const double *retentions,
                                          double *net_amounts);

#endif
