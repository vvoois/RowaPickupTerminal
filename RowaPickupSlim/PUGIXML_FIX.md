# Fix PugiXML Linker Error

## Problem
The project is trying to link against `pugixml.lib` which either:
1. Doesn't exist in the expected path
2. Isn't needed (pugixml is header-only)

## Solutions

### Option 1: Use PugiXML as Header-Only (Recommended)
PugiXML is primarily a header-only library. Just include `pugixml.hpp` and you're done.

**Steps:**
1. Download pugixml from https://pugixml.org/
2. Extract the ZIP file
3. Copy `pugixml.hpp` to your RowaPickupSlim project folder
4. Remove pugixml.lib from linker settings:
   - Right-click project ? Properties
   - Linker ? Input ? Additional Dependencies
   - Remove any pugixml.lib reference
5. Add to Additional Include Directories:
   - C++ ? General ? Additional Include Directories
   - Add: `$(ProjectDir)`

**Current .cpp file uses:**
```cpp
#include "pugixml.hpp"  // ? Header-only, no .lib needed
```

---

### Option 2: Install via NuGet (If you want .lib version)

1. Project ? Manage NuGet Packages
2. Search for: `pugixml`
3. Install the package
4. Visual Studio will auto-configure paths
5. Verify linker settings are correct

---

### Option 3: Manual Path Fix (If you have pugixml.lib)

If you have the compiled pugixml.lib:

1. Right-click project ? Properties
2. VC++ Directories tab:
   - Library Directories: Add path to pugixml.lib location
3. Linker ? Input:
   - Additional Dependencies: Add `pugixml.lib`

---

## Recommended Fix (Right Now)

Since **pugixml is already included as header-only** in your code:

### Step 1: Create `pugixml.hpp` if missing
Download from https://pugixml.org/files/pugixml-1.15.zip

Extract and copy `src/pugixml.hpp` and `src/pugixml.cpp` to your project.

### Step 2: Update project settings
- Remove `pugixml.lib` from linker dependencies
- Keep the include path pointing to pugixml.hpp location

### Step 3: Rebuild
```
Build ? Clean Solution
Build ? Rebuild Solution
```

---

## Current Status

? Code already uses header-only approach:
```cpp
#include "pugixml.hpp"  // Works without .lib
```

? Linker incorrectly configured to look for non-existent .lib file

---

## Quick Command to Remove Linker Dependency

If using command line:
```bash
# Remove from Additional Dependencies in .vcxproj
# Or in Project Properties ? Linker ? Input
# Delete: pugixml.lib
```

---

## File Already in Use
Your `XmlDefinitions.cpp` already includes:
```cpp
#include "pugixml.hpp"
```

This works as-is WITHOUT needing `pugixml.lib`!

---

**Next Action:** 
1. Ensure `pugixml.hpp` exists in project folder
2. Remove pugixml.lib from linker settings
3. Rebuild - should work!
