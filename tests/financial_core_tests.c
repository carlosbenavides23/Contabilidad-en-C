#include <math.h>
#include <stdio.h>

#include "financial_core.h"

static const double TOLERANCE = 1e-9;

static int check_close(const char *case_name, double actual, double expected)
{
    if (fabs(actual - expected) > TOLERANCE) {
        fprintf(stderr,
                "%s: esperado %.12f, obtenido %.12f\n",
                case_name,
                expected,
                actual);
        return -1;
    }
    return 0;
}

static int test_tx001(void)
{
    const double amounts[] = {900.0};
    const double tax_rates[] = {0.15};
    const double retentions[] = {0.02};
    double results[1] = {0.0};

    financial_core_calculate_net_amounts(1,
                                         amounts,
                                         tax_rates,
                                         retentions,
                                         results);
    return check_close("TX001", results[0], 1017.0);
}

static int test_multiple_amounts(void)
{
    const double amounts[] = {900.0, 750.0, 2100.0};
    const double tax_rates[] = {0.15, 0.05, 0.0};
    const double retentions[] = {0.02, 0.01, 0.0};
    const double expected[] = {1017.0, 780.0, 2100.0};
    double results[3] = {0.0, 0.0, 0.0};
    size_t index;

    financial_core_calculate_net_amounts(3,
                                         amounts,
                                         tax_rates,
                                         retentions,
                                         results);
    for (index = 0U; index < 3U; ++index) {
        if (check_close("vector R1", results[index], expected[index]) != 0) {
            return -1;
        }
    }
    return 0;
}

int main(void)
{
    if (test_tx001() != 0 || test_multiple_amounts() != 0) {
        return 1;
    }
    return 0;
}
