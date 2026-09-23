param(
    [string]$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path,
    [string]$ShortcutName = 'Humanity Trinity Space'
)

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing

$brandDirectory = Join-Path $ProjectRoot 'Assets\Brand'
$launcherPath = Join-Path $ProjectRoot 'Launch_HumanityTrinityRebuild.cmd'
$iconPath = Join-Path $brandDirectory 'HumanityTrinityRebuild.ico'
$previewPath = Join-Path $brandDirectory 'HumanityTrinityRebuild_Icon.png'
$desktopPath = [Environment]::GetFolderPath('Desktop')
$shortcutPath = Join-Path $desktopPath ($ShortcutName + '.lnk')

if (-not (Test-Path -LiteralPath $launcherPath)) {
    throw "Main menu launcher not found: $launcherPath"
}
[System.IO.Directory]::CreateDirectory($brandDirectory) | Out-Null

function Scale-Value([float]$Value, [int]$Size) {
    return [single]($Value * $Size / 256.0)
}

function New-RoundedRectanglePath([float]$X, [float]$Y, [float]$Width, [float]$Height, [float]$Radius) {
    $path = [System.Drawing.Drawing2D.GraphicsPath]::new()
    $diameter = 2 * $Radius
    $path.AddArc($X, $Y, $diameter, $diameter, 180, 90)
    $path.AddArc($X + $Width - $diameter, $Y, $diameter, $diameter, 270, 90)
    $path.AddArc($X + $Width - $diameter, $Y + $Height - $diameter, $diameter, $diameter, 0, 90)
    $path.AddArc($X, $Y + $Height - $diameter, $diameter, $diameter, 90, 90)
    $path.CloseFigure()
    return $path
}

