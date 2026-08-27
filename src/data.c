#include "data.h"

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { CSV_LINE_CAPACITY = 4096, CSV_MAX_FIELDS = 4 };

static void set_error(char *error, size_t error_size, const char *message)
{
    if (error != NULL && error_size > 0U) {
        (void)snprintf(error, error_size, "%s", message);
    }
}

static char *duplicate_string(const char *source)
{
    const size_t length = strlen(source) + 1U;
    char *copy = malloc(length);

    if (copy != NULL) {
        (void)memcpy(copy, source, length);
    }
    return copy;
}

static char *trim(char *text)
{
    char *end;

    while (*text == ' ' || *text == '\t') {
        ++text;
    }

    end = text + strlen(text);
    while (end > text &&
           (end[-1] == ' ' || end[-1] == '\t' ||
            end[-1] == '\r' || end[-1] == '\n')) {
        --end;
    }
    *end = '\0';
    return text;
}

/* Supports ordinary CSV quoting and doubled quotes, but not embedded newlines. */
static size_t split_csv_line(char *line, char **fields, size_t field_capacity)
{
    char *read_cursor = line;
    char *write_cursor = line;
    size_t count = 0U;

    while (*read_cursor != '\0' && count < field_capacity) {
        bool quoted = false;
        fields[count] = write_cursor;
        ++count;

        if (*read_cursor == '"') {
            quoted = true;
            ++read_cursor;
        }

        while (*read_cursor != '\0') {
            if (quoted && *read_cursor == '"') {
                if (read_cursor[1] == '"') {
                    *write_cursor = '"';
                    ++write_cursor;
                    read_cursor += 2;
                } else {
                    quoted = false;
                    ++read_cursor;
                }
            } else if (!quoted && *read_cursor == ',') {
                ++read_cursor;
                break;
            } else if (!quoted && (*read_cursor == '\r' || *read_cursor == '\n')) {
                while (*read_cursor == '\r' || *read_cursor == '\n') {
                    ++read_cursor;
                }
                break;
            } else {
                *write_cursor = *read_cursor;
                ++write_cursor;
                ++read_cursor;
            }
        }
        *write_cursor = '\0';
        ++write_cursor;

        while (!quoted && (*read_cursor == ' ' || *read_cursor == '\t')) {
            ++read_cursor;
        }
    }

    return count;
}

static int parse_double(const char *text, double *value)
{
    char *end = NULL;

    errno = 0;
    *value = strtod(text, &end);
    if (errno != 0 || end == text) {
        return -1;
    }
    while (*end == ' ' || *end == '\t') {
        ++end;
    }
    return *end == '\0' ? 0 : -1;
}

static int allocate_demo_transactions(Dataset *dataset,
                                      char *error,
                                      size_t error_size)
{
    static const char *const rows[] = {
        "TX001#2023-01-10#CC_NORTE#900.00#IVA_15",
        "TX002#2023-01-20#CC_SUR#1200.00#IVA_15",
        "TX003#2023-02-05#CC_ESTE#750.00#IVA_05",
        "TX004#2023-02-18#CC_OESTE#2100.00#EXENTO",
        "TX005#2023-03-03#CC_NORTE#1600.00#IVA_15",
        "TX006#2023-03-15#CC_SUR#3600.00#IVA_05",
        "TX007#2023-04-02#CC_ESTE#450.00#IVA_05",
        "TX008#2023-04-08#CC_OESTE#5200.00#IVA_15",
        "TX009#????-??-??#CC_NORTE#1000.00#IVA_15",
        "TX010#INVALID_DATE#CC_SUR#1200.00#IVA_15",
        "TX011#2023-04-20#CC_NORTE#1800.00#IVA_15",
        "TX012#2023-04-25#CC_SUR#2300.00#IVA_05",
        "TX013#2023-05-06#CC_ESTE#1950.00#IVA_05",
        "TX014#2023-05-22#CC_OESTE#850.00#EXENTO",
        "TX015#2023-06-11#CC_NORTE#3100.00#IVA_15",
        "TX016#2023-06-19#CC_SUR#640.00#EXENTO",
        "TX017#2023-07-04#CC_ESTE#2750.00#IVA_15",
        "TX018#2023-08-17#CC_OESTE#1350.00#IVA_05",
        "TX019#2023-09-09#CC_NORTE#4200.00#IVA_15",
        "TX020#2023-10-28#CC_SUR#980.00#IVA_05",
        "TX021#2023-11-13#CC_ESTE#1125.50#EXENTO",
        "TX022#2023-12-07#CC_OESTE#3600.00#IVA_15",
        "TX023#2024-01-14#CC_NORTE#700.00#IVA_05",
        "TX024#2024-02-21#CC_SUR#2500.00#IVA_15"
    };
    const size_t count = sizeof(rows) / sizeof(rows[0]);
    size_t index;

    dataset->transactions = calloc(count, sizeof(*dataset->transactions));
    if (dataset->transactions == NULL) {
        set_error(error, error_size, "Sin memoria para transacciones de ejemplo.");
        return -1;
    }
    dataset->transaction_count = count;

    for (index = 0U; index < count; ++index) {
        dataset->transactions[index].raw = duplicate_string(rows[index]);
        if (dataset->transactions[index].raw == NULL) {
            set_error(error, error_size, "Sin memoria al copiar una transaccion.");
            return -1;
        }
    }
    return 0;
}

