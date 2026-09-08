param(
    [Parameter(Mandatory)]
    [int]$ProcessId,

    [string]$ScreenshotPath = (Join-Path $PSScriptRoot 'storage-clear-input-check.png'),

    [switch]$SafeConfirm
)

$ErrorActionPreference = 'Stop'

Add-Type -AssemblyName System.Drawing
Add-Type @'
using System;
using System.Runtime.InteropServices;

public static class StorageClearInputNative {
    [StructLayout(LayoutKind.Sequential)]
    public struct RECT { public int Left, Top, Right, Bottom; }

    [DllImport("user32.dll")]
    public static extern IntPtr SetThreadDpiAwarenessContext(IntPtr value);
    [DllImport("user32.dll")]
    public static extern bool GetWindowRect(IntPtr window, out RECT rect);
    [DllImport("user32.dll")]
    public static extern bool SetWindowPos(IntPtr window, IntPtr insertAfter, int x, int y, int width, int height, uint flags);
    [DllImport("user32.dll")]
    public static extern bool SetForegroundWindow(IntPtr window);
    [DllImport("user32.dll")]
    public static extern bool SetCursorPos(int x, int y);
    [DllImport("user32.dll")]
    public static extern void mouse_event(uint flags, uint x, uint y, uint data, UIntPtr extraInfo);
    [DllImport("user32.dll")]
    public static extern bool PrintWindow(IntPtr window, IntPtr destination, uint flags);
    [DllImport("user32.dll", EntryPoint="PostMessageW")]
    public static extern bool PostMessageW(IntPtr window, uint message, UIntPtr wParam, IntPtr lParam);

    public static bool PostCodePoints(IntPtr window, uint first, uint second) {
        return PostMessageW(window, 0x0102, new UIntPtr(first), new IntPtr(1))
            && PostMessageW(window, 0x0102, new UIntPtr(second), new IntPtr(1));
    }
}
'@

[StorageClearInputNative]::SetThreadDpiAwarenessContext([IntPtr](-4)) | Out-Null
$process = Get-Process -Id $ProcessId
$process.Refresh()
$window = [IntPtr]$process.MainWindowHandle
if ($window -eq [IntPtr]::Zero) {
    throw 'The storage process does not have a visible main window.'
}

$rect = New-Object StorageClearInputNative+RECT
[StorageClearInputNative]::GetWindowRect($window, [ref]$rect) | Out-Null
$width = $rect.Right - $rect.Left
$height = $rect.Bottom - $rect.Top

# The clear button is anchored to the top-right of the borderless storage window.
$clearX = $rect.Left + [int]($width * 0.9635)
$clearY = $rect.Top + [int]($height * 0.0235)

[StorageClearInputNative]::SetWindowPos($window, [IntPtr](-1), 0, 0, 0, 0, 0x43) | Out-Null
[StorageClearInputNative]::SetForegroundWindow($window) | Out-Null
[StorageClearInputNative]::SetCursorPos($rect.Left + [int]($width * 0.50), $clearY) | Out-Null
Start-Sleep -Milliseconds 100
[StorageClearInputNative]::SetCursorPos($clearX, $clearY) | Out-Null
Start-Sleep -Milliseconds 150
[StorageClearInputNative]::mouse_event(2, 0, 0, 0, [UIntPtr]::Zero)
[StorageClearInputNative]::mouse_event(4, 0, 0, 0, [UIntPtr]::Zero)
Start-Sleep -Milliseconds 500

$bitmap = New-Object System.Drawing.Bitmap $width, $height
$graphics = [System.Drawing.Graphics]::FromImage($bitmap)
$deviceContext = $graphics.GetHdc()
$printed = [StorageClearInputNative]::PrintWindow($window, $deviceContext, 2)
$graphics.ReleaseHdc($deviceContext)
[StorageClearInputNative]::SetWindowPos($window, [IntPtr](-2), 0, 0, 0, 0, 0x43) | Out-Null

if (-not $printed) {
    $graphics.Dispose()
    $bitmap.Dispose()
    throw 'Unable to capture the storage window.'
}

$bitmap.Save($ScreenshotPath, [System.Drawing.Imaging.ImageFormat]::Png)

$minimumInputWidth = 256
$maximumInputWidth = 640
$foundWideInputUnderline = $false
$inputStartX = 0
$inputUnderlineY = 0
$startX = [int]($width * 0.12)
$endX = [int]($width * 0.60)
$startY = [int]($height * 0.35)
$endY = [int]($height * 0.70)

