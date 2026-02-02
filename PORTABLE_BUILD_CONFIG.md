# Portable Build Configuration Guide for RowaPickupSlim

## Visual Studio Project Configuration

### Step 1: Update C++ Runtime Library
For **PORTABLE** executable, set to STATIC runtime (/MT):

**Release|x64 Configuration:**
- Project Properties ? C/C++ ? Code Generation ? Runtime Library
- Change to: **Multi-threaded** (`/MT`)

**Release|Win32 Configuration:**
- Project Properties ? C/C++ ? Code Generation ? Runtime Library
- Change to: **Multi-threaded** (`/MT`)

### Step 2: Linker Settings
Project Properties ? Linker:
- Subsystem: **Windows**
- Generate Debug Info: **No** (for smaller executable)

### Step 3: PugiXML Configuration
Ensure PugiXML is compiled with:
- Runtime Library: **Multi-threaded** (`/MT`)
- No DLL dependencies

### Step 4: Build Release

Using Visual Studio:
```
1. Select "Release | x64" configuration
2. Build ? Build Solution
3. Output: bin\Release\x64\RowaPickupSlim.exe
```

Using Command Line:
```powershell
cd "I:\VSprojects\VCPP\RowaPickupSlim\RowaPickupSlim\RowaPickupSlim"
msbuild RowaPickupSlim.vcxproj /p:Configuration=Release /p:Platform=x64
```

## Verification Checklist

? Executable runs on any Windows 7+ system
? No Visual C++ Redistributable needed
? No .NET Framework needed
? Single .exe file (+ config files)
? All dependencies statically linked

## Deployment Package Structure

```
RowaPickupSlim_Portable/
??? RowaPickupSlim.exe          (Main executable)
??? RowaPickupMaui.config       (Settings file)
??? README.txt                  (Usage instructions)
??? LICENSE                     (License info)
```

## Required Files at Runtime

Only these folders/files are needed on target system:
- `RowaPickupSlim.exe`
- `RowaPickupMaui.config` (optional, created on first run)
- Windows OS (Win7+)

**That's it! No dependencies!** ??
