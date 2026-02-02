# StockInfo Request/Response Debug Analysis

## Problem Identified ??

The StockInfoRequest **IS being sent**, but the list remains empty because:

### **Issue 1: RobotStockLocation = "None"** ??
**Line 20 in SharedVariables.cpp:**
```cpp
std::string SharedVariables::RobotStockLocation = "None";
```

**StockInfoRequest being sent:**
```xml
<StockInfoRequest ... >
  <Criteria StockLocationId="None" />
</StockInfoRequest>
```

? The robot doesn't have a stock location with ID "None", so it returns empty!

---

## Solution

### **Step 1: Check Machine Settings**
In the app, click **File > Settings** and verify:
- **Stock Location**: Should match your robot's actual stock location ID
  - Common values: `"01"`, `"1"`, `"FL01"`, `"BIN1"`, etc.
  - Check with your robot configuration

### **Step 2: Update SharedVariables.cpp**

Once you know the correct location ID, update the default:

```cpp
std::string SharedVariables::RobotStockLocation = "FL01";  // Or whatever your robot uses
```

**OR** set it through the Settings dialog before connecting.

---

## Secondary Issue: Empty Name/Unit Columns

**Line 469 in main.cpp:**
```cpp
ss << "<StockInfoRequest ... IncludeArticleDetails=\"False\" >";
```

This is why columns "Naam", "Eenheid", "Vorm" are empty!

**To show article details, change to:**
```cpp
ss << "<StockInfoRequest ... IncludeArticleDetails=\"True\" >";
```

**Then update the parsing in main.cpp lines 251-260** to extract and display:
- `<Article ... Name="..." />`
- `<Article ... PackagingUnit="..." />`  
- `<Article ... DosageForm="..." />`

---

## Also Check: Destination ID

**Line 465 in main.cpp:**
```cpp
ss << "<StockInfoRequest ... Destination=\"999\" >";
```

`"999"` might not match your robot. It should typically be:
- `"0"` (broadcast)
- `"1"` (single robot)
- Robot-specific ID from StatusResponse

---

## Debugging Steps

1. **Enable logging** - Add this to see what's being sent/received:
```cpp
OutputDebugStringA("StockInfoRequest sent...\n");
OutputDebugStringA(xml.c_str());  // Log the response
```

2. **Check robot response** - Does StatusResponse include the correct StockLocationId?

3. **Verify config file** - Check what's actually saved:
```
C:\ProgramData\RowaPickupMAUI\RowaPickupMaui.config
```

---

## Quick Fix Checklist

- [ ] Open Settings dialog
- [ ] Set "Stock Location" to correct ID (e.g., "FL01")
- [ ] Click Save
- [ ] Restart app
- [ ] Check if table populates
- [ ] If not, enable ArticleDetails=True for name/unit columns