for ($y = $startY; $y -le $endY -and -not $foundWideInputUnderline; $y++) {
    $runStart = -1
    for ($x = $startX; $x -le $endX; $x++) {
        $pixel = $bitmap.GetPixel($x, $y)
        $isInputHighlight = $pixel.R -ge 220 -and $pixel.G -ge 220 -and $pixel.B -le 80
        if ($isInputHighlight -and $runStart -lt 0) {
            $runStart = $x
        }
        elseif (-not $isInputHighlight -and $runStart -ge 0) {
            $runLength = $x - $runStart
            if ($runLength -ge $minimumInputWidth -and $runLength -le $maximumInputWidth) {
                $foundWideInputUnderline = $true
                $inputStartX = $runStart
                $inputUnderlineY = $y
                break
            }
            $runStart = -1
        }
    }
}

if (-not $foundWideInputUnderline) {
    $graphics.Dispose()
    $bitmap.Dispose()
    throw "The clear dialog does not expose a highlighted input field at least $minimumInputWidth pixels wide. Screenshot: $ScreenshotPath"
}

$inputX = $rect.Left + $inputStartX + 24
$inputY = $rect.Top + $inputUnderlineY - 20
[StorageClearInputNative]::SetWindowPos($window, [IntPtr](-1), 0, 0, 0, 0, 0x43) | Out-Null
[StorageClearInputNative]::SetCursorPos($rect.Left + [int]($width * 0.50), $clearY) | Out-Null
Start-Sleep -Milliseconds 100
[StorageClearInputNative]::SetCursorPos($inputX, $inputY) | Out-Null
Start-Sleep -Milliseconds 150
[StorageClearInputNative]::mouse_event(2, 0, 0, 0, [UIntPtr]::Zero)
[StorageClearInputNative]::mouse_event(4, 0, 0, 0, [UIntPtr]::Zero)
Start-Sleep -Milliseconds 150
if ($SafeConfirm) {
    # "測試" exercises the confirmation callback without enqueueing delete_all.
    [StorageClearInputNative]::PostCodePoints($window, 28204, 35430) | Out-Null
}
else {
    [StorageClearInputNative]::PostCodePoints($window, 28165, 38500) | Out-Null
}
Start-Sleep -Milliseconds 400

$typedBitmap = New-Object System.Drawing.Bitmap $width, $height
$typedGraphics = [System.Drawing.Graphics]::FromImage($typedBitmap)
$typedDeviceContext = $typedGraphics.GetHdc()
$typedPrinted = [StorageClearInputNative]::PrintWindow($window, $typedDeviceContext, 2)
$typedGraphics.ReleaseHdc($typedDeviceContext)
[StorageClearInputNative]::SetWindowPos($window, [IntPtr](-2), 0, 0, 0, 0, 0x43) | Out-Null

$typedScreenshotPath = [System.IO.Path]::ChangeExtension($ScreenshotPath, '.typed.png')
$typedBitmap.Save($typedScreenshotPath, [System.Drawing.Imaging.ImageFormat]::Png)
$changedPixels = 0
for ($x = $inputStartX; $x -lt [Math]::Min($inputStartX + $maximumInputWidth, $width); $x++) {
    for ($y = [Math]::Max(0, $inputUnderlineY - 40); $y -lt $inputUnderlineY - 2; $y++) {
        if ($bitmap.GetPixel($x, $y).ToArgb() -ne $typedBitmap.GetPixel($x, $y).ToArgb()) {
            $changedPixels++
        }
    }
}

$typedGraphics.Dispose()
$typedBitmap.Dispose()
$graphics.Dispose()
$bitmap.Dispose()

if (-not $typedPrinted -or $changedPixels -lt 20) {
    throw "The clear input field did not render the typed text. Screenshot: $typedScreenshotPath"
}

if ($SafeConfirm) {
    [StorageClearInputNative]::SetWindowPos($window, [IntPtr](-1), 0, 0, 0, 0, 0x43) | Out-Null
    [StorageClearInputNative]::SetCursorPos($rect.Left + $inputStartX + 48, $rect.Top + $inputUnderlineY + 26) | Out-Null
    Start-Sleep -Milliseconds 150
    [StorageClearInputNative]::mouse_event(2, 0, 0, 0, [UIntPtr]::Zero)
    [StorageClearInputNative]::mouse_event(4, 0, 0, 0, [UIntPtr]::Zero)
    Start-Sleep -Seconds 2
    $process = Get-Process -Id $ProcessId -ErrorAction SilentlyContinue
    if (-not $process) {
        throw 'storage crashed while closing the clear confirmation dialog.'
    }
    $process.Refresh()
    if ($process.MainWindowHandle -eq 0) {
        throw 'storage lost its visible window while closing the clear confirmation dialog.'
    }
}

Write-Host "Clear dialog input box is visible and accepts text. Screenshot: $typedScreenshotPath"