function New-BrandBitmap([int]$Size) {
    $bitmap = [System.Drawing.Bitmap]::new($Size, $Size, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
    $graphics.Clear([System.Drawing.Color]::Transparent)
    $graphics.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
    $graphics.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
    $graphics.CompositingQuality = [System.Drawing.Drawing2D.CompositingQuality]::HighQuality

    $scale = $Size / 256.0
    $outer = New-RoundedRectanglePath (Scale-Value 8 $Size) (Scale-Value 8 $Size) (Scale-Value 240 $Size) (Scale-Value 240 $Size) (Scale-Value 42 $Size)
    $background = [System.Drawing.Drawing2D.LinearGradientBrush]::new(
        [System.Drawing.PointF]::new(0, 0),
        [System.Drawing.PointF]::new($Size, $Size),
        [System.Drawing.Color]::FromArgb(255, 25, 34, 45),
        [System.Drawing.Color]::FromArgb(255, 8, 14, 22))
    $graphics.FillPath($background, $outer)
    $border = [System.Drawing.Pen]::new([System.Drawing.Color]::FromArgb(255, 226, 205, 157), [Math]::Max(1.0, 4.0 * $scale))
    $graphics.DrawPath($border, $outer)

    # Burgundy curtains frame the room. Subtle fold lines remain legible at
    # medium sizes while the two open inner edges point toward the table ring.
    $leftCurtain = [System.Drawing.PointF[]]@(
        [System.Drawing.PointF]::new((Scale-Value 28 $Size), (Scale-Value 30 $Size)),
        [System.Drawing.PointF]::new((Scale-Value 88 $Size), (Scale-Value 30 $Size)),
        [System.Drawing.PointF]::new((Scale-Value 75 $Size), (Scale-Value 157 $Size)),
        [System.Drawing.PointF]::new((Scale-Value 28 $Size), (Scale-Value 177 $Size)))
    $rightCurtain = [System.Drawing.PointF[]]@(
        [System.Drawing.PointF]::new((Scale-Value 168 $Size), (Scale-Value 30 $Size)),
        [System.Drawing.PointF]::new((Scale-Value 228 $Size), (Scale-Value 30 $Size)),
        [System.Drawing.PointF]::new((Scale-Value 228 $Size), (Scale-Value 177 $Size)),
        [System.Drawing.PointF]::new((Scale-Value 181 $Size), (Scale-Value 157 $Size)))
    $curtainBrush = [System.Drawing.Drawing2D.LinearGradientBrush]::new(
        [System.Drawing.PointF]::new((Scale-Value 28 $Size), 0),
        [System.Drawing.PointF]::new((Scale-Value 228 $Size), 0),
        [System.Drawing.Color]::FromArgb(255, 104, 11, 36),
        [System.Drawing.Color]::FromArgb(255, 151, 27, 56))
    $graphics.FillPolygon($curtainBrush, $leftCurtain)
    $graphics.FillPolygon($curtainBrush, $rightCurtain)
    $foldPen = [System.Drawing.Pen]::new([System.Drawing.Color]::FromArgb(125, 244, 116, 130), [Math]::Max(0.8, 2.0 * $scale))
    foreach ($x in 40, 55, 70) {
        $graphics.DrawLine($foldPen, (Scale-Value $x $Size), (Scale-Value 35 $Size), (Scale-Value ($x - 4) $Size), (Scale-Value 164 $Size))
        $graphics.DrawLine($foldPen, (Scale-Value (256 - $x) $Size), (Scale-Value 35 $Size), (Scale-Value (260 - $x) $Size), (Scale-Value 164 $Size))
    }

    # Six congruent trapezoids form the characteristic hollow regular hexagon.
    $outerPoints = @()
    $innerPoints = @()
    for ($index = 0; $index -lt 6; $index++) {
        $angle = (-90 + 60 * $index) * [Math]::PI / 180.0
        $outerPoints += [System.Drawing.PointF]::new(
            (Scale-Value (128 + 58 * [Math]::Cos($angle)) $Size),
            (Scale-Value (107 + 58 * [Math]::Sin($angle)) $Size))
        $innerPoints += [System.Drawing.PointF]::new(
            (Scale-Value (128 + 27 * [Math]::Cos($angle)) $Size),
            (Scale-Value (107 + 27 * [Math]::Sin($angle)) $Size))
    }
    $tableColors = @(
        [System.Drawing.Color]::FromArgb(255, 38, 183, 224),
        [System.Drawing.Color]::FromArgb(255, 247, 199, 54),
        [System.Drawing.Color]::FromArgb(255, 238, 231, 211),
        [System.Drawing.Color]::FromArgb(255, 38, 183, 224),
        [System.Drawing.Color]::FromArgb(255, 247, 199, 54),
        [System.Drawing.Color]::FromArgb(255, 238, 231, 211))
    $tableEdge = [System.Drawing.Pen]::new([System.Drawing.Color]::FromArgb(215, 5, 17, 27), [Math]::Max(0.7, 1.8 * $scale))
    for ($index = 0; $index -lt 6; $index++) {
        $next = ($index + 1) % 6
        $piece = [System.Drawing.PointF[]]@($outerPoints[$index], $outerPoints[$next], $innerPoints[$next], $innerPoints[$index])
        $brush = [System.Drawing.SolidBrush]::new($tableColors[$index])
        $graphics.FillPolygon($brush, $piece)
        $graphics.DrawPolygon($tableEdge, $piece)
        $brush.Dispose()
    }

    # A pale oak trapezoid reads as the raised rear stage beneath the curtains.
    $stage = [System.Drawing.PointF[]]@(
        [System.Drawing.PointF]::new((Scale-Value 43 $Size), (Scale-Value 171 $Size)),
        [System.Drawing.PointF]::new((Scale-Value 213 $Size), (Scale-Value 171 $Size)),
        [System.Drawing.PointF]::new((Scale-Value 231 $Size), (Scale-Value 226 $Size)),
        [System.Drawing.PointF]::new((Scale-Value 25 $Size), (Scale-Value 226 $Size)))
    $stageBrush = [System.Drawing.Drawing2D.LinearGradientBrush]::new(
        [System.Drawing.PointF]::new(0, (Scale-Value 171 $Size)),
        [System.Drawing.PointF]::new(0, (Scale-Value 226 $Size)),
        [System.Drawing.Color]::FromArgb(255, 238, 195, 126),
        [System.Drawing.Color]::FromArgb(255, 173, 110, 52))
    $graphics.FillPolygon($stageBrush, $stage)
    $stageEdge = [System.Drawing.Pen]::new([System.Drawing.Color]::FromArgb(255, 255, 225, 169), [Math]::Max(1.0, 3.0 * $scale))
    $graphics.DrawPolygon($stageEdge, $stage)
    $plankPen = [System.Drawing.Pen]::new([System.Drawing.Color]::FromArgb(110, 86, 43, 19), [Math]::Max(0.6, 1.3 * $scale))
    foreach ($y in 184, 197, 210) {
        $graphics.DrawLine($plankPen, (Scale-Value 34 $Size), (Scale-Value $y $Size), (Scale-Value 222 $Size), (Scale-Value $y $Size))
    }

    foreach ($resource in $background, $border, $curtainBrush, $foldPen, $tableEdge, $stageBrush, $stageEdge, $plankPen, $outer) {
        $resource.Dispose()
    }
    $graphics.Dispose()
    return $bitmap
}

# Keep one lossless preview and build a multi-resolution PNG-compressed ICO.
$previewBitmap = New-BrandBitmap 512
$previewBitmap.Save($previewPath, [System.Drawing.Imaging.ImageFormat]::Png)
$previewBitmap.Dispose()

$sizes = @(16, 24, 32, 48, 64, 128, 256)
$frames = @()
foreach ($size in $sizes) {
    $bitmap = New-BrandBitmap $size
    $stream = [System.IO.MemoryStream]::new()
    $bitmap.Save($stream, [System.Drawing.Imaging.ImageFormat]::Png)
    $frames += ,$stream.ToArray()
    $stream.Dispose()
    $bitmap.Dispose()
}
$file = [System.IO.File]::Open($iconPath, [System.IO.FileMode]::Create)
$writer = [System.IO.BinaryWriter]::new($file)
$writer.Write([uint16]0)
$writer.Write([uint16]1)
$writer.Write([uint16]$sizes.Count)
$offset = 6 + 16 * $sizes.Count
for ($index = 0; $index -lt $sizes.Count; $index++) {
    $encodedSize = if ($sizes[$index] -ge 256) { 0 } else { $sizes[$index] }
    $writer.Write([byte]$encodedSize)
    $writer.Write([byte]$encodedSize)
    $writer.Write([byte]0)
    $writer.Write([byte]0)
    $writer.Write([uint16]1)
    $writer.Write([uint16]32)
    $writer.Write([uint32]$frames[$index].Length)
    $writer.Write([uint32]$offset)
    $offset += $frames[$index].Length
}
foreach ($frame in $frames) {
    $writer.Write($frame)
}
$writer.Dispose()
$file.Dispose()

$shell = New-Object -ComObject WScript.Shell
$shortcut = $shell.CreateShortcut($shortcutPath)
$shortcut.TargetPath = $launcherPath
$shortcut.WorkingDirectory = $ProjectRoot
$shortcut.IconLocation = "$iconPath,0"
$shortcut.Description = 'Choose walkthrough or hide-and-seek in the Humanity Trinity Space reconstruction'
$shortcut.WindowStyle = 7
$shortcut.Save()

# Read back the persisted shell-link properties rather than trusting creation.
$verified = $shell.CreateShortcut($shortcutPath)
if ($verified.TargetPath -ne $launcherPath -or $verified.WorkingDirectory -ne $ProjectRoot) {
    throw 'The desktop shortcut was created but its saved target is incorrect.'
}
if (-not (Test-Path -LiteralPath $iconPath) -or (Get-Item -LiteralPath $iconPath).Length -lt 1024) {
    throw 'The Windows icon was not generated correctly.'
}

[pscustomobject]@{
    Shortcut = $shortcutPath
    Target = $verified.TargetPath
    WorkingDirectory = $verified.WorkingDirectory
    Icon = $verified.IconLocation
    Preview = $previewPath
}
