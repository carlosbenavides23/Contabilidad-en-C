#include "workbook.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "formulas.h"
#include "xlsxwriter.h"

typedef struct {
    lxw_format *header;
    lxw_format *section;
    lxw_format *money;
    lxw_format *percent;
    lxw_format *date;
    lxw_format *anomaly_text;
    lxw_format *anomaly_money;
} WorkbookFormats;

#define RETURN_ON_LXW_ERROR(call)         \
    do {                                  \
        lxw_error call_error = (call);    \
        if (call_error != LXW_NO_ERROR) { \
            return call_error;            \
        }                                 \
    } while (0)

static void set_error(char *error, size_t error_size, const char *message)
{
    if (error != NULL && error_size > 0U) {
        (void)snprintf(error, error_size, "%s", message);
    }
}

static void set_lxw_error(char *error,
                          size_t error_size,
                          const char *operation,
                          lxw_error code)
{
    if (error != NULL && error_size > 0U) {
        (void)snprintf(error,
                       error_size,
                       "%s: %s (codigo %d).",
                       operation,
                       lxw_strerror(code),
                       (int)code);
    }
}

static int create_formats(lxw_workbook *workbook, WorkbookFormats *formats)
{
    formats->header = workbook_add_format(workbook);
    formats->section = workbook_add_format(workbook);
    formats->money = workbook_add_format(workbook);
    formats->percent = workbook_add_format(workbook);
    formats->date = workbook_add_format(workbook);
    formats->anomaly_text = workbook_add_format(workbook);
    formats->anomaly_money = workbook_add_format(workbook);

    if (formats->header == NULL || formats->section == NULL ||
        formats->money == NULL || formats->percent == NULL ||
        formats->date == NULL || formats->anomaly_text == NULL ||
        formats->anomaly_money == NULL) {
        return -1;
    }

    format_set_bold(formats->header);
    format_set_font_color(formats->header, 0xFFFFFFU);
    format_set_fg_color(formats->header, 0x1F4E78U);
    format_set_border(formats->header, LXW_BORDER_THIN);
    format_set_align(formats->header, LXW_ALIGN_CENTER);
    format_set_align(formats->header, LXW_ALIGN_VERTICAL_CENTER);
    format_set_text_wrap(formats->header);

    format_set_bold(formats->section);
    format_set_fg_color(formats->section, 0xD9EAF7U);
    format_set_border(formats->section, LXW_BORDER_THIN);

    format_set_num_format(formats->money, "#,##0.00");
    format_set_num_format(formats->percent, "0.00%");
    format_set_num_format(formats->date, "yyyy-mm-dd");

    format_set_fg_color(formats->anomaly_text, 0xFCE4D6U);
    format_set_border(formats->anomaly_text, LXW_BORDER_THIN);
    format_set_fg_color(formats->anomaly_money, 0xFCE4D6U);
    format_set_border(formats->anomaly_money, LXW_BORDER_THIN);
    format_set_num_format(formats->anomaly_money, "#,##0.00");
    return 0;
}

static lxw_error write_transactions_sheet(lxw_worksheet *worksheet,
                                          const Dataset *dataset,
                                          const WorkbookFormats *formats)
{
    size_t index;

    RETURN_ON_LXW_ERROR(worksheet_write_string(worksheet,
                                                0U,
                                                0U,
                                                "RAW_TRANSACTION",
                                                formats->header));
    for (index = 0U; index < dataset->transaction_count; ++index) {
        const lxw_row_t row = (lxw_row_t)(index + 1U);
        RETURN_ON_LXW_ERROR(worksheet_write_string(
            worksheet, row, 0U, dataset->transactions[index].raw, NULL));
    }

    RETURN_ON_LXW_ERROR(worksheet_set_column(worksheet, 0U, 0U, 58.0, NULL));
    RETURN_ON_LXW_ERROR(worksheet_autofilter(worksheet, 0U, 0U, 499U, 0U));
    worksheet_freeze_panes(worksheet, 1U, 0U);
    return LXW_NO_ERROR;
}

