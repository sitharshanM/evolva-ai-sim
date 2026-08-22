# OpenStreetMap Tile Stitcher for AEON

Add-Type -AssemblyName System.Drawing

function Stitch-TileSet([string]$type, [string]$outFile) {
    Write-Host "Stitching $type OpenStreetMap tiles into $outFile..."
    $canvas = New-Object System.Drawing.Bitmap(2048, 2048)
    $g = [System.Drawing.Graphics]::FromImage($canvas)
    $g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic

    for ($x = 0; $x -lt 8; $x++) {
        for ($y = 0; $y -lt 8; $y++) {
            $tileFile = "assets/tiles/$type/${x}_${y}.png"
            if (Test-Path $tileFile) {
                $absPath = (Resolve-Path $tileFile).Path
                $img = [System.Drawing.Image]::FromFile($absPath)
                $g.DrawImage($img, $x * 256, $y * 256, 256, 256)
                $img.Dispose()
            }
        }
    }
    $g.Dispose()

    $targetPath = Join-Path (Get-Location) $outFile
    $canvas.Save($targetPath, [System.Drawing.Imaging.ImageFormat]::Png)
    $canvas.Dispose()
    Write-Host "Saved $outFile (2048x2048 PNG)."
}

Stitch-TileSet "dark" "assets/osm_dark_world.png"
Stitch-TileSet "street" "assets/osm_street_world.png"
Stitch-TileSet "satellite" "assets/osm_satellite_world.png"
