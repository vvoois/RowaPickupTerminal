#pragma once
// UIHelpers.h
// UI utility functions and constants - REUSABLE in other projects
// Only depends on: windows.h, string

#include <windows.h>
#include <string>

namespace RowaPickupSlim::UIHelpers
{
    // ========================================================================
    // UI Colors (GDI COLORREF) - Portable color definitions
    // ========================================================================
    extern const COLORREF CLR_PURPLE;           // Default/idle state
    extern const COLORREF CLR_GREY;             // Disabled/inactive
    extern const COLORREF CLR_BLUE;             // Our OutputResponse
    extern const COLORREF CLR_YELLOW;           // Other client's OutputResponse
    extern const COLORREF CLR_RED;              // Rejected/Incomplete status
    extern const COLORREF CLR_ORANGE;           // InProcess status
    extern const COLORREF CLR_GREEN;            // Completed status
    extern const COLORREF CLR_TITLEBAR_BG;      // Title bar background (#333333)
    extern const COLORREF CLR_HEADER_BG;        // Header background (#EEE)
    extern const COLORREF CLR_TEXT;             // Default text color
    extern const COLORREF CLR_WHITE;            // White
    extern const COLORREF CLR_BLACK;            // Black

    // ========================================================================
    // Window UI Dimensions - Standard layout measurements
    // ========================================================================
    extern const int TITLE_BAR_HEIGHT;
    extern const int HEADER_HEIGHT;
    extern const int ROW_HEIGHT;
    extern const int PADDING;
    extern const int SEARCH_AREA_HEIGHT;
    extern const int STATUS_BAR_HEIGHT;

    // ========================================================================
    // Column Widths - UI layout columns
    // ========================================================================
    extern const int COL_BUTTON;
    extern const int COL_QTY;
    extern const int COL_ARTICLE_ID;
    extern const int TOTAL_COL_WIDTH;

    // ========================================================================
    // Scrollbar Configuration
    // ========================================================================
    extern const int SCROLLBAR_WIDTH;
    extern const int SCROLLBAR_THUMB_MIN_HEIGHT;

    // ========================================================================
    // Custom Windows Messages (for WM_APP-based message IDs)
    // Use #define for compile-time constants in switch statements
    // ========================================================================
    #define WM_APP_NETWORK_UPDATE (WM_APP + 1)
    #define WM_APP_LANGUAGE_CHANGED (WM_APP + 2)

    // ========================================================================
    // Portable Utility Functions - No app-specific dependencies
    // ========================================================================

    /// Convert UTF-8 string to wide string (UTF-16) for Windows API calls
    /// Reusable: Yes - standard string conversion
    std::wstring Utf8ToWstring(const std::string& utf8);

    /// Generate a unique ID in HHmmssfff format
    /// Reusable: Yes - standard ID generation
    std::string MakeUniqueId();

} // namespace RowaPickupSlim::UIHelpers

