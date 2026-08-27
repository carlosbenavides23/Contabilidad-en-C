param(
    [Parameter(Mandatory = $false)]
    [string]$Path = "output.xlsx"
)

$ErrorActionPreference = "Stop"

function Assert-True {
    param(
        [bool]$Condition,
        [string]$Message
    )

    if (-not $Condition) {
        throw "Validacion fallida: $Message"
    }
}

function Read-ZipText {
    param(
        [System.IO.Compression.ZipArchive]$Archive,
        [string]$EntryName
    )

    $entry = $Archive.GetEntry($EntryName)
    Assert-True ($null -ne $entry) "Falta la entrada $EntryName"
    $reader = [System.IO.StreamReader]::new($entry.Open())
    try {
        return $reader.ReadToEnd()
    }
    finally {
        $reader.Dispose()
    }
}

$resolved = (Resolve-Path -LiteralPath $Path).Path
$archive = [System.IO.Compression.ZipFile]::OpenRead($resolved)

try {
    $workbook = Read-ZipText $archive "xl/workbook.xml"
    $decoding = Read-ZipText $archive "xl/worksheets/sheet4.xml"
    $consolidated = Read-ZipText $archive "xl/worksheets/sheet5.xml"
    $audit = Read-ZipText $archive "xl/worksheets/sheet6.xml"
    $adjustment = Read-ZipText $archive "xl/worksheets/sheet7.xml"
    $temporal = Read-ZipText $archive "xl/worksheets/sheet8.xml"

    $sheetNames = @(
        "Transacciones",
        "Catalogo_Tasas",
        "Topes_Presupuesto",
        "Decodificacion",
        "Consolidado",
        "Auditoria",
        "Ajuste_Contable",
        "Reconstruccion_Temporal"
    )
    foreach ($sheetName in $sheetNames) {
        Assert-True ($workbook.Contains("name=`"$sheetName`"")) "Hoja $sheetName"
    }

    Assert-True ($workbook.Contains('fullCalcOnLoad="1"')) "Recálculo al abrir"
    Assert-True ($decoding.Contains('t="array" ref="A2:A500"')) "CSE de decodificación"
    Assert-True ($decoding.Contains("Catalogo_Tasas!`$B`$2:`$B`$20")) "Lookup de tasa"
    Assert-True ($decoding.Contains(")-(IFERROR(VALUE(MID(")) "Resta de retención en R1"
    Assert-True (-not $decoding.Contains(")*(IFERROR(VALUE(MID(")) "Ausencia de multiplicación incorrecta en R1"
    Assert-True ($consolidated.Contains("SUMPRODUCT(")) "Consolidado SUMPRODUCT"
    Assert-True ($consolidated.Contains("TEXT(")) "Rama porcentual de presupuesto"
    Assert-True ($consolidated.Contains("LN(1+")) "Penalización logarítmica"
    Assert-True (([regex]::Matches($audit, 't="array"')).Count -eq 25) "25 fórmulas CSE de auditoría"
    Assert-True ($audit.Contains("LARGE(")) "Ranking LARGE"
    Assert-True ($audit.Contains("/1000000")) "Desempate por fila"
    Assert-True ($adjustment.Contains('t="array" ref="G1:G4"')) "CSE vertical G1:G4"
    Assert-True ($adjustment.Contains("MMULT(MINVERSE(")) "Solución matricial"
    Assert-True ($adjustment.Contains("MDETERM(")) "Control de determinante"
    for ($index = 0; $index -lt 4; $index++) {
        $adjustmentRow = $index + 1
        $consolidatedRow = $index + 4
        $pattern = '<c r="E' + $adjustmentRow + '"[^>]*><f>SUM\(Consolidado!B' +
                   $consolidatedRow + ':M' + $consolidatedRow + '\)</f>'
        Assert-True ([regex]::IsMatch($adjustment, $pattern)) "Vector B formula E$adjustmentRow"
    }
    Assert-True (([regex]::Matches($temporal, 't="array"')).Count -eq 499) "499 CSE temporales"
    Assert-True ($temporal.Contains("MATCH(2,1/")) "Búsqueda del vecino anterior"
    Assert-True ($temporal.Contains("SIN_VECINOS_VALIDOS")) "Control de bordes temporales"

    Write-Output "XLSX válido: 8 hojas, fórmulas esperadas y recálculo confirmado."
}
finally {
    $archive.Dispose()
}
