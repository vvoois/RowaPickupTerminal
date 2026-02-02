<!-- PowerShell script to fix PugiXML compilation -->
[xml]$xml = Get-Content "I:\VSprojects\VCPP\RowaPickupSlim\RowaPickupSlim\RowaPickupSlim\RowaPickupSlim.vcxproj"

# Remove NuGet pugixml imports
$importGroups = $xml.SelectNodes("//ImportGroup")
foreach ($group in $importGroups) {
    $imports = $group.SelectNodes("Import[@Project[contains(., 'pugixml')]]")
    foreach ($import in $imports) {
        $group.RemoveChild($import) | Out-Null
        Write-Host "Removed: $($import.Project)"
    }
}

# Remove NuGet pugixml error check
$targets = $xml.SelectNodes("//Target")
foreach ($target in $targets) {
    $errors = $target.SelectNodes("Error[@Text[contains(., 'pugixml')]]")
    foreach ($error in $errors) {
        $target.RemoveChild($error) | Out-Null
        Write-Host "Removed error check: $($.Condition)"
    }
}

# Add pugixml.cpp as a compilation source if not already there
$clCompiles = $xml.SelectNodes("//ClCompile[@Include='pugixml.cpp']")
if ($clCompiles.Count -eq 0) {
    $itemGroup = $xml.CreateElement("ItemGroup")
    $clItem = $xml.CreateElement("ClCompile")
    $clItem.SetAttribute("Include", "pugixml.cpp")
    $itemGroup.AppendChild($clItem) | Out-Null
    $xml.DocumentElement.AppendChild($itemGroup) | Out-Null
    Write-Host "Added pugixml.cpp as ClCompile item"
}

# Save
$xml.Save("I:\VSprojects\VCPP\RowaPickupSlim\RowaPickupSlim\RowaPickupSlim\RowaPickupSlim.vcxproj")
Write-Host "? Project file updated!"
Write-Host ""
Write-Host "Next steps:"
Write-Host "1. Reload project in Visual Studio"
Write-Host "2. Build ? Clean Solution"
Write-Host "3. Build ? Build Solution"
Write-Host ""
