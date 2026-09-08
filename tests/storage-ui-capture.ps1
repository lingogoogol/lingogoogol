param(
    [Parameter(Mandatory = $true)]
    [int]$ProcessId,
    [Parameter(Mandatory = $true)]
    [string]$OutputPath
)

$ErrorActionPreference = 'Stop'

Add-Type -AssemblyName System.Drawing
Add-Type @'
using System;
using System.Runtime.InteropServices;

public static class WindowCaptureNative {
    [StructLayout(LayoutKind.Sequential)]
    public struct RECT { public int Left, Top, Right, Bottom; }

    [DllImport("user32.dll")]
    public static extern bool GetWindowRect(IntPtr hwnd, out RECT rect);

    [DllImport("user32.dll")]
    public static extern bool PrintWindow(IntPtr hwnd, IntPtr hdc, uint flags);

    [DllImport("user32.dll")]
    public static extern bool SetProcessDPIAware();
}
'@

[WindowCaptureNative]::SetProcessDPIAware() | Out-Null

$process = Get-Process -Id $ProcessId
$handle = $process.MainWindowHandle
if ($handle -eq [IntPtr]::Zero) {
    throw 'The process does not have a main window.'
}

$rect = New-Object WindowCaptureNative+RECT
if (-not [WindowCaptureNative]::GetWindowRect($handle, [ref]$rect)) {
    throw 'GetWindowRect failed.'
}
$width = $rect.Right - $rect.Left
$height = $rect.Bottom - $rect.Top
if ($width -le 0 -or $height -le 0) {
    throw "Invalid window size: ${width}x${height}."
}

$bitmap = New-Object System.Drawing.Bitmap($width, $height)
$graphics = [System.Drawing.Graphics]::FromImage($bitmap)
try {
    $hdc = $graphics.GetHdc()
    try {
        if (-not [WindowCaptureNative]::PrintWindow($handle, $hdc, 2)) {
            throw 'PrintWindow failed.'
        }
    }
    finally {
        $graphics.ReleaseHdc($hdc)
    }
    $bitmap.Save($OutputPath, [System.Drawing.Imaging.ImageFormat]::Png)

    $sampleCount = 0
    $blackCount = 0
    for ($y = 0; $y -lt $height; $y += 4) {
        for ($x = 0; $x -lt $width; $x += 4) {
            $pixel = $bitmap.GetPixel($x, $y)
            ++$sampleCount
            if ($pixel.R -le 5 -and $pixel.G -le 5 -and $pixel.B -le 5) {
                ++$blackCount
            }
        }
    }
    $blackRatio = $blackCount / $sampleCount
    $headerMinY = $null
    $headerMaxY = $null
    for ($headerY = 0; $headerY -lt [Math]::Min(40, $height); ++$headerY) {
        for ($headerX = 0; $headerX -lt [Math]::Min(200, $width); ++$headerX) {
            $pixel = $bitmap.GetPixel($headerX, $headerY)
            if ($pixel.R -gt 10 -or $pixel.G -gt 10 -or $pixel.B -gt 10) {
                if ($null -eq $headerMinY) { $headerMinY = $headerY }
                $headerMaxY = $headerY
            }
        }
    }
    $headerTextHeight = if ($null -eq $headerMinY) { 0 } else { $headerMaxY - $headerMinY + 1 }
    $solidWhiteBlocks = 0
    for ($blockY = 0; $blockY -le [Math]::Min($height - 8, [int]($height / 4)); $blockY += 8) {
        for ($blockX = 0; $blockX -le $width - 8; $blockX += 8) {
            $allWhite = $true
            for ($pixelY = $blockY; $pixelY -lt $blockY + 8 -and $allWhite; ++$pixelY) {
                for ($pixelX = $blockX; $pixelX -lt $blockX + 8; ++$pixelX) {
                    $pixel = $bitmap.GetPixel($pixelX, $pixelY)
                    if ($pixel.R -lt 250 -or $pixel.G -lt 250 -or $pixel.B -lt 250) {
                        $allWhite = $false
                        break
                    }
                }
            }
            if ($allWhite) {
                ++$solidWhiteBlocks
            }
        }
    }
    "WINDOW_SIZE=${width}x${height}"
    'BLACK_PIXEL_RATIO={0:N4}' -f $blackRatio
    "HEADER_TEXT_HEIGHT=$headerTextHeight"
    "SOLID_WHITE_BLOCKS=$solidWhiteBlocks"
    if ($blackRatio -ge 0.95 -or $solidWhiteBlocks -gt 0 -or $headerTextHeight -lt 14) {
        'UI_RENDER_VERDICT=RED'
        exit 1
    }
    'UI_RENDER_VERDICT=GREEN'
}
finally {
    $graphics.Dispose()
    $bitmap.Dispose()
}
