$ErrorActionPreference = 'Stop'
$configuration = Join-Path $PSScriptRoot '.env'
if (-not (Test-Path -LiteralPath $configuration)) {
    $lines = foreach ($name in @('AEON_DB_PASSWORD','AEON_GRAPH_PASSWORD','AEON_API_TOKEN')) {
        $bytes = New-Object byte[] 32
        $generator = [Security.Cryptography.RandomNumberGenerator]::Create()
        $generator.GetBytes($bytes)
        $generator.Dispose()
        $secret = [BitConverter]::ToString($bytes).Replace('-','').ToLowerInvariant()
        "$name=$secret"
    }
    [IO.File]::WriteAllLines($configuration, $lines)
}
if (-not (Get-Command docker -ErrorAction SilentlyContinue)) {
    Write-Host 'Configuration created. Install and start Docker Desktop, then run this script again.'
    exit 1
}
docker compose --env-file $configuration -f (Join-Path $PSScriptRoot 'compose.yaml') up --build -d
exit $LASTEXITCODE