static lxw_error write_tax_sheet(lxw_worksheet *worksheet,
                                 const Dataset *dataset,
                                 const WorkbookFormats *formats)
{
    static const char *const headers[] = {
        "COD_IMPUESTO", "TASA_DECIMAL", "FACTOR_RETENCION"
    };
    size_t column;
    size_t index;

    for (column = 0U; column < 3U; ++column) {
        RETURN_ON_LXW_ERROR(worksheet_write_string(worksheet,
                                                    0U,
                                                    (lxw_col_t)column,
                                                    headers[column],
                                                    formats->header));
    }
    for (index = 0U; index < dataset->tax_rate_count; ++index) {
        const lxw_row_t row = (lxw_row_t)(index + 1U);
        RETURN_ON_LXW_ERROR(worksheet_write_string(
            worksheet, row, 0U, dataset->tax_rates[index].tax_code, NULL));
        RETURN_ON_LXW_ERROR(worksheet_write_number(
            worksheet, row, 1U, dataset->tax_rates[index].rate, formats->percent));
        RETURN_ON_LXW_ERROR(worksheet_write_number(
            worksheet,
            row,
            2U,
            dataset->tax_rates[index].withholding_factor,
            formats->percent));
    }

    RETURN_ON_LXW_ERROR(worksheet_set_column(worksheet, 0U, 0U, 18.0, NULL));
    RETURN_ON_LXW_ERROR(worksheet_set_column(worksheet, 1U, 2U, 20.0, NULL));
    RETURN_ON_LXW_ERROR(worksheet_autofilter(worksheet, 0U, 0U, 19U, 2U));
    worksheet_freeze_panes(worksheet, 1U, 0U);
    return LXW_NO_ERROR;
}

static lxw_error write_budget_caps_sheet(lxw_worksheet *worksheet,
                                         const Dataset *dataset,
                                         const WorkbookFormats *formats)
{
    static const char *const headers[] = {
        "CENTRO_COSTO",
        "LIMITE_MENSUAL",
        "FACTOR_PENALIZACION",
        "CATEGORIA_RIESGO"
    };
    size_t column;
    size_t index;

    for (column = 0U; column < 4U; ++column) {
        RETURN_ON_LXW_ERROR(worksheet_write_string(worksheet,
                                                    0U,
                                                    (lxw_col_t)column,
                                                    headers[column],
                                                    formats->header));
    }
    for (index = 0U; index < dataset->budget_cap_count; ++index) {
        const BudgetCap *cap = &dataset->budget_caps[index];
        const lxw_row_t row = (lxw_row_t)(index + 1U);
        RETURN_ON_LXW_ERROR(worksheet_write_string(
            worksheet, row, 0U, cap->cost_center, NULL));
        RETURN_ON_LXW_ERROR(worksheet_write_number(
            worksheet, row, 1U, cap->monthly_limit, formats->money));
        RETURN_ON_LXW_ERROR(worksheet_write_number(
            worksheet, row, 2U, cap->penalty_factor, formats->percent));
        RETURN_ON_LXW_ERROR(worksheet_write_string(
            worksheet, row, 3U, cap->risk_category, NULL));
    }

    RETURN_ON_LXW_ERROR(worksheet_set_column(worksheet, 0U, 0U, 18.0, NULL));
    RETURN_ON_LXW_ERROR(worksheet_set_column(worksheet, 1U, 2U, 22.0, NULL));
    RETURN_ON_LXW_ERROR(worksheet_set_column(worksheet, 3U, 3U, 20.0, NULL));
    RETURN_ON_LXW_ERROR(worksheet_autofilter(worksheet, 0U, 0U, 9U, 3U));
    worksheet_freeze_panes(worksheet, 1U, 0U);
    return LXW_NO_ERROR;
}

static lxw_error write_decoding_sheet(lxw_worksheet *worksheet,
                                      const WorkbookFormats *formats)
{
    char *formula = build_net_amount_formula();
    lxw_error result;

    if (formula == NULL) {
        return LXW_ERROR_MEMORY_MALLOC_FAILED;
    }
    result = worksheet_write_string(worksheet,
                                    0U,
                                    0U,
                                    "MONTO_NETO_GRAVADO",
                                    formats->header);
    if (result == LXW_NO_ERROR) {
        result = worksheet_write_array_formula(worksheet,
                                               1U,
                                               0U,
                                               499U,
                                               0U,
                                               formula,
                                               formats->money);
    }
    free(formula);
    if (result != LXW_NO_ERROR) {
        return result;
    }

    RETURN_ON_LXW_ERROR(worksheet_write_string(
        worksheet,
        0U,
        2U,
        "CSE: M*(1+T)-(M*R)",
        formats->section));
    RETURN_ON_LXW_ERROR(worksheet_set_column(worksheet, 0U, 0U, 24.0, NULL));
    RETURN_ON_LXW_ERROR(worksheet_set_column(worksheet, 2U, 2U, 58.0, NULL));
    worksheet_freeze_panes(worksheet, 1U, 0U);
    return LXW_NO_ERROR;
}

