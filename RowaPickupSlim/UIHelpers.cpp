// UIHelpers.cpp
// UI utilities implementation

#include "UIHelpers.h"
#include <chrono>
#include <iomanip>
#include <ctime>
#include <cstdio>

namespace RowaPickupSlim::UIHelpers
{
    // ========================================================================
    // Color Constants (GDI COLORREF)
    // ========================================================================
    const COLORREF CLR_PURPLE = RGB(128, 0, 128);
    const COLORREF CLR_GREY = RGB(200, 200, 200);
    const COLORREF CLR_BLUE = RGB(0, 122, 204);              // Our OutputResponse
    const COLORREF CLR_YELLOW = RGB(255, 192, 0);           // Other client's OutputResponse
    const COLORREF CLR_RED = RGB(220, 0, 0);
    const COLORREF CLR_ORANGE = RGB(255, 140, 0);
    const COLORREF CLR_GREEN = RGB(34, 139, 34);
    const COLORREF CLR_TITLEBAR_BG = RGB(51, 51, 51);       // #333333
    const COLORREF CLR_HEADER_BG = RGB(238, 238, 238);      // #EEE
    const COLORREF CLR_TEXT = RGB(0, 0, 0);
    const COLORREF CLR_WHITE = RGB(255, 255, 255);
    const COLORREF CLR_BLACK = RGB(0, 0, 0);

    // ========================================================================
    // Window UI Dimensions
    // ========================================================================
    const int TITLE_BAR_HEIGHT = 32;
    const int HEADER_HEIGHT = 25;
    const int ROW_HEIGHT = 28;
    const int PADDING = 3;
    const int SEARCH_AREA_HEIGHT = 35;
    const int STATUS_BAR_HEIGHT = 20;

    // ========================================================================
    // Column Widths
    // ========================================================================
    const int COL_BUTTON = 75;
    const int COL_QTY = 75;
    const int COL_ARTICLE_ID = 250;
    const int TOTAL_COL_WIDTH = COL_BUTTON + COL_QTY + COL_ARTICLE_ID;

    // ========================================================================
    // Scrollbar Configuration
    // ========================================================================
    const int SCROLLBAR_WIDTH = 15;
    const int SCROLLBAR_THUMB_MIN_HEIGHT = 20;

    // ========================================================================
    // Custom Windows Messages (defined as macros in header for use in switch statements)
    // ========================================================================
    // Now defined as #define in UIHelpers.h

    // ========================================================================
    // Utility Functions
    // ========================================================================

    std::wstring Utf8ToWstring(const std::string& utf8)
    {
        if (utf8.empty()) return {};
        int needed = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), (int)utf8.size(), NULL, 0);
        std::wstring w;
        w.resize(needed);
        MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), (int)utf8.size(), &w[0], needed);
        return w;
    }

    std::string MakeUniqueId()
    {
        using namespace std::chrono;
        auto now = system_clock::now();
        auto t = system_clock::to_time_t(now);
        auto ms = duration_cast<milliseconds>(now.time_since_epoch()).count() % 1000;
        std::tm tm{};
        localtime_s(&tm, &t);
        char buf[64];
        snprintf(buf, sizeof(buf), "%02d%02d%02d%03lld",
                 tm.tm_hour, tm.tm_min, tm.tm_sec, static_cast<long long>(ms));
        return std::string(buf);
    }

} // namespace RowaPickupSlim::UIHelpers
