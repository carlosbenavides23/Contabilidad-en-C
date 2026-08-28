# Generador de conciliación financiera XLSX en C

Proyecto C17 para Windows que genera un libro `output.xlsx` con fórmulas
financieras clásicas, fórmulas matriciales CSE y datos cargados desde CSV o
desde un dataset integrado. El ejecutable no calcula el modelo: escribe datos,
rangos, estilos y fórmulas mediante libxlsxwriter. Excel o LibreOffice Calc
realizan el cálculo al abrir el archivo.

## Estructura

```text
.
├── CMakeLists.txt
├── vcpkg.json
├── README.md
├── data/
│   ├── transacciones.csv
│   ├── catalogo_tasas.csv
│   └── topes_presupuesto.csv
├── src/
│   ├── main.c
│   ├── data.c
│   ├── data.h
│   ├── formulas.c
│   ├── formulas.h
│   ├── workbook.c
│   └── workbook.h
└── tests/
    ├── formula_tests.c
    └── inspect_xlsx.ps1
```

`data.c` solo carga los valores base. `formulas.c` construye texto de fórmulas
con memoria dinámica. `workbook.c` crea las ocho hojas y escribe esas fórmulas.
No hay parseo financiero ni precálculo en C.

## Requisitos y compilación exacta en Windows

Se recomienda Visual Studio 2022 o posterior con el componente **Desktop
development with C++**, CMake, Git y vcpkg. El manifiesto fija libxlsxwriter
1.2.x mediante un `builtin-baseline` reproducible.

Desde una consola **Developer PowerShell for VS**:

```powershell
git clone https://github.com/microsoft/vcpkg C:\dev\vcpkg
C:\dev\vcpkg\bootstrap-vcpkg.bat

cmake -S . -B build -A x64 `
  -DCMAKE_TOOLCHAIN_FILE=C:/dev/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Release --parallel 4
ctest --test-dir build -C Release --output-on-failure

.\build\Release\financial_reconciliation.exe
```

El modo manifiesto instala automáticamente `libxlsxwriter`. La alternativa
explícita es:

```powershell
C:\dev\vcpkg\vcpkg.exe install libxlsxwriter:x64-windows
```

Con el triplet dinámico `x64-windows`, la integración de vcpkg copia las DLL
necesarias junto al ejecutable. También puede utilizarse Ninja:

```powershell
cmake -S . -B build -G Ninja `
  -DCMAKE_BUILD_TYPE=Release `
  -DCMAKE_TOOLCHAIN_FILE=C:/dev/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build
ctest --test-dir build --output-on-failure
```

El proyecto activa `/W4 /utf-8 /permissive-` con MSVC y
`-Wall -Wextra -Wpedantic -Wconversion` con GCC/Clang.

## Backend nativo R1 (C + Fortran)

`financial_reconciliation_native` carga los tres CSV de `data/`. C separa las
transacciones crudas y resuelve tasa y retencion mediante el catalogo; Fortran
recibe los arrays numericos y calcula el vector de montos netos. El ejecutable
solo muestra resultados numericos en la terminal y no genera archivos XLSX.

Las lineas completamente vacias se omiten. Una transaccion con estructura
invalida, monto no numerico o codigo fiscal desconocido detiene el proceso con
un error, sin sustituir valores. Las fechas no se interpretan en R1, por lo que
los marcadores de fecha presentes siguen procesandose; su tratamiento queda
reservado para R6.

## Ejecución

Dataset integrado y nombre predeterminado:

```powershell
.\build\Release\financial_reconciliation.exe
```

Dataset integrado y salida elegida:

```powershell
.\build\Release\financial_reconciliation.exe mi_modelo.xlsx
```

Carga de CSV:

```powershell
.\build\Release\financial_reconciliation.exe `
  data\transacciones.csv `
  data\catalogo_tasas.csv `
  data\topes_presupuesto.csv `
  output_csv.xlsx