static lxw_error write_consolidated_sheet(lxw_worksheet *worksheet,
                                          const Dataset *dataset,
                                          const WorkbookFormats *formats)
{
    static const char *const month_references[] = {
        "B$3", "C$3", "D$3", "E$3", "F$3", "G$3",
        "H$3", "I$3", "J$3", "K$3", "L$3", "M$3"
    };
    size_t index;
    size_t month;

    RETURN_ON_LXW_ERROR(worksheet_write_string(
        worksheet, 0U, 0U, "ANO_ANALISIS", formats->section));
    RETURN_ON_LXW_ERROR(worksheet_write_number(worksheet, 0U, 1U, 2023.0, NULL));
    RETURN_ON_LXW_ERROR(worksheet_write_string(
        worksheet, 1U, 0U, "MES_CONSOLIDADO", formats->section));
    RETURN_ON_LXW_ERROR(worksheet_write_number(worksheet, 1U, 1U, 4.0, NULL));
    RETURN_ON_LXW_ERROR(worksheet_write_string(
        worksheet, 2U, 0U, "CENTRO_COSTO", formats->header));
    for (month = 0U; month < 12U; ++month) {
        RETURN_ON_LXW_ERROR(worksheet_write_number(worksheet,
                                                   2U,
                                                   (lxw_col_t)(month + 1U),
                                                   (double)(month + 1U),
                                                   formats->header));
    }
    RETURN_ON_LXW_ERROR(worksheet_write_string(
        worksheet, 2U, 13U, "SUBTOTAL_PONDERADO_RIESGO", formats->header));

    for (index = 0U; index < dataset->budget_cap_count; ++index) {
        const lxw_row_t row = (lxw_row_t)(index + 3U);
        char center_reference[24];
        char *formula;

        (void)snprintf(center_reference,
                       sizeof(center_reference),
                       "$A%zu",
                       index + 4U);
        RETURN_ON_LXW_ERROR(worksheet_write_string(
            worksheet,
            row,
            0U,
            dataset->budget_caps[index].cost_center,
            formats->section));

        for (month = 0U; month < 12U; ++month) {
            lxw_error result;
            formula = build_budget_formula(center_reference,
                                           month_references[month]);
            if (formula == NULL) {
                return LXW_ERROR_MEMORY_MALLOC_FAILED;
            }
            result = worksheet_write_formula(worksheet,
                                             row,
                                             (lxw_col_t)(month + 1U),
                                             formula,
                                             formats->money);
            free(formula);
            if (result != LXW_NO_ERROR) {
                return result;
            }
        }

        formula = build_consolidated_formula(center_reference);
        if (formula == NULL) {
            return LXW_ERROR_MEMORY_MALLOC_FAILED;
        }
        {
            const lxw_error result = worksheet_write_formula(
                worksheet, row, 13U, formula, formats->money);
            free(formula);
            if (result != LXW_NO_ERROR) {
                return result;
            }
        }
    }

    RETURN_ON_LXW_ERROR(worksheet_set_column(worksheet, 0U, 0U, 18.0, NULL));
    RETURN_ON_LXW_ERROR(worksheet_set_column(worksheet, 1U, 12U, 14.0, NULL));
    RETURN_ON_LXW_ERROR(worksheet_set_column(worksheet, 13U, 13U, 29.0, NULL));
    worksheet_freeze_panes(worksheet, 3U, 1U);
    return LXW_NO_ERROR;
}