static int allocate_demo_tax_rates(Dataset *dataset,
                                   char *error,
                                   size_t error_size)
{
    static const struct {
        const char *code;
        double rate;
        double withholding;
    } rows[] = {
        {"IVA_15", 0.15, 0.02},
        {"IVA_05", 0.05, 0.01},
        {"EXENTO", 0.00, 0.00}
    };
    const size_t count = sizeof(rows) / sizeof(rows[0]);
    size_t index;

    dataset->tax_rates = calloc(count, sizeof(*dataset->tax_rates));
    if (dataset->tax_rates == NULL) {
        set_error(error, error_size, "Sin memoria para tasas de ejemplo.");
        return -1;
    }
    dataset->tax_rate_count = count;

    for (index = 0U; index < count; ++index) {
        dataset->tax_rates[index].tax_code = duplicate_string(rows[index].code);
        if (dataset->tax_rates[index].tax_code == NULL) {
            set_error(error, error_size, "Sin memoria al copiar una tasa.");
            return -1;
        }
        dataset->tax_rates[index].rate = rows[index].rate;
        dataset->tax_rates[index].withholding_factor = rows[index].withholding;
    }
    return 0;
}

static int allocate_demo_budget_caps(Dataset *dataset,
                                     char *error,
                                     size_t error_size)
{
    static const struct {
        const char *center;
        double limit;
        double penalty;
        const char *risk;
    } rows[] = {
        {"CC_NORTE", 3000.00, 0.10, "ALTO"},
        {"CC_SUR", 3500.00, 0.08, "MEDIO"},
        {"CC_ESTE", 2000.00, 0.12, "ALTO"},
        {"CC_OESTE", 4000.00, 0.05, "BAJO"}
    };
    const size_t count = sizeof(rows) / sizeof(rows[0]);
    size_t index;

    dataset->budget_caps = calloc(count, sizeof(*dataset->budget_caps));
    if (dataset->budget_caps == NULL) {
        set_error(error, error_size, "Sin memoria para topes de ejemplo.");
        return -1;
    }
    dataset->budget_cap_count = count;

    for (index = 0U; index < count; ++index) {
        BudgetCap *cap = &dataset->budget_caps[index];
        cap->cost_center = duplicate_string(rows[index].center);
        cap->risk_category = duplicate_string(rows[index].risk);
        if (cap->cost_center == NULL || cap->risk_category == NULL) {
            set_error(error, error_size, "Sin memoria al copiar un tope.");
            return -1;
        }
        cap->monthly_limit = rows[index].limit;
        cap->penalty_factor = rows[index].penalty;
    }
    return 0;
}

