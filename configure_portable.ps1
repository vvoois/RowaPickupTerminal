# Configure RowaPickupSlim for Portable Build
# This script adds static runtime linking to Release configurations

[xml]$xml = Get-Content "I:\VSprojects\VCPP\RowaPickupSlim\RowaPickupSlim\RowaPickupSlim\RowaPickupSlim.vcxproj"

# Namespace for MSBuild
$ns = @{
    'msb' = 'http://schemas.microsoft.com/developer/msbuild/2003'
}

# Find Release x64 ClCompile settings
$xpath_x64 = "//msb:ItemDefinitionGroup[@Condition='$(Configuration)|$(Platform)==Release|x64']//msb:ClCompile"
$releaseX64 = $xml.SelectSingleNode($xpath_x64, $ns)
if ($releaseX64) {
    if (-not $releaseX64.RuntimeLibrary) {
        $runtimeLib = $xml.CreateElement("RuntimeLibrary", $ns['msb'])
        $runtimeLib.InnerText = "MultiThreaded"
        $releaseX64.AppendChild($runtimeLib) | Out-Null
        Write-Host "? Added RuntimeLibrary=MultiThreaded to Release|x64 ClCompile" -ForegroundColor Green
    }
} else {
    Write-Host "? Release|x64 ClCompile not found, searching for ItemDefinitionGroups..." -ForegroundColor Yellow
    $xml.SelectNodes("//msb:ItemDefinitionGroup", $ns) | ForEach-Object {
        Write-Host "  Found: $($_.Condition)" 
    }
}

# Find Release Win32 ClCompile settings
$xpath_win32 = "//msb:ItemDefinitionGroup[@Condition='$(Configuration)|$(Platform)==Release|Win32']//msb:ClCompile"
$releaseWin32 = $xml.SelectSingleNode($xpath_win32, $ns)
if ($releaseWin32) {
    if (-not $releaseWin32.RuntimeLibrary) {
        $runtimeLib = $xml.CreateElement("RuntimeLibrary", $ns['msb'])
        $runtimeLib.InnerText = "MultiThreaded"
        $releaseWin32.AppendChild($runtimeLib) | Out-Null
        Write-Host "? Added RuntimeLibrary=MultiThreaded to Release|Win32 ClCompile" -ForegroundColor Green
    }
}

# Save updated project
$settings = New-Object System.Xml.XmlWriterSettings
$settings.Indent = $true
$settings.IndentChars = "  "

$writer = [System.Xml.XmlWriter]::Create("I:\VSprojects\VCPP\RowaPickupSlim\RowaPickupSlim\RowaPickupSlim\RowaPickupSlim.vcxproj", $settings)
$xml.WriteTo($writer)
$writer.Close()

Write-Host "`nProject file updated successfully!" -ForegroundColor Green
Write-Host "`nNext steps:`n" -ForegroundColor Cyan
Write-Host "1. Open the project in Visual Studio"
Write-Host "2. Select Release|x64 configuration"
Write-Host "3. Build ? Build Solution"
Write-Host "4. Output: bin\Release\x64\RowaPickupSlim.exe`n"