static lxw_error write_audit_sheet(lxw_worksheet *worksheet,
                                   const WorkbookFormats *formats)
{
    static const char *const headers[] = {
        "ID_TX", "MONTO_BASE", "FECHA", "CENTRO_COSTO", "COD_IMPUESTO"
    };
    static const TopField fields[] = {
        TOP_FIELD_ID,
        TOP_FIELD_AMOUNT,
        TOP_FIELD_DATE,
        TOP_FIELD_COST_CENTER,
        TOP_FIELD_TAX_CODE
    };
    size_t column;
    size_t rank;

    for (column = 0U; column < 5U; ++column) {
        RETURN_ON_LXW_ERROR(worksheet_write_string(worksheet,
                                                    0U,
                                                    (lxw_col_t)column,
                                                    headers[column],
                                                    formats->header));
    }
    for (rank = 0U; rank < 5U; ++rank) {
        for (column = 0U; column < 5U; ++column) {
            char *formula = build_top5_formula(fields[column]);
            lxw_error result;
            lxw_format *format = column == 1U
                                     ? formats->anomaly_money
                                     : formats->anomaly_text;

            if (formula == NULL) {
                return LXW_ERROR_MEMORY_MALLOC_FAILED;
            }
            result = worksheet_write_array_formula(worksheet,
                                                   (lxw_row_t)(rank + 1U),
                                                   (lxw_col_t)column,
                                                   (lxw_row_t)(rank + 1U),
                                                   (lxw_col_t)column,
                                                   formula,
                                                   format);
            free(formula);
            if (result != LXW_NO_ERROR) {
                return result;
            }
        }
    }

    RETURN_ON_LXW_ERROR(worksheet_set_column(worksheet, 0U, 0U, 14.0, NULL));
    RETURN_ON_LXW_ERROR(worksheet_set_column(worksheet, 1U, 1U, 16.0, NULL));
    RETURN_ON_LXW_ERROR(worksheet_set_column(worksheet, 2U, 2U, 16.0, NULL));
    RETURN_ON_LXW_ERROR(worksheet_set_column(worksheet, 3U, 4U, 20.0, NULL));
    worksheet_freeze_panes(worksheet, 1U, 0U);
    return LXW_NO_ERROR;
}

static lxw_error write_adjustment_sheet(lxw_worksheet *worksheet,
                                        const WorkbookFormats *formats)
{
    static const double matrix[4][4] = {
        {4.0, 1.0, 2.0, 0.0},
        {1.0, 3.0, 0.0, 1.0},
        {2.0, 0.0, 5.0, 1.0},
        {0.0, 1.0, 1.0, 4.0}
    };
    size_t row;
    size_t column;
    char *determinant;
    char *solution;
    lxw_error result;

    for (row = 0U; row < 4U; ++row) {
        for (column = 0U; column < 4U; ++column) {
            RETURN_ON_LXW_ERROR(worksheet_write_number(
                worksheet,
                (lxw_row_t)row,
                (lxw_col_t)column,
                matrix[row][column],
                formats->money));
        }
        {
            char *annual_deviation = build_annual_deviation_formula(row + 4U);
            lxw_error formula_result;

            if (annual_deviation == NULL) {
                return LXW_ERROR_MEMORY_MALLOC_FAILED;
            }
            formula_result = worksheet_write_formula(worksheet,
                                                     (lxw_row_t)row,
                                                     4U,
                                                     annual_deviation,
                                                     formats->money);
            free(annual_deviation);
            if (formula_result != LXW_NO_ERROR) {
                return formula_result;
            }
        }
    }

    determinant = build_determinant_formula();
    solution = build_linear_solution_formula();
    if (determinant == NULL || solution == NULL) {
        free(determinant);
        free(solution);
        return LXW_ERROR_MEMORY_MALLOC_FAILED;
    }

    result = worksheet_write_formula(worksheet,
                                     0U,
                                     5U,
                                     determinant,
                                     formats->money);
    if (result == LXW_NO_ERROR) {
        result = worksheet_write_array_formula(worksheet,
                                               0U,
                                               6U,
                                               3U,
                                               6U,
                                               solution,
                                               formats->money);
    }
    free(determinant);
    free(solution);
    if (result != LXW_NO_ERROR) {
        return result;
    }

    RETURN_ON_LXW_ERROR(worksheet_write_string(
        worksheet, 5U, 0U, "MATRIZ_A (A1:D4)", formats->section));
    RETURN_ON_LXW_ERROR(worksheet_write_string(
        worksheet, 5U, 4U, "B: DESVIACION ANUAL", formats->section));
    RETURN_ON_LXW_ERROR(worksheet_write_string(
        worksheet, 5U, 5U, "DETERMINANTE", formats->section));
    RETURN_ON_LXW_ERROR(worksheet_write_string(
        worksheet, 5U, 6U, "SOLUCION_X (G1:G4)", formats->section));
    RETURN_ON_LXW_ERROR(worksheet_set_column(worksheet, 0U, 6U, 20.0, NULL));
    return LXW_NO_ERROR;
}

