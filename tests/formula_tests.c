#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "formulas.h"

enum { EXCEL_FORMULA_LIMIT = 8192 };

static int check_formula(const char *name,
                         char *formula,
                         const char *required_text)
{
    const size_t length = formula == NULL ? 0U : strlen(formula);

    if (formula == NULL) {
        fprintf(stderr, "%s devolvio NULL.\n", name);
        return 1;
    }
    if (length == 0U || length > EXCEL_FORMULA_LIMIT) {
        fprintf(stderr, "%s tiene longitud invalida: %zu.\n", name, length);
        free(formula);
        return 1;
    }
    if (strstr(formula, required_text) == NULL) {
        fprintf(stderr, "%s no contiene %s.\n", name, required_text);
        free(formula);
        return 1;
    }
    printf("%-24s %4zu caracteres\n", name, length);
    free(formula);
    return 0;
}

static int check_r1_regression(char *formula)
{
    const char *required = ")-(IFERROR(VALUE(MID(";
    const char *forbidden = ")*(IFERROR(VALUE(MID(";
    int failed = 0;

    if (formula == NULL) {
        fputs("neto CSE devolvio NULL.\n", stderr);
        return 1;
    }
    if (strstr(formula, required) == NULL) {
        fputs("neto CSE no contiene la resta M*(1+T)-(M*R).\n", stderr);
        failed = 1;
    }
    if (strstr(formula, forbidden) != NULL) {
        fputs("neto CSE conserva la multiplicacion incorrecta.\n", stderr);
        failed = 1;
    }
    printf("%-24s %4zu caracteres\n", "neto CSE", strlen(formula));
    free(formula);
    return failed;
}

static int check_annual_deviation(void)
{
    char *formula = build_annual_deviation_formula(4U);
    const char *expected = "=SUM(Consolidado!B4:M4)";
    int failed = 0;

    if (formula == NULL) {
        fputs("vector B devolvio NULL.\n", stderr);
        return 1;
    }
    if (strcmp(formula, expected) != 0) {
        fprintf(stderr, "vector B inesperado: %s.\n", formula);
        failed = 1;
    }
    printf("%-24s %4zu caracteres\n", "vector B", strlen(formula));
    free(formula);
    return failed;
}

int main(void)
{
    int failures = 0;

    failures += check_formula("monto",
                              build_amount_expression("Transacciones!$A$2:$A$500"),
                              "VALUE(MID(");
    failures += check_r1_regression(build_net_amount_formula());
    failures += check_formula("consolidado",
                              build_consolidated_formula("$A4"),
                              "SUMPRODUCT(");
    failures += check_formula("presupuesto",
                              build_budget_formula("$A4", "B$3"),
                              "LN(1+");
    failures += check_formula("top 5",
                              build_top5_formula(TOP_FIELD_ID),
                              "LARGE(");
    failures += check_formula("determinante",
                              build_determinant_formula(),
                              "MDETERM(");
    failures += check_annual_deviation();
    failures += check_formula("solucion CSE",
                              build_linear_solution_formula(),
                              "MMULT(MINVERSE(");
    failures += check_formula("reconstruccion CSE",
                              build_temporal_reconstruction_formula(9U),
                              "MATCH(2,1/");

    if (failures != 0) {
        fprintf(stderr, "%d comprobaciones fallaron.\n", failures);
        return EXIT_FAILURE;
    }

    puts("Todas las formulas superan la validacion estructural.");
    return EXIT_SUCCESS;
}
