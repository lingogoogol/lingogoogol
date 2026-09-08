param(
    [Parameter(Mandatory)]
    [int]$ProcessId,

    [Parameter(Mandatory)]
    [ValidateSet('upload-folder', 'upload-file')]
    [string]$Action
)

$ErrorActionPreference = 'Stop'

Add-Type @'
using System;
using System.Runtime.InteropServices;

public static class StorageActionCrashNative {
    [StructLayout(LayoutKind.Sequential)]
    public struct RECT { public int Left, Top, Right, Bottom; }

    [DllImport("user32.dll")]
    public static extern IntPtr SetThreadDpiAwarenessContext(IntPtr value);
    [DllImport("user32.dll")]
    public static extern bool GetWindowRect(IntPtr window, out RECT rect);
    [DllImport("user32.dll")]
    public static extern bool SetForegroundWindow(IntPtr window);
    [DllImport("user32.dll")]
    public static extern bool SetCursorPos(int x, int y);
    [DllImport("user32.dll")]
    public static extern void mouse_event(uint flags, uint x, uint y, uint data, UIntPtr extraInfo);
    [DllImport("user32.dll")]
    public static extern void keybd_event(byte virtualKey, byte scanCode, uint flags, UIntPtr extraInfo);
}
'@

[StorageActionCrashNative]::SetThreadDpiAwarenessContext([IntPtr](-4)) | Out-Null
$process = Get-Process -Id $ProcessId
$process.Refresh()
$window = [IntPtr]$process.MainWindowHandle
if ($window -eq [IntPtr]::Zero) {
    throw 'The storage process does not have a visible main window.'
}

$rect = New-Object StorageActionCrashNative+RECT
[StorageActionCrashNative]::GetWindowRect($window, [ref]$rect) | Out-Null
$width = $rect.Right - $rect.Left
$height = $rect.Bottom - $rect.Top
$xRatio = if ($Action -eq 'upload-folder') { 0.828 } else { 0.895 }
$x = $rect.Left + [int]($width * $xRatio)
$y = $rect.Top + [int]($height * 0.0235)

[StorageActionCrashNative]::SetForegroundWindow($window) | Out-Null
[StorageActionCrashNative]::SetCursorPos($rect.Left + [int]($width * 0.50), $y) | Out-Null
Start-Sleep -Milliseconds 100
[StorageActionCrashNative]::SetCursorPos($x, $y) | Out-Null
Start-Sleep -Milliseconds 150
[StorageActionCrashNative]::mouse_event(2, 0, 0, 0, [UIntPtr]::Zero)
[StorageActionCrashNative]::mouse_event(4, 0, 0, 0, [UIntPtr]::Zero)
Start-Sleep -Seconds 2

$process = Get-Process -Id $ProcessId -ErrorAction SilentlyContinue
if (-not $process) {
    throw "storage crashed after action: $Action"
}
$process.Refresh()
if ($process.MainWindowHandle -eq 0) {
    throw "storage lost its visible window after action: $Action"
}

[StorageActionCrashNative]::keybd_event(0x1B, 0, 0, [UIntPtr]::Zero)
[StorageActionCrashNative]::keybd_event(0x1B, 0, 2, [UIntPtr]::Zero)
Start-Sleep -Milliseconds 300
Write-Host "storage remained alive after action: $Action"
