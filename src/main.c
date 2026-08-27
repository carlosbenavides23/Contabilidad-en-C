#include <stdio.h>

#include "data.h"
#include "workbook.h"

static void print_usage(const char *program_name)
{
    fprintf(stderr,
            "Uso:\n"
            "  %s\n"
            "  %s output.xlsx\n"
            "  %s transacciones.csv catalogo_tasas.csv "
            "topes_presupuesto.csv output.xlsx\n",
            program_name,
            program_name,
            program_name);
}

int main(int argc, char **argv)
{
    Dataset dataset = {0};
    const char *output_path = "output.xlsx";
    char error_message[512] = {0};
    int status;

    if (argc == 1 || argc == 2) {
        if (argc == 2) {
            output_path = argv[1];
        }
        status = dataset_init_demo(&dataset, error_message, sizeof(error_message));
    } else if (argc == 5) {
        output_path = argv[4];
        status = dataset_load_csv(&dataset,
                                  argv[1],
                                  argv[2],
                                  argv[3],
                                  error_message,
                                  sizeof(error_message));
    } else {
        print_usage(argv[0]);
        return 2;
    }

    if (status != 0) {
        fprintf(stderr, "Error al cargar datos: %s\n", error_message);
        dataset_free(&dataset);
        return 1;
    }

    status = generate_financial_workbook(output_path,
                                         &dataset,
                                         error_message,
                                         sizeof(error_message));
    dataset_free(&dataset);

    if (status != 0) {
        fprintf(stderr, "Error al generar el libro: %s\n", error_message);
        return 1;
    }

    printf("Libro generado: %s\n", output_path);
    printf("Excel o LibreOffice Calc recalculara las formulas al abrirlo.\n");
    return 0;
}

