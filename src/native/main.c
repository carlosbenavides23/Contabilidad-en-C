#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "data.h"
#include "financial_core.h"

enum { TRANSACTION_ID_CAPACITY = 64 };

static const TaxRate *find_tax_rate(const Dataset *dataset,
                                    const char *tax_code,
                                    size_t tax_code_length)
{
    size_t index;

    for (index = 0U; index < dataset->tax_rate_count; ++index) {
        const char *candidate = dataset->tax_rates[index].tax_code;
        if (strlen(candidate) == tax_code_length &&
            strncmp(candidate, tax_code, tax_code_length) == 0) {
            return &dataset->tax_rates[index];
        }
    }
    return NULL;
}

static int parse_transaction(const char *raw,
                             const Dataset *dataset,
                             char *id,
                             size_t id_capacity,
                             double *amount,
                             double *tax_rate,
                             double *retention,
                             char *error,
                             size_t error_size)
{
    const char *separator_1 = strchr(raw, '#');
    const char *separator_2;
    const char *separator_3;
    const char *separator_4;
    const TaxRate *resolved_rate;
    char *amount_end = NULL;
    size_t id_length;
    size_t tax_code_length;

    if (separator_1 == NULL) {
        (void)snprintf(error, error_size, "Faltan campos separados por '#'.");
        return -1;
    }
    separator_2 = strchr(separator_1 + 1, '#');
    separator_3 = separator_2 == NULL ? NULL : strchr(separator_2 + 1, '#');
    separator_4 = separator_3 == NULL ? NULL : strchr(separator_3 + 1, '#');

    if (separator_2 == NULL || separator_3 == NULL || separator_4 == NULL ||
        strchr(separator_4 + 1, '#') != NULL || separator_1 == raw ||
        separator_2 == separator_1 + 1 || separator_3 == separator_2 + 1 ||
        separator_4 == separator_3 + 1 || separator_4[1] == '\0') {
        (void)snprintf(error, error_size, "Estructura de transaccion invalida.");
        return -1;
    }

    id_length = (size_t)(separator_1 - raw);
    if (id_length >= id_capacity) {
        (void)snprintf(error, error_size, "ID de transaccion demasiado largo.");
        return -1;
    }
    (void)memcpy(id, raw, id_length);
    id[id_length] = '\0';

    errno = 0;
    *amount = strtod(separator_3 + 1, &amount_end);
    if (errno != 0 || amount_end != separator_4 || !isfinite(*amount)) {
        (void)snprintf(error, error_size, "Monto base invalido.");
        return -1;
    }

    tax_code_length = strlen(separator_4 + 1);
    resolved_rate = find_tax_rate(dataset, separator_4 + 1, tax_code_length);
    if (resolved_rate == NULL) {
        (void)snprintf(error, error_size, "Codigo fiscal desconocido.");
        return -1;
    }

    *tax_rate = resolved_rate->rate;
    *retention = resolved_rate->withholding_factor;
    return 0;
}

static void print_usage(const char *program_name)
{
    fprintf(stderr,
            "Uso:\n"
            "  %s\n"
            "  %s transacciones.csv catalogo_tasas.csv topes_presupuesto.csv\n",
            program_name,
            program_name);
}

int main(int argc, char **argv)
{
    const char *transactions_path = "data/transacciones.csv";
    const char *tax_rates_path = "data/catalogo_tasas.csv";
    const char *budget_caps_path = "data/topes_presupuesto.csv";
    Dataset dataset = {0};
    char error_message[512] = {0};
    char (*ids)[TRANSACTION_ID_CAPACITY] = NULL;
    double *amounts = NULL;
    double *tax_rates = NULL;
    double *retentions = NULL;
    double *net_amounts = NULL;
    size_t processed_count = 0U;
    size_t index;
    int status = 1;

    if (argc == 4) {
        transactions_path = argv[1];
        tax_rates_path = argv[2];
        budget_caps_path = argv[3];
    } else if (argc != 1) {
        print_usage(argv[0]);
        return 2;
    }

    if (dataset_load_csv(&dataset,
                         transactions_path,
                         tax_rates_path,
                         budget_caps_path,
                         error_message,
                         sizeof(error_message)) != 0) {
        fprintf(stderr, "Error al cargar datos: %s\n", error_message);
        goto cleanup;
    }
    if (dataset.transaction_count > (size_t)INT_MAX) {
        fprintf(stderr, "Error: demasiadas transacciones para el nucleo Fortran.\n");
        goto cleanup;
    }

    ids = calloc(dataset.transaction_count, sizeof(*ids));
    amounts = calloc(dataset.transaction_count, sizeof(*amounts));
    tax_rates = calloc(dataset.transaction_count, sizeof(*tax_rates));
    retentions = calloc(dataset.transaction_count, sizeof(*retentions));
    net_amounts = calloc(dataset.transaction_count, sizeof(*net_amounts));
    if (ids == NULL || amounts == NULL || tax_rates == NULL ||
        retentions == NULL || net_amounts == NULL) {
        fprintf(stderr, "Error: sin memoria para preparar R1.\n");
        goto cleanup;
    }

    for (index = 0U; index < dataset.transaction_count; ++index) {
        const char *raw = dataset.transactions[index].raw;

        if (raw == NULL || raw[0] == '\0') {
            continue;
        }
        if (parse_transaction(raw,
                              &dataset,
                              ids[processed_count],
                              sizeof(ids[processed_count]),
                              &amounts[processed_count],
                              &tax_rates[processed_count],
                              &retentions[processed_count],
                              error_message,
                              sizeof(error_message)) != 0) {
            fprintf(stderr,
                    "Error en transaccion %zu: %s\n",
                    index + 1U,
                    error_message);
            goto cleanup;
        }
        ++processed_count;
    }

    financial_core_calculate_net_amounts((int)processed_count,
                                         amounts,
                                         tax_rates,
                                         retentions,
                                         net_amounts);

    for (index = 0U; index < processed_count; ++index) {
        printf("%s | monto_base=%.2f | tasa=%.2f | retencion=%.2f "
               "| monto_neto=%.2f\n",
               ids[index],
               amounts[index],
               tax_rates[index],
               retentions[index],
               net_amounts[index]);
    }
    printf("Transacciones procesadas: %zu\n", processed_count);
    status = 0;

cleanup:
    free(ids);
    free(amounts);
    free(tax_rates);
    free(retentions);
    free(net_amounts);
    dataset_free(&dataset);
    return status;
}