int dataset_init_demo(Dataset *dataset, char *error, size_t error_size)
{
    if (dataset == NULL) {
        set_error(error, error_size, "Dataset nulo.");
        return -1;
    }

    (void)memset(dataset, 0, sizeof(*dataset));
    if (allocate_demo_transactions(dataset, error, error_size) != 0 ||
        allocate_demo_tax_rates(dataset, error, error_size) != 0 ||
        allocate_demo_budget_caps(dataset, error, error_size) != 0) {
        dataset_free(dataset);
        return -1;
    }
    return 0;
}

static int load_transactions(Dataset *dataset,
                             const char *path,
                             char *error,
                             size_t error_size)
{
    FILE *file = fopen(path, "r");
    char line[CSV_LINE_CAPACITY];
    bool first_record = true;
    size_t count = 0U;

    if (file == NULL) {
        (void)snprintf(error, error_size, "No se pudo abrir %s.", path);
        return -1;
    }
    dataset->transactions = calloc(MAX_TRANSACTION_ROWS,
                                   sizeof(*dataset->transactions));
    if (dataset->transactions == NULL) {
        (void)fclose(file);
        set_error(error, error_size, "Sin memoria para cargar transacciones.");
        return -1;
    }

    while (fgets(line, (int)sizeof(line), file) != NULL) {
        char *value = trim(line);
        char *fields[1];

        if (*value == '\0') {
            continue;
        }
        if (first_record) {
            first_record = false;
            continue;
        }
        if (count >= MAX_TRANSACTION_ROWS) {
            (void)fclose(file);
            set_error(error, error_size, "El CSV excede 499 transacciones.");
            return -1;
        }
        if (split_csv_line(value, fields, 1U) != 1U) {
            (void)fclose(file);
            set_error(error, error_size, "Fila de transaccion CSV invalida.");
            return -1;
        }
        dataset->transactions[count].raw = duplicate_string(trim(fields[0]));
        if (dataset->transactions[count].raw == NULL) {
            (void)fclose(file);
            set_error(error, error_size, "Sin memoria para una transaccion CSV.");
            return -1;
        }
        ++count;
        dataset->transaction_count = count;
    }

    if (ferror(file) != 0 || fclose(file) != 0) {
        set_error(error, error_size, "Error de lectura en transacciones CSV.");
        return -1;
    }
    if (count == 0U) {
        set_error(error, error_size, "El CSV no contiene transacciones.");
        return -1;
    }
    return 0;
}

static int load_tax_rates(Dataset *dataset,
                          const char *path,
                          char *error,
                          size_t error_size)
{
    FILE *file = fopen(path, "r");
    char line[CSV_LINE_CAPACITY];
    bool first_record = true;
    size_t count = 0U;

    if (file == NULL) {
        (void)snprintf(error, error_size, "No se pudo abrir %s.", path);
        return -1;
    }
    dataset->tax_rates = calloc(MAX_TAX_ROWS, sizeof(*dataset->tax_rates));
    if (dataset->tax_rates == NULL) {
        (void)fclose(file);
        set_error(error, error_size, "Sin memoria para cargar tasas.");
        return -1;
    }

    while (fgets(line, (int)sizeof(line), file) != NULL) {
        char *fields[3];
        char *value = trim(line);
        TaxRate *rate;

        if (*value == '\0') {
            continue;
        }
        if (first_record) {
            first_record = false;
            continue;
        }
        if (count >= MAX_TAX_ROWS || split_csv_line(value, fields, 3U) != 3U) {
            (void)fclose(file);
            set_error(error, error_size, "Fila de tasas CSV invalida o fuera de rango.");
            return -1;
        }
        rate = &dataset->tax_rates[count];
        rate->tax_code = duplicate_string(trim(fields[0]));
        if (rate->tax_code == NULL ||
            parse_double(trim(fields[1]), &rate->rate) != 0 ||
            parse_double(trim(fields[2]), &rate->withholding_factor) != 0) {
            free(rate->tax_code);
            rate->tax_code = NULL;
            (void)fclose(file);
            set_error(error, error_size, "Valor invalido en tasas CSV.");
            return -1;
        }
        ++count;
        dataset->tax_rate_count = count;
    }

    if (ferror(file) != 0 || fclose(file) != 0) {
        set_error(error, error_size, "Error de lectura en tasas CSV.");
        return -1;
    }
    if (count == 0U) {
        set_error(error, error_size, "El CSV no contiene tasas.");
        return -1;
    }
    return 0;
}