```

Los CSV aceptan campos ordinarios o entre comillas, con comillas duplicadas,
pero no registros que contengan saltos de línea dentro de un campo. Los límites
son 499 transacciones, 19 tasas y 9 topes, iguales a los rangos del modelo.

## Convenciones XLSX

libxlsxwriter recibe funciones internas en inglés, comas como separadores de
argumentos, punto decimal y referencias A1. Por ejemplo, se escribe `IF`,
`INDEX`, `MATCH` y `SUMPRODUCT`, no sus nombres localizados. La interfaz de
Excel o Calc puede mostrarlos traducidos.

Las fórmulas normales se entregan como `=...`. Las CSE se entregan como
`{=...}` a `worksheet_write_array_formula()`. En el XML, libxlsxwriter elimina
los delimitadores de interfaz y escribe, por ejemplo:

```xml
<f t="array" ref="G1:G4">IF(...,MMULT(MINVERSE(...),...),...)</f>
```

El formato generado es XLSX, no ODF. Las fórmulas se limitan a operaciones y
funciones clásicas con equivalentes directos en Calc; no se usan matrices
dinámicas ni funciones de Microsoft 365.

## Construcción y validación de las fórmulas

Las expresiones siguientes usan `R = Transacciones!$A$2:$A$500` para que sean
legibles. `src/formulas.c` expande siempre `R` y todas las referencias completas.

### 1. Decodificación matricial

Conceptualmente, para cada fila:

```text
p1 = FIND("#", R)
p2 = FIND("#", R, p1 + 1)
p3 = FIND("#", R, p2 + 1)
p4 = FIND("#", R, p3 + 1)
M  = VALUE(MID(R, p3 + 1, p4 - p3 - 1))
C  = MID(R, p4 + 1, LEN(R) - p4)
T  = INDEX(Catalogo_Tasas!$B$2:$B$20,
           MATCH(C, Catalogo_Tasas!$A$2:$A$20, 0))
Ret = INDEX(Catalogo_Tasas!$C$2:$C$20,
            MATCH(C, Catalogo_Tasas!$A$2:$A$20, 0))
MontoNeto = M * (1 + T) - (M * Ret)
```

Para TX001 el resultado comprobable es
`900 * 1.15 - 900 * 0.02 = 1035 - 18 = 1017.00`.

Representación XLSX resumida:

```excel
{=IF(R<>"",M*(1+IFERROR(INDEX(...,MATCH(C,...,0)),0))
             -(M*IFERROR(INDEX(...,MATCH(C,...,0)),0)),"")}
```

Se almacena como una CSE con referencia `A2:A500` en `Decodificacion`. En C,
`build_hash_position()`, `build_amount_expression()`,
`build_tax_code_expression()` y `build_net_amount_formula()` ensamblan la
cadena; `write_decoding_sheet()` la pasa a
`worksheet_write_array_formula()`.

### 2. Consolidado multicriterio

Conceptualmente:

```text
SUMPRODUCT(
  (year(R) = $B$1) *
  (month(R) = $B$2) *
  (center(R) = $A4) *
  (amount(R) > average_nonblank(amount(R))) *
  amount(R)
)
```

Representación XLSX resumida:

```excel
=SUMPRODUCT((yearExpr=$B$1)*(monthExpr=$B$2)*(centerExpr=$A4)*
 (amountExpr>(SUMPRODUCT((R<>"")*amountExpr)/SUMPRODUCT(--(R<>""))))*
 amountExpr)
```

No hay columnas auxiliares. `build_consolidated_formula()` genera la fórmula
y `write_consolidated_sheet()` la escribe en `N4:N...`. `B1` selecciona año y
`B2` mes.

### 3. Matriz mensual de desviación

La decisión de layout es `A4:A...` para centros, `B3:M3` para meses y
`B4:M...` para resultados. Para cada celda:

```text
S = SUMPRODUCT((year=$B$1)*(month=B$3)*(center=$A4)*amount)
L = INDEX(limits, MATCH($A4, budget_centers, 0))
P = INDEX(penalties, MATCH($A4, budget_centers, 0))

