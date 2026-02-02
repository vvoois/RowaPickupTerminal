# C++ GDI32 UI Implementation Guide

## Problem
The C++ application compiles and runs but shows an **empty window** because:
1. The XAML-based MAUI UI is **not migrated** to C++ GDI32
2. GDI32 requires **manual window drawing** via WM_PAINT
3. Current main.cpp has UI placeholder code but **no complete implementation**

## Solution: Add GDI32-Based UI

### UI Elements Needed (from MainPage.xaml)

| MAUI Element | C++ GDI32 Equivalent | Purpose |
|-------------|---------------------|---------|
| TitleBar | Window title + header area | Connection status display |
| HeaderGrid | Table header row | Column labels |
| CollectionView | Paint loop with rows | Article list display |
| Buttons (Uitladen) | Clickable regions | Output requests |
| Entry (SearchPhrase) | Edit control | Search input |
| Button (Zoek) | Button control | Search trigger |
| Label (Connection) | Static text | Status display |

---

## Implementation Steps

### Step 1: Add UI Constants to main.cpp

```cpp
// Color constants for GDI32
static const COLORREF CLR_TITLEBAR = RGB(51, 51, 51);     // #333333
static const COLORREF CLR_HEADER_BG = RGB(238, 238, 238); // #EEE
static const COLORREF CLR_TEXT = RGB(0, 0, 0);
static const COLORREF CLR_WHITE = RGB(255, 255, 255);
static const COLORREF CLR_BLACK = RGB(0, 0, 0);

// Window dimensions
const int TITLE_BAR_HEIGHT = 32;
const int HEADER_HEIGHT = 25;
const int ROW_HEIGHT = 28;
const int PADDING = 3;

// Column widths (from MainPage.xaml)
const int COL_BUTTON = 75;
const int COL_QTY = 75;
const int COL_ARTICLE_ID = 250;
const int COL_NAME = 400;
const int COL_UNIT = 75;
const int COL_FORM = 75;

// Control IDs for window messages
enum ControlID {
    ID_SEARCH_EDIT = 1001,
    ID_SEARCH_BUTTON = 1002,
    ID_REFRESH_BUTTON = 1003
};
```

### Step 2: Create Window Controls

```cpp
case WM_CREATE:
{
    // Create search text box
    HWND hSearchEdit = CreateWindowExW(
        WS_EX_CLIENTEDGE,
        L"EDIT",
        L"",
        WS_CHILD | WS_VISIBLE | ES_LEFT | ES_AUTOHSCROLL,
        150, hWnd_height - 35,  // Position at bottom
        500, 22,
        hWnd,
        (HMENU)(intptr_t)ID_SEARCH_EDIT,
        (HINSTANCE)GetModuleHandleW(NULL),
        NULL
    );

    // Create search button
    HWND hSearchBtn = CreateWindowW(
        L"BUTTON",
        L"Zoek",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        660, hWnd_height - 35,
        50, 22,
        hWnd,
        (HMENU)(intptr_t)ID_SEARCH_BUTTON,
        (HINSTANCE)GetModuleHandleW(NULL),
        NULL
    );

    // Create refresh button
    HWND hRefreshBtn = CreateWindowW(
        L"BUTTON",
        L"Verversen",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        720, hWnd_height - 35,
        80, 22,
        hWnd,
        (HMENU)(intptr_t)ID_REFRESH_BUTTON,
        (HINSTANCE)GetModuleHandleW(NULL),
        NULL
    );
    break;
}
```

### Step 3: Enhance WM_PAINT for Complete UI