static int load_budget_caps(Dataset *dataset,
                            const char *path,
                            char *error,
                            size_t error_size)
{
    FILE *file = fopen(path, "r");
    char line[CSV_LINE_CAPACITY];
    bool first_record = true;
    size_t count = 0U;

    if (file == NULL) {
        (void)snprintf(error, error_size, "No se pudo abrir %s.", path);
        return -1;
    }
    dataset->budget_caps = calloc(MAX_BUDGET_ROWS, sizeof(*dataset->budget_caps));
    if (dataset->budget_caps == NULL) {
        (void)fclose(file);
        set_error(error, error_size, "Sin memoria para cargar topes.");
        return -1;
    }

    while (fgets(line, (int)sizeof(line), file) != NULL) {
        char *fields[4];
        char *value = trim(line);
        BudgetCap *cap;

        if (*value == '\0') {
            continue;
        }
        if (first_record) {
            first_record = false;
            continue;
        }
        if (count >= MAX_BUDGET_ROWS || split_csv_line(value, fields, 4U) != 4U) {
            (void)fclose(file);
            set_error(error, error_size, "Fila de topes CSV invalida o fuera de rango.");
            return -1;
        }
        cap = &dataset->budget_caps[count];
        cap->cost_center = duplicate_string(trim(fields[0]));
        cap->risk_category = duplicate_string(trim(fields[3]));
        if (cap->cost_center == NULL || cap->risk_category == NULL ||
            parse_double(trim(fields[1]), &cap->monthly_limit) != 0 ||
            parse_double(trim(fields[2]), &cap->penalty_factor) != 0) {
            free(cap->cost_center);
            free(cap->risk_category);
            cap->cost_center = NULL;
            cap->risk_category = NULL;
            (void)fclose(file);
            set_error(error, error_size, "Valor invalido en topes CSV.");
            return -1;
        }
        ++count;
        dataset->budget_cap_count = count;
    }

    if (ferror(file) != 0 || fclose(file) != 0) {
        set_error(error, error_size, "Error de lectura en topes CSV.");
        return -1;
    }
    if (count == 0U) {
        set_error(error, error_size, "El CSV no contiene topes.");
        return -1;
    }
    return 0;
}

int dataset_load_csv(Dataset *dataset,
                     const char *transactions_path,
                     const char *tax_rates_path,
                     const char *budget_caps_path,
                     char *error,
                     size_t error_size)
{
    if (dataset == NULL || transactions_path == NULL || tax_rates_path == NULL ||
        budget_caps_path == NULL) {
        set_error(error, error_size, "Argumento nulo al cargar CSV.");
        return -1;
    }

    (void)memset(dataset, 0, sizeof(*dataset));
    if (load_transactions(dataset, transactions_path, error, error_size) != 0 ||
        load_tax_rates(dataset, tax_rates_path, error, error_size) != 0 ||
        load_budget_caps(dataset, budget_caps_path, error, error_size) != 0) {
        dataset_free(dataset);
        return -1;
    }
    return 0;
}

void dataset_free(Dataset *dataset)
{
    size_t index;

    if (dataset == NULL) {
        return;
    }

    for (index = 0U; index < dataset->transaction_count; ++index) {
        free(dataset->transactions[index].raw);
    }
    free(dataset->transactions);

    for (index = 0U; index < dataset->tax_rate_count; ++index) {
        free(dataset->tax_rates[index].tax_code);
    }
    free(dataset->tax_rates);

    for (index = 0U; index < dataset->budget_cap_count; ++index) {
        free(dataset->budget_caps[index].cost_center);
        free(dataset->budget_caps[index].risk_category);
    }
    free(dataset->budget_caps);

    (void)memset(dataset, 0, sizeof(*dataset));
}