IF(S <= L,
   TEXT((L-S)/L, "0.00%"),
   (S-L)*(1+P)^2 + (((S-L)^2/L)*LN(1+P)))
```

Representación XLSX:

```excel
=IF(S<=L,TEXT((L-S)/L,"0.00%"),
 (S-L)*(1+P)^2+(((S-L)^2/L)*LN(1+P)))
```

`S`, `L` y `P` se expanden cada vez; no son nombres ni celdas auxiliares.
`build_monthly_sum_expression()`, `build_budget_lookup()` y
`build_budget_formula()` hacen esa expansión. Una verificación manual es abril
de `CC_ESTE`: `S=450`, `L=2000`, por lo que devuelve texto `77.50%`. Para
`CC_OESTE`, `S=5200`, `L=4000`, `P=0.05`, la rama numérica es aproximadamente
`1340.56`.

### 4. Top 5 de anomalías

Conceptualmente:

```text
rank_amount = amount(R) +
              (ROW(R)-ROW(Transacciones!$A$2)+1)/1000000
k = ROW()-1
position = MATCH(LARGE(rank_amount, k), rank_amount, 0)
result = INDEX(parsed_field(R), position)
```

Representación XLSX para la columna de ID:

```excel
{=INDEX(idExpr,
   MATCH(LARGE(amountExpr+(ROW(R)-ROW(Transacciones!$A$2)+1)/1000000,
               ROW()-1),
         amountExpr+(ROW(R)-ROW(Transacciones!$A$2)+1)/1000000,0))}
```

`build_top5_formula()` produce una CSE por celda de `A2:E6`; las otras columnas
reutilizan la posición para devolver monto, fecha, centro y código fiscal. Con
el dataset entregado, el orden esperado es TX008 (5200), TX019 (4200), TX022
(3600, fila posterior), TX006 (3600) y TX015 (3100). Así se comprueba el
desempate sin ordenar datos en C.

### 5. Álgebra lineal

Conceptualmente:

```text
det = MDETERM(A1:D4)
B_i = SUM(Consolidado!B_fila:M_fila)
X = MMULT(MINVERSE(A1:D4), E1:E4)
```

`E1:E4` contiene fórmulas `SUM` que acumulan la desviación/penalización anual
de cada centro desde las doce celdas mensuales de `Consolidado`. `SUM` ignora
los porcentajes almacenados como texto y agrega únicamente los excesos
numéricos; C no calcula el vector B.

Representación XLSX:

```excel
=IF(ABS(MDETERM($A$1:$D$4))<1E-12,
    "MATRIZ_SINGULAR",MDETERM($A$1:$D$4))

{=IF(ABS(MDETERM($A$1:$D$4))<1E-12,"MATRIZ_SINGULAR",
     IFERROR(MMULT(MINVERSE($A$1:$D$4),$E$1:$E$4),
             "MATRIZ_SINGULAR"))}
```

La segunda se escribe como una sola CSE vertical `G1:G4`. El `IF` evita llamar
lógicamente a la inversa cuando el determinante es casi cero y `IFERROR` añade
una defensa compatible frente a diferencias de evaluación entre motores.
`build_annual_deviation_formula()`, `build_determinant_formula()` y
`build_linear_solution_formula()` construyen las cadenas.

### 6. Reconstrucción temporal

Para cada posición `k`, la validez comprueba estructura, conversión numérica y
que `TEXT(DATE(...),"yyyy-mm-dd")` coincida con el texto original. Esto también
rechaza fechas normalizadas como `2023-99-99`.

Conceptualmente:

```text
valid = canonical_yyyy_mm_dd(date(R))
prev = MATCH(2, 1/(valid*(ROW(R)<ROW())))
next = MATCH(1, valid*(ROW(R)>ROW()), 0)

