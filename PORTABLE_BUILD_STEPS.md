# Portable C++ Build Configuration - Manual Steps

## For RowaPickupSlim Portable Executable

### Visual Studio Configuration (GUI Method - RECOMMENDED)

1. **Open Visual Studio Project**
   - Open: `I:\VSprojects\VCPP\RowaPickupSlim\RowaPickupSlim\RowaPickupSlim\RowaPickupSlim.vcxproj`

2. **Configure Release|x64**
   - Solution Explorer: Right-click project ? Properties
   - Configuration: Select **Release**
   - Platform: Select **x64**
   - Left panel: C/C++ ? Code Generation
   - **Runtime Library**: Change to **Multi-threaded (/MT)**
   - Click Apply

3. **Configure Release|Win32**
   - Configuration: Select **Release**
   - Platform: Select **Win32**
   - Left panel: C/C++ ? Code Generation
   - **Runtime Library**: Change to **Multi-threaded (/MT)**
   - Click Apply ? OK

4. **Build Portable Release**
   - In Visual Studio: Select **Release | x64** from configuration dropdown
   - Build ? Build Solution
   - Output: `bin\Release\x64\RowaPickupSlim.exe`

### Verification

After building, verify the executable is portable:

```powershell
# Check runtime dependencies
dumpbin /dependents "bin\Release\x64\RowaPickupSlim.exe"
```

Expected output should show:
- ? KERNEL32.dll
- ? USER32.dll  
- ? GDI32.dll
- ? (NO) MSVCR120.dll or similar runtime DLL

### Result

Your `RowaPickupSlim.exe` can now run on any Windows system without:
- Visual C++ Redistributable
- .NET Framework
- External libraries

**Ready for distribution!** ??
