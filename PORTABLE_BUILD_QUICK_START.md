# RowaPickupSlim Portable Build Summary

## What You Need

For a fully **portable C++ Win32 application** with no external dependencies:

### 1. **Static C++ Runtime (/MT)**
   - Release|x64: Runtime Library = **Multi-threaded (/MT)**
   - Release|Win32: Runtime Library = **Multi-threaded (/MT)**

### 2. **PugiXML Library**
   - Compile statically with `/MT`
   - No separate DLL needed

### 3. **Windows APIs Only**
   - GDI32 (included with Windows)
   - USER32 (included with Windows)
   - KERNEL32 (included with Windows)

## Configuration Checklist

- [ ] Open RowaPickupSlim.vcxproj in Visual Studio
- [ ] Project Properties ? C/C++ ? Code Generation
- [ ] Set Runtime Library to `/MT` for Release|x64
- [ ] Set Runtime Library to `/MT` for Release|Win32
- [ ] Rebuild solution
- [ ] Test executable on clean Windows machine

## Build Steps

### Visual Studio GUI
```
1. Configuration dropdown: Select "Release | x64"
2. Build ? Build Solution
3. Output: bin\Release\x64\RowaPickupSlim.exe
```

### Command Line
```powershell
cd "I:\VSprojects\VCPP\RowaPickupSlim\RowaPickupSlim\RowaPickupSlim"
msbuild RowaPickupSlim.vcxproj /p:Configuration=Release /p:Platform=x64
```

## Result

| Item | Status |
|------|--------|
| Executable Size | Small (< 10MB) |
| Dependencies | None |
| Runtime Required | None |
| Visual C++ Redistributable | Not needed |
| .NET Framework | Not needed |
| Windows Version | Win7+ |

## Deployment

Copy only:
- `RowaPickupSlim.exe`
- `RowaPickupMaui.config` (optional)
- `README.txt` (optional)

**That's it!** No installation, no dependencies, just run the .exe ??