Yest = INDEX(date_serials,prev)
       + (k-prev)
       * ((INDEX(date_serials,next)-INDEX(date_serials,prev))/(next-prev))
```

Representación XLSX resumida:

```excel
{=IF(currentRaw="","",
   IF(currentValid,currentDateSerial,
      IFERROR(previousDate+(ROW()-ROW(Transacciones!$A$2)+1-prev)*
              ((nextDate-previousDate)/(next-prev)),
              "SIN_VECINOS_VALIDOS")))}
```

`build_date_serial_expression()`, `build_date_valid_expression()`, las dos
búsquedas con `MATCH` y `build_temporal_reconstruction_formula()` generan una
CSE autocontenida por fila en `Reconstruccion_Temporal!A2:A500`. No existen
columnas auxiliares y los vecinos pueden estar arbitrariamente lejos. En el
dataset, TX009 y TX010 quedan entre 2023-04-08 y 2023-04-20; los valores
esperados son 2023-04-12 y 2023-04-16.

Se conserva `INDEX` para recuperar los seriales anterior y posterior. `OFFSET`
solo puede desplazar una referencia de hoja; no puede tomar como referencia el
vector calculado por `build_date_serial_expression()`. La sustitución clásica
correcta tendría que desplazar `Transacciones!$A$2` hasta la celda RAW hallada
por `MATCH` y repetir allí todo el análisis `DATE/MID/FIND/VALUE`. Al expandir
esa variante en los tres usos de la fecha anterior y el uso de la siguiente,
la fórmula resultante mide 20.876 caracteres, frente a 7.197 con `INDEX`, y
supera el límite de 8.192 de Excel. Acortarla mediante `DATEVALUE` o coerción
directa del texto ISO introduciría dependencia regional y dejaría de ser una
alternativa defendible para Excel heredado y Calc. Por tanto, forzar `OFFSET`
empeoraría tanto la validez como la compatibilidad de R6.

La comprobación del caso de demostración usa posiciones relativas 8, 9, 10 y
11: entre 2023-04-08 y 2023-04-20 hay 12 días repartidos en tres intervalos.
La interpolación conserva exactamente cuatro días por intervalo, por lo que
las posiciones inválidas 9 y 10 producen 2023-04-12 y 2023-04-16.

## Recálculo

libxlsxwriter no evalúa fórmulas. Por defecto genera en `xl/workbook.xml`:

```xml
<calcPr calcId="124519" fullCalcOnLoad="1"/>
```

Por ello el libro solicita recálculo completo al abrirse. Los valores en caché
del XML son cero hasta que Excel o Calc recalcule y guarde el archivo. No se usa
Excel, COM, VBA, macros ni un motor de fórmulas desde C.

## Pruebas y comprobación manual

Validación automatizada:

```powershell
ctest --test-dir build -C Release --output-on-failure
powershell -ExecutionPolicy Bypass -File tests\inspect_xlsx.ps1 output.xlsx
```

`formula_tests` verifica que cada constructor produzca los tokens esperados y
que ninguna fórmula supere 8192 caracteres. `inspect_xlsx.ps1` abre el ZIP de
forma no destructiva y comprueba hojas, rangos CSE, funciones clave y
`fullCalcOnLoad`.

Comprobación en Excel o LibreOffice Calc:

1. Abra `output.xlsx` y permita el recálculo.
2. En `Decodificacion`, confirme TX001 contra el cálculo manual anterior.
3. En `Consolidado`, cambie `B1/B2` y verifique el subtotal multicriterio y las
   ramas de abril indicadas para `CC_ESTE` y `CC_OESTE`.
4. En `Auditoria`, confirme el orden esperado, incluidos los dos montos 3600.
5. En `Ajuste_Contable`, confirme que `E1:E4` suma los excesos numéricos de
   cada fila de `Consolidado` y que `G1:G4` sigue resolviendo `A*X=B`.
6. En `Reconstruccion_Temporal`, confirme 2023-04-12 y 2023-04-16 en las filas
   de TX009 y TX010.

## Limitaciones y decisiones técnicas

- **Requerimiento 3:** la rama bajo límite devuelve texto por exigencia de
  `TEXT`; la rama sobre límite devuelve número. La matriz es deliberadamente de
  tipo mixto.
- **Requerimiento 6:** una CSE multicelda clásica puede devolver un vector, pero
  se evalúa como una sola expresión matricial; no equivale a copiar la fórmula
  y no crea 499 contextos escalares independientes para el patrón actual
  `MATCH(...ROW(R)<ROW()...)`. Si `ROW()` se sustituye por el vector 1..499, la
  comparación de dos vectores verticales es elemento a elemento, no las 499
  búsquedas completas requeridas. La vectorización clásica directa necesita
  construir una máscara triangular 499x499, reducirla por fila y entregar los
  ordinales resultantes a `MATCH`; al expandir `TRANSPOSE/MMULT`, la validación
  de fecha y la interpolación sin nombres ni auxiliares, la cadena mide 14.065
  caracteres. `OFFSET` tampoco aporta la iteración por fila: devuelve una
  referencia desplazada y su sustitución directa mide 20.876 caracteres. Ambas
  exceden el límite de 8.192 y la segunda además obliga a volver a analizar el
  texto RAW. Sin auxiliares ni `MAP`, `BYROW` o `LAMBDA`, no hay una única CSE
  defendible que satisfaga simultáneamente la semántica, el límite y la
  compatibilidad exigidos. Se mantienen 499 CSE clásicas autocontenidas, una
  por posición. La búsqueda anterior y posterior sí es arbitrariamente
  distante y no usa auxiliares; por ello R6 continúa marcado como parcial.
- La compatibilidad con Calc es a través de su importador XLSX y del subconjunto
  clásico utilizado; no se afirma que el archivo sea ODF/OpenFormula.
- Las pruebas automatizadas validan estructura y sintaxis almacenada. La
  evaluación numérica final corresponde necesariamente a Excel o Calc.

## Estado final

| Requerimiento | Implementado | Método | Limitaciones |
| --- | --- | --- | --- |
| 1. Decodificación matricial | Sí | CSE A2:A500, FIND/MID/LEN/VALUE e INDEX/MATCH; resta la retención | Ninguna conocida |
| 2. Consolidado multicriterio | Sí | Una fórmula SUMPRODUCT por centro, sin auxiliares | Fechas inválidas quedan fuera por IFERROR |
| 3. Matriz de desviación | Sí | IF único con S/L/P expandidos y repetidos | Resultado mixto texto/número exigido |
| 4. Top 5 | Sí | CSE INDEX/MATCH/LARGE/ROW con desempate 1e-6 | Supone montos no negativos para que blancos no entren al top |
| 5. Álgebra lineal | Sí | B acumula desviaciones de Consolidado; CSE G1:G4 con MMULT/MINVERSE y guardia MDETERM | El motor externo realiza el cálculo |
| 6. Reconstrucción temporal | Parcial | Una CSE autocontenida por fila, MATCH anterior/posterior arbitrario | No es una sola CSE multiresultado para A2:A500 |
| CSV y dataset | Sí | Loader CSV y 24 filas integradas | Sin saltos de línea dentro de campos CSV |
| Formato y recálculo | Sí | Estilos, anchos, paneles y fullCalcOnLoad | Valores cacheados en cero hasta abrir/recalcular |

## Referencias técnicas

- [libxlsxwriter: Working with Formulas](https://libxlsxwriter.github.io/working_with_formulas.html)
- [libxlsxwriter: array_formula.c](https://libxlsxwriter.github.io/array_formula_8c-example.html)
- [libxlsxwriter: Getting Started](https://libxlsxwriter.github.io/getting_started.html)
- [vcpkg: paquete libxlsxwriter](https://vcpkg.io/en/package/libxlsxwriter.html)