```cpp
case WM_PAINT:
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hWnd, &ps);
    
    // Draw title bar
    RECT titleRect = { 0, 0, ps.rcPaint.right, TITLE_BAR_HEIGHT };
    HBRUSH hBrushTitle = CreateSolidBrush(CLR_TITLEBAR);
    FillRect(hdc, &titleRect, hBrushTitle);
    
    // Draw connection status
    SetTextColor(hdc, CLR_WHITE);
    SetBkMode(hdc, TRANSPARENT);
    std::wstring connStatus = utf8_to_wstring("Verbonden " + g_state.robotState);
    TextOutW(hdc, ps.rcPaint.right - 250, 8, connStatus.c_str(), (int)connStatus.size());
    
    // Draw header row
    RECT headerRect = { 0, TITLE_BAR_HEIGHT, ps.rcPaint.right, TITLE_BAR_HEIGHT + HEADER_HEIGHT };
    HBRUSH hBrushHeader = CreateSolidBrush(CLR_HEADER_BG);
    FillRect(hdc, &headerRect, hBrushHeader);
    
    // Column headers
    SetTextColor(hdc, CLR_BLACK);
    int x = PADDING;
    int y = TITLE_BAR_HEIGHT + 5;
    
    TextOutW(hdc, x, y, L"", 0);                              // Button col
    x += COL_BUTTON;
    TextOutW(hdc, x, y, L"Aantal", 6);                         // Quantity
    x += COL_QTY;
    TextOutW(hdc, x, y, L"Artikel", 7);                        // Article ID
    x += COL_ARTICLE_ID;
    TextOutW(hdc, x, y, L"Naam", 4);                           // Name
    x += COL_NAME;
    TextOutW(hdc, x, y, L"Eenheid", 7);                        // Unit
    x += COL_UNIT;
    TextOutW(hdc, x, y, L"Vorm", 4);                           // Form
    
    // Draw article rows
    {
        std::lock_guard<std::mutex> lock(g_state.mtx);
        int rowY = TITLE_BAR_HEIGHT + HEADER_HEIGHT;
        
        for (size_t i = 0; i < g_state.articles.size(); ++i)
        {
            if (rowY + ROW_HEIGHT > ps.rcPaint.bottom) break;
            
            const auto& art = g_state.articles[i];
            std::string id = art.first;
            int qty = art.second;
            
            // Find color
            COLORREF fillColor = (qty == 0) ? CLR_GREY : CLR_PURPLE;
            for (auto const& r : g_state.outputRecords)
            {
                if (std::get<1>(r) == id)
                {
                    fillColor = std::get<3>(r);
                    break;
                }
            }
            
            // Draw row background (button column with color)
            RECT rowRect = { 0, rowY, COL_BUTTON, rowY + ROW_HEIGHT };
            HBRUSH brush = CreateSolidBrush(fillColor);
            FillRect(hdc, &rowRect, brush);
            DeleteObject(brush);
            
            // Draw row data
            x = COL_BUTTON + PADDING;
            SetTextColor(hdc, CLR_BLACK);
            
            std::string qtyStr = std::to_string(qty);
            std::wstring wQty = utf8_to_wstring(qtyStr);
            TextOutW(hdc, x, rowY + 5, wQty.c_str(), (int)wQty.size());
            x += COL_QTY;
            
            std::wstring wId = utf8_to_wstring(id);
            TextOutW(hdc, x, rowY + 5, wId.c_str(), (int)wId.size());
            x += COL_ARTICLE_ID;
            
            // Find article name from database
            std::wstring wName = L"";
            x += COL_NAME;
            
            std::wstring wUnit = L"";
            x += COL_UNIT;
            
            std::wstring wForm = L"";
            TextOutW(hdc, x, rowY + 5, wForm.c_str(), (int)wForm.size());
            
            rowY += ROW_HEIGHT;
        }
    }
    
    EndPaint(hWnd, &ps);
    break;
}
```

### Step 4: Handle Button Clicks

```cpp
case WM_COMMAND:
{
    int control_id = LOWORD(wParam);
    
    switch (control_id)
    {
        case ID_SEARCH_BUTTON:
        {
            // Get search text from edit control
            HWND hEdit = GetDlgItem(hWnd, ID_SEARCH_EDIT);
            wchar_t searchText[256] = {0};
            GetWindowTextW(hEdit, searchText, 256);
            
            std::string search = std::string(searchText, searchText + wcslen(searchText));
            // TODO: Perform search
            InvalidateRect(hWnd, NULL, FALSE);
            break;
        }
        
        case ID_REFRESH_BUTTON:
        {
            // Send StockInfoRequest
            // TODO: async call to SendStockInfoRequest()
            break;
        }
    }
    break;
}
```

### Step 5: Handle Mouse Clicks on Article Rows

```cpp
case WM_LBUTTONDOWN:
{
    int xPos = GET_X_LPARAM(lParam);
    int yPos = GET_Y_LPARAM(lParam);
    
    // Check if click is in article row area
    if (yPos > TITLE_BAR_HEIGHT + HEADER_HEIGHT && yPos < ps.rcPaint.bottom - 40)
    {
        // Calculate which row was clicked
        int rowIndex = (yPos - TITLE_BAR_HEIGHT - HEADER_HEIGHT) / ROW_HEIGHT;
        
        {
            std::lock_guard<std::mutex> lock(g_state.mtx);
            if (rowIndex >= 0 && rowIndex < (int)g_state.articles.size())
            {
                // Get article ID and initiate output
                std::string articleId = g_state.articles[rowIndex].first;
                int articleQty = g_state.articles[rowIndex].second;
                
                if (articleQty > 0)
                {
                    // TODO: Send output request
                }
                
                g_state.selectedIndex = rowIndex;
                InvalidateRect(hWnd, NULL, FALSE);
            }
        }
    }
    break;
}
```

---

## Migration Checklist

### UI Elements
- [ ] Title bar with connection status
- [ ] Header row with column labels
- [ ] Article list with:
  - [ ] Color-coded buttons (state)
  - [ ] Article ID display
  - [ ] Name display
  - [ ] Quantity display
  - [ ] Packaging unit display
  - [ ] Form display
  - [ ] Click handling

### Controls
- [ ] Search text box
- [ ] Search button
- [ ] Refresh button

### Features
- [ ] Search functionality
- [ ] Article row selection
- [ ] Output request on button click
- [ ] Real-time status updates

---

## Key Differences: MAUI vs GDI32

| MAUI | GDI32 |
|------|-------|
| XAML markup | WM_PAINT drawing |
| Data binding | Manual text rendering |
| Click handlers | WM_LBUTTONDOWN |
| Styles/Colors | RGB constants |
| ScrollView | Paint clipping |
| Button control | Drawn rectangle + click detection |
| Entry control | CreateWindowW("EDIT") |

---

## Performance Tips

1. **Double buffering**: Use MemoryDC to avoid flicker
2. **Clipping**: Only paint visible rows
3. **Caching**: Store calculated column positions
4. **Threading**: Use locks when accessing g_state

---

**Next Steps:**
1. Add UI constants and window control creation
2. Implement complete WM_PAINT handler
3. Add mouse click detection
4. Connect to search functionality
5. Test with sample data

This will give you a fully functional UI matching the MAUI version!
