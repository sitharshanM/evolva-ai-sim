# OpenStreetMap & Esri Satellite Tile Downloader Script for AEON

param(
    [string]$TileType = "dark"
)

$ErrorActionPreference = "Stop"

$urlTemplates = @{
    "dark"      = "https://a.basemaps.cartocdn.com/dark_all/3/{x}/{y}.png"
    "street"    = "https://tile.openstreetmap.org/3/{x}/{y}.png"
    "satellite" = "https://server.arcgisonline.com/ArcGIS/rest/services/World_Imagery/MapServer/tile/3/{y}/{x}"
}

$template = $urlTemplates[$TileType]
if (-not $template) {
    $template = $urlTemplates["dark"]
}

$outputDir = "assets/tiles/$TileType"
New-Item -ItemType Directory -Force -Path $outputDir | Out-Null

Write-Host "Fetching $TileType map tiles from OpenStreetMap API..."

for ($x = 0; $x -lt 8; $x++) {
    for ($y = 0; $y -lt 8; $y++) {
        $url = $template.Replace("{x}", $x.ToString()).Replace("{y}", $y.ToString())
        $outFile = "$outputDir/${x}_${y}.png"
        
        if (-not (Test-Path $outFile)) {
            try {
                Invoke-WebRequest -Uri $url -OutFile $outFile -UserAgent "AeonSimulator/1.0 (contact@aeon.org)" -TimeoutSec 10
                Write-Host "Downloaded tile [$x, $y]"
            } catch {
                Write-Warning "Failed to fetch tile [$x, $y] from $url"
            }
        }
    }
}

Write-Host "Finished fetching $TileType OpenStreetMap tiles."
