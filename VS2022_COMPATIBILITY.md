# VS 2026 (v145) Primary - Optional VS 2022 Downgrade Guide

## Current Development Environment

? **Primary**: Visual Studio 2026 (v145) – Fully supported & tested  
?? **Legacy**: Visual Studio 2022 (v143) – Community builds only

## If You Want to Support VS 2022 (Optional)

**This is NOT required for contributing.** This guide is for maintainers who want to support legacy builds.

### Requirements for Dual-Version Support
- Access to both VS 2026 and VS 2022
- Testing on both platforms before release
- Limiting C++ features to C++17 baseline

### Steps to Enable VS 2022 Builds

#### 1. Modify .vcxproj (For v143 variant)

Create a **second project configuration** in Visual Studio:

```xml
<PropertyGroup Label="Configuration" Condition="'$(PlatformToolset)'=='v143'">
    <PlatformToolset>v143</PlatformToolset>
    <LanguageStandard>stdcpp17</LanguageStandard>
    <WindowsTargetPlatformVersion>10.0.22000</WindowsTargetPlatformVersion>
</PropertyGroup>

<PropertyGroup Label="Configuration" Condition="'$(PlatformToolset)'=='v145'">
    <PlatformToolset>v145</PlatformToolset>
    <LanguageStandard>stdcpp17</LanguageStandard>
    <WindowsTargetPlatformVersion>10.0.26100</WindowsTargetPlatformVersion>
</PropertyGroup>
```

#### 2. Test Build on VS 2022

- Install VS 2022 on test machine
- Clone repo
- Open `.sln` – VS 2022 auto-migrates to v143
- Build in Release mode
- Test all features

#### 3. Document Results

If successful, add to README:
```markdown
### Legacy Support
- VS 2022 (v143) builds supported but not regularly tested
- Contributors welcome to file issues for v143 compatibility
```

## When NOT to Support VS 2022

? **Keep v145-only if:**
- You want to use newer C++20 features
- Maintenance burden becomes significant
- Most users are already on VS 2026+
- No community demand for v143 support

## Current Status: v145 Primary Only ?

**This project targets VS 2026 (v145) minimum.** Backporting to VS 2022 is optional and handled separately if needed.

## For Contributors

- **Build with**: VS 2026 (v145)
- **Test target**: v145 platform toolset
- **Do NOT use**: C++20/23 features (keep C++17)
- **Report issues**: Include VS version in bug reports

---

**Legacy support for VS 2022 is NOT required.** Use this guide only if you specifically want to maintain v143 compatibility.