static lxw_error write_temporal_sheet(lxw_worksheet *worksheet,
                                      const WorkbookFormats *formats)
{
    lxw_row_t row;

    RETURN_ON_LXW_ERROR(worksheet_write_string(
        worksheet, 0U, 0U, "FECHA_RECONSTRUIDA", formats->header));
    RETURN_ON_LXW_ERROR(worksheet_write_string(
        worksheet,
        0U,
        2U,
        "CSE individual por fila; busca vecinos validos arbitrarios",
        formats->section));

    for (row = 1U; row <= 499U; ++row) {
        char *formula = build_temporal_reconstruction_formula((size_t)row + 1U);
        lxw_error result;

        if (formula == NULL) {
            return LXW_ERROR_MEMORY_MALLOC_FAILED;
        }
        result = worksheet_write_array_formula(worksheet,
                                               row,
                                               0U,
                                               row,
                                               0U,
                                               formula,
                                               formats->date);
        free(formula);
        if (result != LXW_NO_ERROR) {
            return result;
        }
    }

    RETURN_ON_LXW_ERROR(worksheet_set_column(worksheet, 0U, 0U, 24.0, NULL));
    RETURN_ON_LXW_ERROR(worksheet_set_column(worksheet, 2U, 2U, 58.0, NULL));
    worksheet_freeze_panes(worksheet, 1U, 0U);
    return LXW_NO_ERROR;
}

static lxw_error populate_workbook(lxw_workbook *workbook,
                                   const Dataset *dataset,
                                   const WorkbookFormats *formats)
{
    lxw_worksheet *transactions = workbook_add_worksheet(workbook, "Transacciones");
    lxw_worksheet *taxes = workbook_add_worksheet(workbook, "Catalogo_Tasas");
    lxw_worksheet *budgets = workbook_add_worksheet(workbook, "Topes_Presupuesto");
    lxw_worksheet *decoding = workbook_add_worksheet(workbook, "Decodificacion");
    lxw_worksheet *consolidated = workbook_add_worksheet(workbook, "Consolidado");
    lxw_worksheet *audit = workbook_add_worksheet(workbook, "Auditoria");
    lxw_worksheet *adjustment = workbook_add_worksheet(workbook, "Ajuste_Contable");
    lxw_worksheet *temporal = workbook_add_worksheet(workbook,
                                                     "Reconstruccion_Temporal");

    if (transactions == NULL || taxes == NULL || budgets == NULL ||
        decoding == NULL || consolidated == NULL || audit == NULL ||
        adjustment == NULL || temporal == NULL) {
        return LXW_ERROR_MEMORY_MALLOC_FAILED;
    }

    worksheet_activate(transactions);
    RETURN_ON_LXW_ERROR(write_transactions_sheet(transactions, dataset, formats));
    RETURN_ON_LXW_ERROR(write_tax_sheet(taxes, dataset, formats));
    RETURN_ON_LXW_ERROR(write_budget_caps_sheet(budgets, dataset, formats));
    RETURN_ON_LXW_ERROR(write_decoding_sheet(decoding, formats));
    RETURN_ON_LXW_ERROR(write_consolidated_sheet(consolidated, dataset, formats));
    RETURN_ON_LXW_ERROR(write_audit_sheet(audit, formats));
    RETURN_ON_LXW_ERROR(write_adjustment_sheet(adjustment, formats));
    RETURN_ON_LXW_ERROR(write_temporal_sheet(temporal, formats));
    return LXW_NO_ERROR;
}

int generate_financial_workbook(const char *output_path,
                                const Dataset *dataset,
                                char *error,
                                size_t error_size)
{
    lxw_workbook *workbook;
    WorkbookFormats formats = {0};
    lxw_error write_result;
    lxw_error close_result;

    if (output_path == NULL || dataset == NULL) {
        set_error(error, error_size, "Ruta de salida o dataset nulo.");
        return -1;
    }

    workbook = workbook_new(output_path);
    if (workbook == NULL) {
        set_error(error, error_size, "No se pudo crear el objeto workbook.");
        return -1;
    }
    if (create_formats(workbook, &formats) != 0) {
        close_result = workbook_close(workbook);
        (void)close_result;
        set_error(error, error_size, "No se pudieron crear los formatos.");
        return -1;
    }

    write_result = populate_workbook(workbook, dataset, &formats);
    close_result = workbook_close(workbook);

    if (write_result != LXW_NO_ERROR) {
        set_lxw_error(error, error_size, "Error al escribir una hoja", write_result);
        return -1;
    }
    if (close_result != LXW_NO_ERROR) {
        set_lxw_error(error, error_size, "workbook_close fallo", close_result);
        return -1;
    }
    return 0;
}
