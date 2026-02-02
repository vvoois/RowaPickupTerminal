// main.cpp
// Win32 app with GDI UI mapped from MainPage.xaml.cs behavior.
// - Mouse/keyboard selection of articles
// - Send OutputRequest on Enter / click-confirm
// - Visual colors for article rows based on output state
// - Parses incoming WWKS messages (pugixml) and updates UI/state
//
// Requires: networkclient.h, pugixml.hpp, networkclient.cpp compiled into project.

// ============================================================================
// SYSTEM HEADERS - winsock2.h MUST come BEFORE windows.h
// ============================================================================
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#pragma comment(lib, "comctl32.lib")

#include "resource.h"
#include <string>
#include <vector>
#include <tuple>
#include <set>
#include <mutex>
#include <memory>
#include <thread>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <algorithm>

// ============================================================================
// APPLICATION HEADERS
// ============================================================================
#include "pugixml.hpp"
#include "networkclient.h"
#include "SharedVariables.h"
#include "SettingsDialog.h"
#include "Localization.h"
#include "UIHelpers.h"
#include "LoggingSystem.h"

// ============================================================================
// NAMESPACE USAGE
// ============================================================================
using namespace RowaPickupSlim;
// WM_APP_NETWORK_UPDATE and WM_APP_LANGUAGE_CHANGED are #define macros in UIHelpers.h

// ============================================================================
// GLOBAL STATE - Simple local definition
// ============================================================================
struct AppState
{
    std::string connectionState = "Not connected";
    std::string robotState = "unknown";
    std::string lastMessageType;
    
    std::vector<std::pair<std::string,int>> baseArticlesList;
    std::vector<std::pair<std::string,int>> articles;
    std::vector<std::pair<std::string,int>> fullArticlesList;
    
    // Output records: (orderId, articleId, quantityRequested, packsDelivered, color, isOurOutput)
    std::vector<std::tuple<std::string,std::string,int,int,COLORREF,bool>> outputRecords;
    std::set<std::string> ourOutputRequestIds;
    
    // Device status
    std::vector<std::tuple<std::string,std::string,std::string,std::string>> devices;
    
    // UI state
    int selectedIndex = -1;
    int scrollOffset = 0;
    
    // Synchronization
    std::mutex mtx;
} g_state;

std::unique_ptr<NetworkClient> g_client;
std::string g_logFilePath;

// ============================================================================
// Logging System Implementation
// ============================================================================

// Initialize log file with timestamp
static void InitializeLogging()
{
    LoggingSystem::Initialize();
    g_logFilePath = LoggingSystem::GetLogFilePath();
}

// Write message to log file
static void LogMessage(const std::string& message)
{
    LoggingSystem::LogMessage(message);
}

// Clean up log files older than 31 days
static void CleanupOldLogFiles()
{
    LoggingSystem::CleanupOldLogFiles();
}

// App-specific simple shared settings - now loaded from SharedVariables
// (Load via SettingsLoader::LoadSettings() before creating main window)

// ============================================================================
// UI Colors, Dimensions, and Control IDs - delegated to UIHelpers module
// ============================================================================
// Import color constants from UIHelpers
using UIHelpers::CLR_PURPLE;
using UIHelpers::CLR_GREY;
using UIHelpers::CLR_BLUE;
using UIHelpers::CLR_YELLOW;
using UIHelpers::CLR_RED;
using UIHelpers::CLR_ORANGE;
using UIHelpers::CLR_GREEN;
using UIHelpers::CLR_TITLEBAR_BG;
using UIHelpers::CLR_HEADER_BG;
using UIHelpers::CLR_TEXT;
using UIHelpers::CLR_WHITE;
using UIHelpers::CLR_BLACK;

// Import UI dimensions from UIHelpers
using UIHelpers::TITLE_BAR_HEIGHT;
using UIHelpers::HEADER_HEIGHT;
using UIHelpers::ROW_HEIGHT;
using UIHelpers::PADDING;
using UIHelpers::SEARCH_AREA_HEIGHT;
using UIHelpers::STATUS_BAR_HEIGHT;

// Import column widths from UIHelpers
using UIHelpers::COL_BUTTON;
using UIHelpers::COL_QTY;
using UIHelpers::COL_ARTICLE_ID;
using UIHelpers::TOTAL_COL_WIDTH;

// Import scrollbar dimensions from UIHelpers
using UIHelpers::SCROLLBAR_WIDTH;
using UIHelpers::SCROLLBAR_THUMB_MIN_HEIGHT;

// ============================================================================
// Control IDs
// ============================================================================
#define ID_SEARCH_EDIT 1001
#define ID_REFRESH_BUTTON 1003
#define ID_SEARCH_DEBOUNCE_TIMER 9001

struct ScrollbarState
{
    bool isDragging = false;      // Is user currently dragging the thumb?
    int thumbStartY = 0;          // Y position when drag started
    int scrollStartOffset = 0;    // Scroll offset when drag started
} g_scrollbarState;

// Utility: convert UTF-8 to wide string for TextOutW - delegated to UIHelpers
static std::wstring utf8_to_wstring(const std::string& s)
{
    return UIHelpers::Utf8ToWstring(s);
}

// Utility: format id like HHmmssfff - delegated to UIHelpers
static std::string make_unique_id()
{
    return UIHelpers::MakeUniqueId();
}

// Helper to find index of article by id in g_state.articles
static int find_article_index(const std::string& articleId)
{
    for (size_t i = 0; i < g_state.articles.size(); ++i)
    {
        if (g_state.articles[i].first == articleId)
            return (int)i;
    }
    return -1;
}

// Helper to find and update article quantity in both articles and fullArticlesList
// This keeps both lists in sync so filtering doesn't lose quantity changes
static void update_article_quantity_in_both_lists(const std::string& articleId, int newQty)
{
    // Update in filtered articles list
    for (size_t i = 0; i < g_state.articles.size(); ++i)
    {
        if (g_state.articles[i].first == articleId)
        {
            g_state.articles[i].second = (newQty < 0) ? 0 : newQty;
            break;
        }
    }
    
    // Also update in full articles list to keep both in sync
    for (size_t i = 0; i < g_state.fullArticlesList.size(); ++i)
    {
        if (g_state.fullArticlesList[i].first == articleId)
        {
            g_state.fullArticlesList[i].second = (newQty < 0) ? 0 : newQty;
            break;
        }
    }
}

// Update outputRecords based on order id and set color according to status and ownership.
// If Completed, remove record and adjust article quantity if possible.
static void update_output_record_from_message(const std::string& orderId, const std::string& articleId, int quantityRequested, int packsDelivered, const std::string& status, bool isOurOutput)
{
    std::lock_guard<std::mutex> lock(g_state.mtx);
    auto it = std::find_if(g_state.outputRecords.begin(), g_state.outputRecords.end(),
        [&](auto const& t){ return std::get<0>(t) == orderId; });

    COLORREF color = CLR_PURPLE;
    if (status == "Queued")
    {
        color = isOurOutput ? CLR_BLUE : CLR_YELLOW;
    }
    else if (status == "Rejected" || status == "Incomplete")
    {
        color = CLR_RED;
    }
    else if (status == "InProcess")
    {
        color = CLR_ORANGE;
    }
    else if (status == "Completed")
    {
        color = CLR_GREEN;
    }

    // ALWAYS adjust article quantity for Completed status (whether we have a record or not)
    // Update both articles and fullArticlesList to keep them in sync for filtering
    if (status == "Completed" && !articleId.empty())
    {
        int idx = find_article_index(articleId);
        if (idx >= 0)
        {
            int newQty = g_state.articles[idx].second - quantityRequested;
            update_article_quantity_in_both_lists(articleId, newQty);
        }
    }

    if (it != g_state.outputRecords.end())
    {
        if (status == "Completed")
        {
            g_state.outputRecords.erase(it);
        }
        else
        {
            // update color / qty / packs delivered if changed
            std::get<2>(*it) = quantityRequested;
            std::get<3>(*it) = packsDelivered;
            std::get<4>(*it) = color;
            std::get<5>(*it) = isOurOutput;
        }
    }
    else
    {
        if (status != "Completed")
        {
            g_state.outputRecords.emplace_back(orderId, articleId, quantityRequested, packsDelivered, color, isOurOutput);
        }
    }
}

// Parse incoming WWKS XML, update g_state and post UI update
static void handle_incoming_xml_and_update_state(const std::string& xml, HWND hwnd)
{
    pugi::xml_document doc;
    pugi::xml_parse_result res = doc.load_string(xml.c_str());
    if (!res) return;

    pugi::xml_node root = doc.child("WWKS");
    if (!root) return;

    std::string messageType;
    if (root.child("HelloResponse")) messageType = "HelloResponse";
    else if (root.child("StatusResponse")) messageType = "StatusResponse";
    else if (root.child("StockInfoResponse")) messageType = "StockInfoResponse";
    else if (root.child("OutputResponse")) messageType = "OutputResponse";
    else if (root.child("OutputMessage")) messageType = "OutputMessage";
    else if (root.child("TaskInfoResponse")) messageType = "TaskInfoResponse";
    else if (root.child("InputMessage")) messageType = "InputMessage";

    // Debug logging
    {
        char debugMsg[256];
        snprintf(debugMsg, sizeof(debugMsg), "=== Received: %s ===", messageType.empty() ? "UNKNOWN" : messageType.c_str());
        LogMessage(debugMsg);
        if (messageType.empty())
        {
            LogMessage("Raw XML:");
            LogMessage(xml);
        }
    }

    {
        std::lock_guard<std::mutex> lock(g_state.mtx);
        g_state.lastMessageType = messageType;
    }

    if (messageType == "StatusResponse")
    {
        pugi::xml_node sr = root.child("StatusResponse");
        if (!sr) sr = root.find_node([](pugi::xml_node n){ return std::string(n.name()) == "StatusResponse"; });
        
        if (sr)
        {
            // New format: State is directly on StatusResponse element
            std::string state = sr.attribute("State") ? sr.attribute("State").value() : "";
            
            if (!state.empty())
            {
                // Convert state to Dutch display
                std::string stateDisplay = (state == "Ready" || state == "ready") ? "Gereed" : "Inactief";
                std::string display = "(Mosaic [" + stateDisplay + "])";
                
                // Extract all peripheral devices
                std::vector<std::tuple<std::string,std::string,std::string,std::string>> deviceList;
                for (pugi::xml_node comp : sr.children("Component"))
                {
                    std::string compType = comp.attribute("Type") ? comp.attribute("Type").value() : "";
                    std::string compDesc = comp.attribute("Description") ? comp.attribute("Description").value() : "";
                    std::string compState = comp.attribute("State") ? comp.attribute("State").value() : "";
                    std::string compStateText = comp.attribute("StateText") ? comp.attribute("StateText").value() : "";
                    
                    if (!compDesc.empty())
                    {
                        deviceList.emplace_back(compType, compDesc, compState, compStateText);
                    }
                }
                
                std::lock_guard<std::mutex> lock(g_state.mtx);
                g_state.robotState = display;
                g_state.devices = deviceList;
                
                char debugMsg[256];
                snprintf(debugMsg, sizeof(debugMsg), "StatusResponse: State=%s, Devices=%zu", state.c_str(), deviceList.size());
                LogMessage(debugMsg);
                
                // Log each device
                for (const auto& dev : deviceList)
                {
                    snprintf(debugMsg, sizeof(debugMsg), "  Device: %s - %s (%s)", 
                        std::get<0>(dev).c_str(), std::get<1>(dev).c_str(), std::get<3>(dev).c_str());
                    LogMessage(debugMsg);
                }
            }
            else
            {
                // Fallback: look for old format with Component children
                std::vector<std::tuple<std::string,std::string,std::string,std::string>> deviceList;
                for (pugi::xml_node comp : sr.children("Component"))
                {
                    if (std::string(comp.attribute("Type").value()) == "StorageSystem")
                    {
                        std::string desc = comp.attribute("Description") ? comp.attribute("Description").value() : "";
                        std::string compState = comp.attribute("State") ? comp.attribute("State").value() : "";
                        std::string stateText = comp.attribute("StateText") ? comp.attribute("StateText").value() : "";
                        
                        // extract last token as robot number
                        std::string robotNumber = "0";
                        size_t p = desc.find_last_of(' ');
                        if (p != std::string::npos && p+1 < desc.size()) robotNumber = desc.substr(p+1);
                        
                        std::string display = "(ROB" + robotNumber + " [" + (compState == "Ready" ? "Gereed" : "Inactief") + "])";
                        
                        std::lock_guard<std::mutex> lock(g_state.mtx);
                        g_state.robotState = display;
                        
                        deviceList.emplace_back("StorageSystem", desc, compState, stateText);
                        break;
                    }
                }
                
                if (!deviceList.empty())
                {
                    std::lock_guard<std::mutex> lock(g_state.mtx);
                    g_state.devices = deviceList;
                }
            }
        }
    }

    if (messageType == "StockInfoResponse")
    {
        pugi::xml_node sir = root.child("StockInfoResponse");
        if (!sir) sir = root.find_node([](pugi::xml_node n){ return std::string(n.name()) == "StockInfoResponse"; });
        
        std::vector<std::pair<std::string,int>> list;
        if (sir)
        {
            for (pugi::xml_node a : sir.children("Article"))
            {
                std::string id = a.attribute("Id") ? a.attribute("Id").value() : "";
                int qty = a.attribute("Quantity") ? a.attribute("Quantity").as_int() : 0;
                
                // Filter: only keep articles starting with "RoWa" (case-insensitive)
                if (!id.empty() && id.size() >= 4)
                {
                    std::string prefix = id.substr(0, 4);
                    // Convert to uppercase for comparison
                    std::transform(prefix.begin(), prefix.end(), prefix.begin(), ::toupper);
                    
                    if (prefix == "ROWA")
                    {
                        list.emplace_back(id, qty);
                    }
                }
                
                if (list.size() >= 200) break;
            }
        }
        {
            std::lock_guard<std::mutex> lock(g_state.mtx);
            g_state.articles = std::move(list);
            g_state.fullArticlesList = g_state.articles;  // Keep a backup of the full list
            
            // reset selection if out of range
            if (g_state.selectedIndex >= (int)g_state.articles.size()) g_state.selectedIndex = (int)g_state.articles.size() - 1;
        }
    }

    // Handle OutputMessage / OutputResponse / TaskInfoResponse updates
    if (messageType == "OutputMessage" || messageType == "OutputResponse" || messageType == "TaskInfoResponse")
    {
        // Try to find order id and article id and status
        std::string orderId;
        std::string articleId;
        int quantityRequested = 1;
        int packsDelivered = 0;
        std::string status;

        // OutputMessage - can have multiple articles, each with multiple packs
        pugi::xml_node om = root.child("OutputMessage");
        if (om)
        {
            orderId = om.attribute("Id") ? om.attribute("Id").value() : "";
            pugi::xml_node det = om.child("Details");
            if (det) status = det.attribute("Status") ? det.attribute("Status").value() : "";
            
            // Determine ownership
            bool isOurOutput = false;
            {
                std::lock_guard<std::mutex> lock(g_state.mtx);
                isOurOutput = (g_state.ourOutputRequestIds.count(orderId) > 0);
            }
            
            bool hasArticles = false;
            // Process all articles in this OutputMessage
            for (pugi::xml_node art : om.children("Article"))
            {
                hasArticles = true;
                articleId = art.attribute("Id") ? art.attribute("Id").value() : "";
                
                // Count the number of Pack elements - each pack is one picked item
                int packCount = 0;
                for (pugi::xml_node pack : art.children("Pack"))
                {
                    packCount++;
                }
                packsDelivered = packCount;
                quantityRequested = packCount;  // For completed, this becomes the delivered amount
                
                // Debug logging
                char debugMsg[256];
                snprintf(debugMsg, sizeof(debugMsg), "OutputMessage: ArticleId=%s, Packs=%d, Status=%s, IsOur=%s", 
                    articleId.c_str(), packCount, status.c_str(), isOurOutput ? "true" : "false");
                LogMessage(debugMsg);
                
                update_output_record_from_message(orderId, articleId, quantityRequested, packsDelivered, status, isOurOutput);
            }
            
            // If no articles found but status is present, look up the original article from the order
            if (!hasArticles && !status.empty())
            {
                {
                    std::lock_guard<std::mutex> lock(g_state.mtx);
                    auto it = std::find_if(g_state.outputRecords.begin(), g_state.outputRecords.end(),
                        [&](const auto& t){ return std::get<0>(t) == orderId; });
                    
                    if (it != g_state.outputRecords.end())
                    {
                        articleId = std::get<1>(*it);
                        quantityRequested = std::get<2>(*it);  // Original requested quantity
                        packsDelivered = 0;  // No packs were delivered
                        isOurOutput = std::get<5>(*it);  // Get ownership from existing record
                        
                        char debugMsg[256];
                        snprintf(debugMsg, sizeof(debugMsg), "OutputMessage (empty): ArticleId=%s, Requested=%d, Delivered=0, Status=%s, IsOur=%s",
                            articleId.c_str(), quantityRequested, status.c_str(), isOurOutput ? "true" : "false");
                        LogMessage(debugMsg);
                    }
                }  // Lock released here
                
                update_output_record_from_message(orderId, articleId, quantityRequested, packsDelivered, status, isOurOutput);
            }
        }

        // OutputResponse (sometimes wraps different structure)
        pugi::xml_node orr = root.child("OutputResponse");
        if (orr)
        {
            orderId = orr.attribute("Id") ? orr.attribute("Id").value() : "";
            int quantityReq = 1;
            int packsDel = 0;
            
            pugi::xml_node crit = orr.child("Criteria");
            if (crit) {
                articleId = crit.attribute("ArticleId") ? crit.attribute("ArticleId").value() : "";
                quantityReq = crit.attribute("Quantity") ? crit.attribute("Quantity").as_int() : 1;
            }
            pugi::xml_node det = orr.child("Details");
            if (det) status = det.attribute("Status") ? det.attribute("Status").value() : "";
            
            // Determine ownership: check if this OutputRequest ID is in our sent requests
            bool isOurOutput = false;
            {
                std::lock_guard<std::mutex> lock(g_state.mtx);
                isOurOutput = (g_state.ourOutputRequestIds.count(orderId) > 0);
            }
            
            update_output_record_from_message(orderId, articleId, quantityReq, packsDel, status, isOurOutput);
        }

        // TaskInfoResponse
        pugi::xml_node tir = root.child("TaskInfoResponse");
        if (tir)
        {
            pugi::xml_node task = tir.child("Task");
            if (task)
            {
                orderId = tir.attribute("Id") ? tir.attribute("Id").value() : "";
                if (orderId.empty()) orderId = task.attribute("Id") ? task.attribute("Id").value() : orderId;
                status = task.attribute("Status") ? task.attribute("Status").value() : "";
                // Articles under <Task>
                pugi::xml_node art = task.child("Article");
                if (art) articleId = art.attribute("Id") ? art.attribute("Id").value() : "";
                
                // Determine ownership
                bool isOurOutput = false;
                {
                    std::lock_guard<std::mutex> lock(g_state.mtx);
                    isOurOutput = (g_state.ourOutputRequestIds.count(orderId) > 0);
                }
                
                update_output_record_from_message(orderId, articleId, 1, 0, status, isOurOutput);
            }
        }
    }

    // Update UI
    PostMessage(hwnd, WM_APP_NETWORK_UPDATE, 0, 0);
}

// Send OutputRequest for an articleId with quantity
static void send_output_request_for_article(const std::string& articleId, int qty)
{
    if (!g_client) return;
    std::string id = make_unique_id();
    std::ostringstream ss;
    ss << "<WWKS Version=\"2.0\" TimeStamp=\"";
    // timestamp in UTC-like simple format
    {
        auto now = std::chrono::system_clock::now();
        std::time_t t = std::chrono::system_clock::to_time_t(now);
        std::tm tm{};
        gmtime_s(&tm, &t);
        char buf[64];
        strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tm);
        ss << buf;
    }
    ss << "\">";
    ss << "<OutputRequest Id=\"" << id << "\" Source=\"" << SharedVariables::SourceNumber << "\" Destination=\"999\">";
    ss << "<Details OutputDestination=\"" << SharedVariables::OutputNumber << "\" Priority=\"" << SharedVariables::SelectedPrioItemText << "\" />";
    ss << "<Criteria ArticleId=\"" << articleId << "\" Quantity=\"" << qty << "\" />";
    ss << "</OutputRequest>";
    ss << "</WWKS>";

    // Track this OutputRequest ID as ours for later ownership detection
    {
        std::lock_guard<std::mutex> lock(g_state.mtx);
        g_state.ourOutputRequestIds.insert(id);
        // Add local output record as Queued(Blue) with 0 packs delivered initially, mark as ours
        g_state.outputRecords.emplace_back(id, articleId, qty, 0, CLR_BLUE, true);
    }

    // send via network client (append newline like WriteLine)
    g_client->SendMessage(ss.str());
    // notify UI update
    HWND hwnd = FindWindowW(L"RowaPickupMainWindowClass", NULL);
    if (hwnd) PostMessage(hwnd, WM_APP_NETWORK_UPDATE, 0, 0);
}

// Send OutputRequest for all articles with their quantities
static void send_output_request_for_all_articles()
{
    if (!g_client) return;
    
    std::vector<std::pair<std::string,int>> articlesToSend;
    {
        std::lock_guard<std::mutex> lock(g_state.mtx);
        articlesToSend = g_state.articles;
    }
    
    int count = 0;
    for (const auto& art : articlesToSend)
    {
        if (art.second > 0)  // Only send if quantity > 0
        {
            send_output_request_for_article(art.first, art.second);
            count++;
        }
    }
    
    char debugMsg[256];
    snprintf(debugMsg, sizeof(debugMsg), "Sent OutputRequest for %d articles", count);
    LogMessage(debugMsg);
}

// Global keyboard hook handle for capturing ESC in MessageBox
static HHOOK g_keyboardHook = NULL;

// Low-level keyboard hook to intercept ESC in MessageBox dialogs
LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode == HC_ACTION && wParam == WM_KEYDOWN)
    {
        KBDLLHOOKSTRUCT* pKB = (KBDLLHOOKSTRUCT*)lParam;
        
        // ESC key (VK_ESCAPE = 27)
        if (pKB->vkCode == VK_ESCAPE)
        {
            HWND hWnd = GetForegroundWindow();
            if (hWnd)
            {
                // Get the window class name
                wchar_t className[256] = {0};
                GetClassNameW(hWnd, className, sizeof(className) / sizeof(wchar_t));
                
                // MessageBox uses dialog class "#32770"
                if (wcscmp(className, L"#32770") == 0)
                {
                    // Send IDNO command to close the MessageBox with "No" response
                    SendMessageW(hWnd, WM_COMMAND, IDNO, 0);
                    return 1; // Consume the keystroke so it doesn't propagate
                }
            }
        }
    }
    
    return CallNextHookEx(g_keyboardHook, nCode, wParam, lParam);
}

// Propose output for an article - shows confirmation dialog and sends if approved
// Returns true if user clicked Yes, false otherwise (including ESC)
static bool show_output_confirmation_dialog(HWND hWnd, const std::string& articleId, int qty)
{
    // Build localized message
    std::wstring confirmMsg = Localization::GetString(RowaPickupSlim::STR_CONFIRM_OUTPUT);
    // Replace {0} with article ID and {1} with quantity
    size_t pos1 = confirmMsg.find(L"{0}");
    if (pos1 != std::wstring::npos)
        confirmMsg.replace(pos1, 3, utf8_to_wstring(articleId));
    size_t pos2 = confirmMsg.find(L"{1}");
    if (pos2 != std::wstring::npos)
        confirmMsg.replace(pos2, 3, utf8_to_wstring(std::to_string(qty)));
    
    // Install low-level keyboard hook before showing MessageBox
    if (!g_keyboardHook)
    {
        g_keyboardHook = SetWindowsHookExW(WH_KEYBOARD_LL, LowLevelKeyboardProc, NULL, 0);
    }
    
    // Show MessageBox with localized title and message
    int resp = MessageBoxW(hWnd, confirmMsg.c_str(), 
                          Localization::GetString(RowaPickupSlim::STR_CONFIRM_OUTPUT_TITLE).c_str(), 
                          MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON1);
    
    // Remove hook after MessageBox closes
    if (g_keyboardHook)
    {
        UnhookWindowsHookEx(g_keyboardHook);
        g_keyboardHook = NULL;
    }
    
    // ESC returns IDNO (the "No" button) for MB_YESNO dialogs
    return (resp == IDYES);
}

// Propose output for an article - shows confirmation dialog and sends if approved
static void propose_output_for_article(HWND hWnd, const std::string& articleId, int qty)
{
    if (articleId.empty() || qty <= 0) return;
    
    if (show_output_confirmation_dialog(hWnd, articleId, qty))
    {
        std::thread([articleId, qty](){
            send_output_request_for_article(articleId, qty);
        }).detach();
    }
}

// Perform debounced search - called from timer
static void perform_search_filter(HWND hWnd)
{
    // Get search text from edit control
    HWND hEdit = GetDlgItem(hWnd, ID_SEARCH_EDIT);
    if (!hEdit) return;
    
    wchar_t searchText[256] = {0};
    GetWindowTextW(hEdit, searchText, 256);
    
    // Convert search text to string
    char searchBuf[256];
    WideCharToMultiByte(CP_UTF8, 0, searchText, -1, searchBuf, sizeof(searchBuf), NULL, NULL);
    std::string searchStr = searchBuf;
    
    // Convert search string to uppercase for case-insensitive search
    std::transform(searchStr.begin(), searchStr.end(), searchStr.begin(), ::toupper);
    
    LogMessage("Search (debounced)");
    char debugMsg[256];
    snprintf(debugMsg, sizeof(debugMsg), "  Search text: %s", searchStr.c_str());
    LogMessage(debugMsg);

    // Check if scan output mode is enabled and search string is a complete article code
    bool scanOutputEnabled = SharedVariables::ScanOutput;
    std::string completeArticleCode;
    std::string matchedArticleId;
    int matchedArticleQty = 0;
    bool foundArticle = false;
    
    if (scanOutputEnabled && !searchStr.empty())
    {
        // Check if input is a complete article code (either "RoWa00020556" or "00020556")
        bool isCompleteCode = false;
        
        if (searchStr.length() >= 11 && searchStr.substr(0, 4) == "ROWA")
        {
            // Full code like "RoWa00020556"
            completeArticleCode = searchStr;
            isCompleteCode = true;
        }
        else if (searchStr.length() == 8 && std::all_of(searchStr.begin(), searchStr.end(), ::isdigit))
        {
            // Just the unique part like "00020556"
            completeArticleCode = "ROWA" + searchStr;
            isCompleteCode = true;
        }
        
        // If we have a complete code, try to find and output it
        if (isCompleteCode)
        {
            {
                std::lock_guard<std::mutex> lock(g_state.mtx);
                
                // Search for exact article match
                for (const auto& art : g_state.articles)
                {
                    std::string upperArticleId = art.first;
                    std::transform(upperArticleId.begin(), upperArticleId.end(), upperArticleId.begin(), ::toupper);
                    
                    if (upperArticleId == completeArticleCode && art.second > 0)
                    {
                        foundArticle = true;
                        matchedArticleId = art.first;
                        matchedArticleQty = art.second;
                        break;
                    }
                }
            }  // Lock released here
            
            if (foundArticle)
            {
                // Found matching article with quantity > 0
                snprintf(debugMsg, sizeof(debugMsg), "  Scan Output detected: %s (qty=%d), sending output request", matchedArticleId.c_str(), matchedArticleQty);
                LogMessage(debugMsg);
                
                // Send output request
                //propose_output_for_article(hWnd, matchedArticleId, matchedArticleQty);

                send_output_request_for_article(matchedArticleId, matchedArticleQty);
                
                // Select all text in search field (don't clear it) so user/scanner can instantly replace it
                // EM_SETSEL: first param = start pos (0), second param = end pos (-1 means to end)
                SendMessageW(hEdit, EM_SETSEL, 0, -1);
                SetFocus(hEdit);  // Ensure focus is on edit control for next input
                
                // Keep current filter state and scroll position so user can follow output progress
                // (don't reset articles list or scroll offset)
                
                // Trigger UI update
                PostMessage(hWnd, WM_APP_NETWORK_UPDATE, 0, 0);
                return;
            }
            else
            {
                // Article not found or qty is 0
                snprintf(debugMsg, sizeof(debugMsg), "  Scan Output: Article %s not found or qty=0", completeArticleCode.c_str());
                LogMessage(debugMsg);
                return;  // Don't proceed with normal filter if scan mode is active
            }
        }
    }

    // Normal search filter (when not in scan mode or code is incomplete)
    {
        std::lock_guard<std::mutex> lock(g_state.mtx);
        
        // Reset selection
        g_state.selectedIndex = -1;
        g_state.scrollOffset = 0;
        
        // Always filter from the full list
        std::vector<std::pair<std::string,int>> filtered;
        
        if (searchStr.empty())
        {
            // Empty search - show all articles from full list
            filtered = g_state.fullArticlesList;
        }
        else
        {
            // Filter from full list
            for (const auto& art : g_state.fullArticlesList)
            {
                std::string upperArticle = art.first;
                std::transform(upperArticle.begin(), upperArticle.end(), upperArticle.begin(), ::toupper);
                
                // Check if contains search text
                if (upperArticle.find(searchStr) != std::string::npos)
                {
                    filtered.emplace_back(art);
                }
            }
        }
        
        g_state.articles = filtered;
    }
    
    // Trigger UI update
    PostMessage(hWnd, WM_APP_NETWORK_UPDATE, 0, 0);
}

// Handle Tab/Shift+Tab navigation between controls (List, Search, Refresh button)
static void handle_tab_navigation(HWND hWnd, bool shiftPressed)
{
    HWND hSearchEdit = GetDlgItem(hWnd, ID_SEARCH_EDIT);
    HWND hRefreshBtn = GetDlgItem(hWnd, ID_REFRESH_BUTTON);
    HWND focused = GetFocus();
    
    // Determine focus order: List (main window) -> Search Edit -> Refresh Button -> List
    if (shiftPressed)
    {
        // Shift+Tab: go backward
        if (focused == hRefreshBtn)
        {
            // Refresh -> Search
            SetFocus(hSearchEdit);
        }
        else if (focused == hSearchEdit)
        {
            // Search -> List (set focus to main window, which will select first article)
            SetFocus(hWnd);
            std::lock_guard<std::mutex> lock(g_state.mtx);
            if (g_state.articles.size() > 0 && g_state.selectedIndex < 0)
            {
                g_state.selectedIndex = 0;
            }
            InvalidateRect(hWnd, NULL, TRUE);
        }
        else
        {
            // List -> Refresh
            SetFocus(hRefreshBtn);
        }
    }
    else
    {
        // Tab: go forward
        if (focused == hSearchEdit)
        {
            // Search -> Refresh
            SetFocus(hRefreshBtn);
        }
        else if (focused == hRefreshBtn)
        {
            // Refresh -> List (set focus to main window)
            SetFocus(hWnd);
            std::lock_guard<std::mutex> lock(g_state.mtx);
            if (g_state.articles.size() > 0 && g_state.selectedIndex < 0)
            {
                g_state.selectedIndex = 0;
            }
            InvalidateRect(hWnd, NULL, TRUE);
        }
        else
        {
            // List -> Search
            SetFocus(hSearchEdit);
        }
    }
}

// ============================================================================
// Custom Tooltip Implementation
// ============================================================================

// Global tooltip window handle
static HWND g_hTooltipWindow = NULL;
static std::wstring g_tooltipText;

// Custom tooltip window class
LRESULT CALLBACK TooltipWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        
        // Draw background
        RECT rc;
        GetClientRect(hwnd, &rc);
        HBRUSH hBrush = CreateSolidBrush(RGB(255, 255, 200));  // Light yellow
        FillRect(hdc, &rc, hBrush);
        DeleteObject(hBrush);
        
        // Draw border
        HPEN hPen = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
        HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
        MoveToEx(hdc, 0, 0, NULL);
        LineTo(hdc, rc.right - 1, 0);
        LineTo(hdc, rc.right - 1, rc.bottom - 1);
        LineTo(hdc, 0, rc.bottom - 1);
        LineTo(hdc, 0, 0);
        SelectObject(hdc, hOldPen);
        DeleteObject(hPen);
        
        // Draw text
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(0, 0, 0));
        
        RECT textRect = rc;
        textRect.left += 5;
        textRect.top += 3;
        textRect.right -= 5;
        textRect.bottom -= 3;
        
        DrawTextW(hdc, g_tooltipText.c_str(), -1, &textRect, DT_LEFT | DT_WORDBREAK);
        
        EndPaint(hwnd, &ps);
        return 0;
    }
    
    case WM_DESTROY:
        g_hTooltipWindow = NULL;
        return 0;
    }
    
    return DefWindowProc(hwnd, message, wParam, lParam);
}

// Create or show tooltip at position
static void ShowTooltip(HWND hParent, int x, int y, const std::wstring& text)
{
    if (text.empty()) return;
    
    // Create tooltip window class once
    static bool classRegistered = false;
    if (!classRegistered)
    {
        WNDCLASSW wc = {};
        wc.lpfnWndProc = TooltipWndProc;
        wc.hInstance = (HINSTANCE)GetModuleHandleW(NULL);
        wc.lpszClassName = L"RowaTooltipClass";
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        RegisterClassW(&wc);
        classRegistered = true;
    }
    
    g_tooltipText = text;
    
    // Calculate tooltip size
    HDC hdc = GetDC(NULL);
    RECT textRect = { 0, 0, 300, 500 };
    DrawTextW(hdc, text.c_str(), -1, &textRect, DT_CALCRECT | DT_WORDBREAK);
    ReleaseDC(NULL, hdc);
    
    int width = textRect.right - textRect.left + 10;
    int height = textRect.bottom - textRect.top + 6;
    
    if (g_hTooltipWindow)
    {
        // Update existing tooltip
        SetWindowPos(g_hTooltipWindow, HWND_TOPMOST, x, y, width, height, SWP_NOACTIVATE | SWP_SHOWWINDOW);
        InvalidateRect(g_hTooltipWindow, NULL, FALSE);
    }
    else
    {
        // Create new tooltip window
        g_hTooltipWindow = CreateWindowExW(
            WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
            L"RowaTooltipClass",
            NULL,
            WS_POPUP,
            x, y, width, height,
            NULL, NULL, (HINSTANCE)GetModuleHandleW(NULL), NULL
        );
        
        if (g_hTooltipWindow)
        {
            ShowWindow(g_hTooltipWindow, SW_SHOWNA);
        }
    }
}

// Hide tooltip
static void HideTooltip()
{
    if (g_hTooltipWindow)
    {
        ShowWindow(g_hTooltipWindow, SW_HIDE);
    }
}

// Original search edit and button window procedures
static WNDPROC g_originalSearchEditProc = NULL;
static WNDPROC g_originalRefreshBtnProc = NULL;

// Subclassed refresh button window procedure - handles Tab navigation
LRESULT CALLBACK RefreshButtonWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_KEYDOWN:
    {
        // Handle Tab key - send to parent window for focus navigation
        if (wParam == VK_TAB)
        {
            HWND hMainWnd = GetParent(hwnd);
            if (hMainWnd)
            {
                // Post WM_KEYDOWN to parent so it can handle Tab navigation
                PostMessage(hMainWnd, WM_KEYDOWN, wParam, lParam);
            }
            return 0;  // Don't let the button handle it
        }
        // Fall through to default handler for other keys
        return CallWindowProc(g_originalRefreshBtnProc, hwnd, message, wParam, lParam);
    }
    
    default:
        return CallWindowProc(g_originalRefreshBtnProc, hwnd, message, wParam, lParam);
    }
}

// Subclassed search edit window procedure - handles debounce timer
LRESULT CALLBACK SearchEditWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_KEYDOWN:
    {
        // Handle Tab key - send to parent window for focus navigation
        if (wParam == VK_TAB)
        {
            HWND hMainWnd = GetParent(hwnd);
            if (hMainWnd)
            {
                // Post WM_KEYDOWN to parent so it can handle Tab navigation
                PostMessage(hMainWnd, WM_KEYDOWN, wParam, lParam);
            }
            return 0;  // Don't let the edit control handle it
        }
        // Handle Enter key - send to parent window
        if (wParam == VK_RETURN)
        {
            HWND hMainWnd = GetParent(hwnd);
            if (hMainWnd)
            {
                // Post WM_KEYDOWN to parent so it can handle the search Enter logic
                PostMessage(hMainWnd, WM_KEYDOWN, wParam, lParam);
            }
            return 0;  // Don't let the edit control handle it
        }
        // Handle Backspace and Delete - trigger debounce timer for search filter reset
        if (wParam == VK_BACK || wParam == VK_DELETE)
        {
            HWND hMainWnd = GetParent(hwnd);
            if (hMainWnd)
            {
                // Convert ReadSpeed string to integer, default to 40ms if empty or invalid
                int debounceMs = 40;
                if (!SharedVariables::ReadSpeed.empty())
                {
                    try
                    {
                        debounceMs = std::stoi(SharedVariables::ReadSpeed);
                        // Clamp to reasonable range (10ms to 500ms)
                        if (debounceMs < 10) debounceMs = 10;
                        if (debounceMs > 500) debounceMs = 500;
                    }
                    catch (...)
                    {
                        debounceMs = 40;  // Default on parse error
                    }
                }
                
                KillTimer(hMainWnd, ID_SEARCH_DEBOUNCE_TIMER);
                SetTimer(hMainWnd, ID_SEARCH_DEBOUNCE_TIMER, debounceMs, NULL);
            }
            // Fall through to default handler to process the key
            return CallWindowProc(g_originalSearchEditProc, hwnd, message, wParam, lParam);
        }
        // Fall through to default handler for other keys
        return CallWindowProc(g_originalSearchEditProc, hwnd, message, wParam, lParam);
    }
    
    case WM_CHAR:
    {
        // Skip newline character from Enter
        if (wParam == '\r' || wParam == '\n')
            return 0;
        
        // Call original handler for other characters
        CallWindowProc(g_originalSearchEditProc, hwnd, message, wParam, lParam);
        
        // Set debounce timer for search using ReadSpeed setting (in milliseconds)
        HWND hMainWnd = GetParent(hwnd);
        if (hMainWnd)
        {
            // Convert ReadSpeed string to integer, default to 40ms if empty or invalid
            int debounceMs = 40;
            if (!SharedVariables::ReadSpeed.empty())
            {
                try
                {
                    debounceMs = std::stoi(SharedVariables::ReadSpeed);
                    // Clamp to reasonable range (10ms to 500ms)
                    if (debounceMs < 10) debounceMs = 10;
                    if (debounceMs > 500) debounceMs = 500;
                }
                catch (...)
                {
                    debounceMs = 40;  // Default on parse error
                }
            }
            
            KillTimer(hMainWnd, ID_SEARCH_DEBOUNCE_TIMER);
            SetTimer(hMainWnd, ID_SEARCH_DEBOUNCE_TIMER, debounceMs, NULL);
        }
        
        return 0;
    }
    
    default:
        return CallWindowProc(g_originalSearchEditProc, hwnd, message, wParam, lParam);
    }
}

// WndProc: handle painting, mouse, keyboard and network updates
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
    {
        // Initialize logging system
        InitializeLogging();
        LogMessage("=== RowaPickupSlim Started ===");
        
        // Clean up log files older than 31 days
        CleanupOldLogFiles();

        // Load settings from config file before connecting
        SettingsLoader::LoadSettings();
        LogMessage("Settings loaded");
        
        // Initialize localization system with language from settings
        Localization::Initialize(SharedVariables::Language);
        LogMessage("Localization initialized");
        
        // Now update menu strings with localized text
        HMENU hMainMenu = GetMenu(hWnd);
        if (hMainMenu)
        {
            HMENU hFileMenu = GetSubMenu(hMainMenu, 0);
            if (hFileMenu)
            {
                // Update File menu items with localized strings
                ModifyMenuW(hFileMenu, 1, MF_BYCOMMAND | MF_STRING, 1, 
                           Localization::GetString(RowaPickupSlim::STR_FILE_SETTINGS).c_str());
                ModifyMenuW(hFileMenu, 3, MF_BYCOMMAND | MF_STRING, 3, 
                           Localization::GetString(RowaPickupSlim::STR_FILE_EXIT).c_str());
            }
            // Update File menu title
            ModifyMenuW(hMainMenu, 0, MF_BYPOSITION | MF_STRING, (UINT_PTR)GetSubMenu(hMainMenu, 0),
                       Localization::GetString(RowaPickupSlim::STR_FILE_MENU).c_str());
            DrawMenuBar(hWnd);  // Refresh menu bar display
        }
        
        
        
        // Enable mouse leave tracking for tooltips
        TRACKMOUSEEVENT tme = {};
        tme.cbSize = sizeof(TRACKMOUSEEVENT);
        tme.dwFlags = TME_LEAVE;
        tme.hwndTrack = hWnd;
        TrackMouseEvent(&tme);

        // Get window size for positioning controls
        RECT rcClient;
        GetClientRect(hWnd, &rcClient);
        int clientWidth = rcClient.right - rcClient.left;
        int clientHeight = rcClient.bottom - rcClient.top;

        // Create search text box (at bottom) - wide enough for full article codes like "RoWa00020556"
        HWND hSearchEdit = CreateWindowExW(
            WS_EX_CLIENTEDGE,
            L"EDIT",
            L"",
            WS_CHILD | WS_VISIBLE | ES_LEFT | ES_AUTOHSCROLL,
            PADDING, clientHeight - SEARCH_AREA_HEIGHT -5,
            400, 22,
            hWnd,
            (HMENU)(intptr_t)ID_SEARCH_EDIT,
            (HINSTANCE)GetModuleHandleW(NULL),
            NULL
        );
        
        // Subclass search edit to handle typing with debounce
        if (hSearchEdit && !g_originalSearchEditProc)
        {
            g_originalSearchEditProc = (WNDPROC)SetWindowLongPtrW(hSearchEdit, GWLP_WNDPROC, (LONG_PTR)SearchEditWndProc);
        }

        // Create refresh button (positioned after search field)
        HWND hRefreshBtn = CreateWindowW(
            L"BUTTON",
            Localization::GetString(RowaPickupSlim::STR_REFRESH_BUTTON).c_str(),
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            415, clientHeight - SEARCH_AREA_HEIGHT -5 ,
            80, 22,
            hWnd,
            (HMENU)(intptr_t)ID_REFRESH_BUTTON,
            (HINSTANCE)GetModuleHandleW(NULL),
            NULL
        );
        
        // Subclass refresh button to handle Tab navigation
        if (hRefreshBtn && !g_originalRefreshBtnProc)
        {
            g_originalRefreshBtnProc = (WNDPROC)SetWindowLongPtrW(hRefreshBtn, GWLP_WNDPROC, (LONG_PTR)RefreshButtonWndProc);
        }

        // create and start network client
        g_client = std::make_unique<NetworkClient>();
        
        // Connect logging callback
        g_client->LogMessage = [](const std::string& msg) {
            LogMessage(msg);
        };
        
        g_client->MessageReceived = [hWnd](const std::string& type, const std::string& xml) {
            // parse and update
            handle_incoming_xml_and_update_state(xml, hWnd);
        };
        
        // Connection state callback - update UI when connection state changes
        g_client->ConnectionStateChanged = [hWnd](ConnectionState state, ConnectionError error, const std::string& description) {
            std::lock_guard<std::mutex> lock(g_state.mtx);
            
             // Update connection state string with detailed information
            switch (state)
            {
            case ConnectionState::NotConnected:
                g_state.connectionState = "Not connected";
                break;
            case ConnectionState::Attempting:
                g_state.connectionState = "Attempting to connect...";
                break;
            case ConnectionState::Connected:
                g_state.connectionState = "Connected";
                break;
            case ConnectionState::Failed:
            {
                std::string errorMsg = "Connection failed";
                switch (error)
                {
                case ConnectionError::Timeout:
                    errorMsg = "Connection timeout";
                    break;
                case ConnectionError::ConnectionRefused:
                    errorMsg = "Server refused connection";
                    break;
                case ConnectionError::ConnectionReset:
                    errorMsg = "Server broke connection";
                    break;
                case ConnectionError::NetworkUnreachable:
                    errorMsg = "Network unreachable";
                    break;
                case ConnectionError::HostUnreachable:
                    errorMsg = "Host unreachable";
                    break;
                default:
                    errorMsg = description;
                    break;
                }
                g_state.connectionState = errorMsg;
                break;
            }
            }
            
            // Post update to refresh the UI
            PostMessage(hWnd, WM_APP_NETWORK_UPDATE, 0, 0);
        };

        // connect on separate thread using configured IP/port from SharedVariables
        std::thread([hWnd](){
            bool ok = g_client->Connect(SharedVariables::ClientIpAddress, SharedVariables::ClientPort);
            {
                std::lock_guard<std::mutex> lock(g_state.mtx);
                g_state.connectionState = ok ? "Connected" : "Not connected";
            }
            
            // If connection failed, start automatic polling
            if (!ok)
            {
                LogMessage("Initial connection failed. Starting connection polling...");
                g_client->StartConnectionPolling(SharedVariables::ClientIpAddress, SharedVariables::ClientPort);
                PostMessage(hWnd, WM_APP_NETWORK_UPDATE, 0, 0);
                return;
            }
            
            // Connection successful - Connect() already handles the WWKS2 handshake
            // (HelloRequest -> StatusRequest -> StockInfoRequest)
            // Just trigger a UI update
            PostMessage(hWnd, WM_APP_NETWORK_UPDATE, 0, 0);
        }).detach();

        return 0;
    }

    case WM_APP_NETWORK_UPDATE:
    {
        InvalidateRect(hWnd, NULL, TRUE);
        return 0;
    }

    case WM_APP_LANGUAGE_CHANGED:
    {
        // Language changed - update menu and button text, then repaint
        HMENU hMainMenu = GetMenu(hWnd);
        if (hMainMenu)
        {
            HMENU hFileMenu = GetSubMenu(hMainMenu, 0);
            if (hFileMenu)
            {
                // Update File menu items with localized strings
                ModifyMenuW(hFileMenu, 1, MF_BYCOMMAND | MF_STRING, 1, 
                           Localization::GetString(RowaPickupSlim::STR_FILE_SETTINGS).c_str());
                ModifyMenuW(hFileMenu, 3, MF_BYCOMMAND | MF_STRING, 3, 
                           Localization::GetString(RowaPickupSlim::STR_FILE_EXIT).c_str());
            }
            // Update File menu title
            ModifyMenuW(hMainMenu, 0, MF_BYPOSITION | MF_STRING, (UINT_PTR)GetSubMenu(hMainMenu, 0),
                       Localization::GetString(RowaPickupSlim::STR_FILE_MENU).c_str());
            DrawMenuBar(hWnd);  // Refresh menu bar display
        }
        
        // Update refresh button text
        HWND hRefreshBtn = GetDlgItem(hWnd, ID_REFRESH_BUTTON);
        if (hRefreshBtn)
        {
            SetWindowTextW(hRefreshBtn, Localization::GetString(RowaPickupSlim::STR_REFRESH_BUTTON).c_str());
        }
        
        // Repaint to update all dynamic strings
        InvalidateRect(hWnd, NULL, TRUE);
        return 0;
    }



    case WM_TIMER:
    {
        if (wParam == ID_SEARCH_DEBOUNCE_TIMER)
        {
            KillTimer(hWnd, ID_SEARCH_DEBOUNCE_TIMER);
            perform_search_filter(hWnd);
        }
        return 0;
    }

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);

        // snapshot state
        std::string conn, robot, lastType;
        std::vector<std::pair<std::string,int>> articles;
        std::vector<std::tuple<std::string,std::string,int,int,COLORREF,bool>> outputs;
        int sel = -1;
        int scrollOffset = 0;
        {
            std::lock_guard<std::mutex> lock(g_state.mtx);
            conn = g_state.connectionState;
            robot = g_state.robotState;
            lastType = g_state.lastMessageType;
            articles = g_state.articles;
            outputs = g_state.outputRecords;
            sel = g_state.selectedIndex;
            scrollOffset = g_state.scrollOffset;
        }

        // Get window dimensions
        RECT rcWindow;
        GetClientRect(hWnd, &rcWindow);
        int wnd_width = rcWindow.right - rcWindow.left;
        int wnd_height = rcWindow.bottom - rcWindow.top;

        // Draw title bar
        RECT titleRect = { 0, 0, wnd_width, TITLE_BAR_HEIGHT };
        HBRUSH hBrushTitle = CreateSolidBrush(CLR_TITLEBAR_BG);
        FillRect(hdc, &titleRect, hBrushTitle);
        DeleteObject(hBrushTitle);

        // Draw connection status in title bar
        SetTextColor(hdc, CLR_WHITE);
        SetBkMode(hdc, TRANSPARENT);
        
        // Build title bar text: show connection state + robot state
        std::wstring titleText;
        
        // Check connection status to determine what to display
        if (conn == "Connected")
        {
            titleText = Localization::GetString(STR_CONNECTED) + L" " + utf8_to_wstring(robot);
        }
        else
        {
            // Show the actual connection status (not connected, attempting, timeout, etc)
            titleText = utf8_to_wstring(conn);
        }
        
        TextOutW(hdc, wnd_width - 350, 8, titleText.c_str(), (int)titleText.size());

        // Draw status bar at bottom with connection state message
        int statusBarY = wnd_height - STATUS_BAR_HEIGHT;
        RECT statusRect = { 0, statusBarY, wnd_width, wnd_height };
        HBRUSH hBrushStatus = CreateSolidBrush(RGB(240, 240, 240));  // Light grey
        FillRect(hdc, &statusRect, hBrushStatus);
        DeleteObject(hBrushStatus);
        
        // Draw separator line above status bar
        HPEN hPen = CreatePen(PS_SOLID, 1, RGB(200, 200, 200));
        HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
        MoveToEx(hdc, 0, statusBarY, NULL);
        LineTo(hdc, wnd_width, statusBarY);
        SelectObject(hdc, hOldPen);
        DeleteObject(hPen);
        
        // Draw connection state message in status bar
        SetTextColor(hdc, CLR_BLACK);
        std::wstring statusMsg = utf8_to_wstring(conn);
        TextOutW(hdc, PADDING, statusBarY + 2, statusMsg.c_str(), (int)statusMsg.size());

        // Draw header row background
        RECT headerRect = { 0, TITLE_BAR_HEIGHT, wnd_width - SCROLLBAR_WIDTH, TITLE_BAR_HEIGHT + HEADER_HEIGHT };
        HBRUSH hBrushHeader = CreateSolidBrush(CLR_HEADER_BG);
        FillRect(hdc, &headerRect, hBrushHeader);
        DeleteObject(hBrushHeader);

        // Draw column headers
        SetTextColor(hdc, CLR_BLACK);
        int x = PADDING;
        int y = TITLE_BAR_HEIGHT + 5;

        // Column headers matching MainPage.xaml layout
        TextOutW(hdc, x, y, L"", 0);                              // Button col
        x += COL_BUTTON;
        TextOutW(hdc, x, y, Localization::GetString(RowaPickupSlim::STR_COL_QUANTITY).c_str(), 
                (int)Localization::GetString(RowaPickupSlim::STR_COL_QUANTITY).length());
        x += COL_QTY;
        TextOutW(hdc, x, y, Localization::GetString(RowaPickupSlim::STR_COL_ARTICLE).c_str(), 
                (int)Localization::GetString(RowaPickupSlim::STR_COL_ARTICLE).length());
        x += COL_ARTICLE_ID;

        // Calculate visible rows
        int contentHeight = wnd_height - TITLE_BAR_HEIGHT - HEADER_HEIGHT - SEARCH_AREA_HEIGHT - STATUS_BAR_HEIGHT;
        int visibleRows = contentHeight / ROW_HEIGHT;
        int totalRows = (int)articles.size();
        int maxScroll = (totalRows > visibleRows) ? (totalRows - visibleRows) : 0;
        
        // Clamp scroll offset
        if (scrollOffset > maxScroll) scrollOffset = maxScroll;
        if (scrollOffset < 0) scrollOffset = 0;

        // Draw article rows
        {
            int rowY = TITLE_BAR_HEIGHT + HEADER_HEIGHT;
            int rowIndex = 0;

            for (int i = scrollOffset; i < totalRows && rowIndex < visibleRows; i++, rowIndex++)
            {
                const auto& art = articles[i];
                const std::string& id = art.first;
                int qty = art.second;

                // Find color for this article
                COLORREF fillColor = (qty == 0) ? CLR_GREY : CLR_PURPLE;
                std::wstring tooltipMsg;  // For output record tooltip
                
                for (const auto& rec : outputs)
                {
                    if (std::get<1>(rec) == id)
                    {
                        fillColor = std::get<4>(rec);  // 6-tuple: idx 4 is color
                        int requested = std::get<2>(rec);
                        int delivered = std::get<3>(rec);
                        bool isOurs = std::get<5>(rec);  // 6-tuple: idx 5 is ownership
                        
                        if (fillColor == CLR_BLUE)
                        {
                            // Queued (our output)
                            tooltipMsg = Localization::GetString(RowaPickupSlim::STR_OUR_REQUEST_LOADING);
                        }
                        else if (fillColor == CLR_YELLOW)
                        {
                            // Queued (other client's output)
                            tooltipMsg = Localization::GetString(RowaPickupSlim::STR_OTHER_REQUEST_LOADING);
                        }
                        else if (fillColor == CLR_ORANGE)
                        {
                            // InProcess
                            tooltipMsg = Localization::GetString(RowaPickupSlim::STR_PICKER_LOADING);
                        }
                        else if (fillColor == CLR_RED)
                        {
                            // Incomplete
                            if (delivered == 0)
                            {
                                tooltipMsg = Localization::GetString(RowaPickupSlim::STR_PICKER_FAILED);
                            }
                            else
                            {
                                tooltipMsg = Localization::GetString(RowaPickupSlim::STR_PICKER_PARTIAL_FULL);
                                size_t pos1 = tooltipMsg.find(L"{0}");
                                if (pos1 != std::wstring::npos)
                                    tooltipMsg.replace(pos1, 3, utf8_to_wstring(std::to_string(delivered)));
                                size_t pos2 = tooltipMsg.find(L"{1}");
                                if (pos2 != std::wstring::npos)
                                    tooltipMsg.replace(pos2, 3, utf8_to_wstring(std::to_string(requested)));
                            }
                        }
                        break;
                    }
                }

                // Draw button column with color
                RECT rowRect = { 0, rowY, COL_BUTTON, rowY + ROW_HEIGHT };
                HBRUSH brush = CreateSolidBrush(fillColor);
                FillRect(hdc, &rowRect, brush);
                DeleteObject(brush);

                // Highlight if selected - draw frame around entire row with light grey
                if (i == sel)
                {
                    RECT fullRowRect = { 0, rowY, wnd_width - SCROLLBAR_WIDTH, rowY + ROW_HEIGHT };
                    HPEN hPen = CreatePen(PS_SOLID, 2, RGB(150, 150, 150));  // Light grey frame
                    HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
                    HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
                    
                    // Draw rectangle frame (no fill - NULL_BRUSH ensures only outline)
                    Rectangle(hdc, fullRowRect.left, fullRowRect.top, fullRowRect.right, fullRowRect.bottom);
                    
                    SelectObject(hdc, hOldBrush);
                    SelectObject(hdc, hOldPen);
                    DeleteObject(hPen);
                }

                // Draw text data in columns
                x = COL_BUTTON + PADDING;
                SetTextColor(hdc, CLR_BLACK);

                // Quantity
                std::string qtyStr = std::to_string(qty);
                std::wstring wQty = utf8_to_wstring(qtyStr);
                TextOutW(hdc, x, rowY + 5, wQty.c_str(), (int)wQty.size());
                x += COL_QTY;

                // Article ID
                std::wstring wId = utf8_to_wstring(id);
                TextOutW(hdc, x, rowY + 5, wId.c_str(), (int)wId.size());
                x += COL_ARTICLE_ID;

                rowY += ROW_HEIGHT;
            }
        }

        // Draw scrollbar
        {
            int scrollbarTop = TITLE_BAR_HEIGHT + HEADER_HEIGHT;
            int scrollbarHeight = contentHeight;
            int scrollbarLeft = wnd_width - SCROLLBAR_WIDTH;

            // Scrollbar background
            RECT scrollbarBg = { scrollbarLeft, scrollbarTop, wnd_width, scrollbarTop + scrollbarHeight };
            HBRUSH scrollbarBrush = CreateSolidBrush(RGB(220, 220, 220));
            FillRect(hdc, &scrollbarBg, scrollbarBrush);
            DeleteObject(scrollbarBrush);

            // Scrollbar thumb
            if (totalRows > visibleRows)
            {
                int thumbHeight = (visibleRows > 0) ? ((visibleRows * scrollbarHeight) / totalRows) : 20;
                if (thumbHeight < 20) thumbHeight = 20;
                
                int thumbTop = scrollbarTop;
                if (maxScroll > 0)
                    thumbTop = scrollbarTop + (scrollOffset * (scrollbarHeight - thumbHeight)) / maxScroll;
                
                RECT scrollbarThumb = { scrollbarLeft + 2, thumbTop, wnd_width - 2, thumbTop + thumbHeight };
                HBRUSH thumbBrush = CreateSolidBrush(RGB(100, 100, 100));
                FillRect(hdc, &scrollbarThumb, thumbBrush);
                DeleteObject(thumbBrush);
            }
        }

        EndPaint(hWnd, &ps);
        return 0;
    }

    case WM_MOUSEMOVE:
    {
        int mx = GET_X_LPARAM(lParam);
        int my = GET_Y_LPARAM(lParam);
        
        RECT rcWindow;
        GetClientRect(hWnd, &rcWindow);
        int wnd_width = rcWindow.right - rcWindow.left;
        int wnd_height = rcWindow.bottom - rcWindow.top;
        
        // Handle scrollbar thumb dragging
        if (g_scrollbarState.isDragging)
        {
            // Calculate the scrollbar area dimensions
            int contentHeight = wnd_height - TITLE_BAR_HEIGHT - HEADER_HEIGHT - SEARCH_AREA_HEIGHT - STATUS_BAR_HEIGHT;
            int scrollbarTop = TITLE_BAR_HEIGHT + HEADER_HEIGHT;
            int scrollbarHeight = contentHeight;
            int scrollbarLeft = wnd_width - SCROLLBAR_WIDTH;
            
            // Get current state info
            std::lock_guard<std::mutex> lock(g_state.mtx);
            int totalRows = (int)g_state.articles.size();
            int visibleRows = contentHeight / ROW_HEIGHT;
            int maxScroll = (totalRows > visibleRows) ? (totalRows - visibleRows) : 0;
            
            if (maxScroll > 0)
            {
                // Calculate thumb height
                int thumbHeight = (visibleRows > 0) ? ((visibleRows * scrollbarHeight) / totalRows) : 20;
                if (thumbHeight < SCROLLBAR_THUMB_MIN_HEIGHT) thumbHeight = SCROLLBAR_THUMB_MIN_HEIGHT;
                int scrollbarTrackHeight = scrollbarHeight - thumbHeight;
                
                // Calculate new scroll offset based on mouse position
                int currentThumbY = my - scrollbarTop;
                currentThumbY = (currentThumbY < 0) ? 0 : (currentThumbY > scrollbarTrackHeight) ? scrollbarTrackHeight : currentThumbY;
                
                // Map thumb position to scroll offset
                int newScrollOffset = (currentThumbY * maxScroll) / scrollbarTrackHeight;
                newScrollOffset = (newScrollOffset < 0) ? 0 : (newScrollOffset > maxScroll) ? maxScroll : newScrollOffset;
                
                // Only redraw if scroll offset actually changed
                if (newScrollOffset != g_state.scrollOffset)
                {
                    g_state.scrollOffset = newScrollOffset;
                    
                    // Invalidate the content area (list + scrollbar) to redraw with erasing background
                    RECT contentRect = { 0, TITLE_BAR_HEIGHT + HEADER_HEIGHT, wnd_width, wnd_height - SEARCH_AREA_HEIGHT - STATUS_BAR_HEIGHT };
                    InvalidateRect(hWnd, &contentRect, TRUE);  // TRUE = erase background before repainting
                }
            }
            
            return 0;
        }
        
        // Check if hovering over title bar (connection status area)
        if (my < TITLE_BAR_HEIGHT && mx > wnd_width - 300)
        {
            // Build device list text
            std::vector<std::tuple<std::string,std::string,std::string,std::string>> devList;
            {
                std::lock_guard<std::mutex> lock(g_state.mtx);
                devList = g_state.devices;
            }
            
            if (!devList.empty())
            {
                std::wstring tooltipText = L"Apparatuur:\n\n";
                for (const auto& dev : devList)
                {
                    tooltipText += utf8_to_wstring(std::get<1>(dev));
                    tooltipText += L": ";
                    tooltipText += utf8_to_wstring(std::get<3>(dev));
                    tooltipText += L"\n";
                }
                
                // Convert client coordinates to screen coordinates
                POINT pt = { mx + 15, my + 15 };
                ClientToScreen(hWnd, &pt);
                
                // Show custom tooltip at screen position
                ShowTooltip(hWnd, pt.x, pt.y, tooltipText);
            }
        }
        // Check if hovering over article row button (colored status)
        else if (my >= TITLE_BAR_HEIGHT + HEADER_HEIGHT)
        {
            int contentHeight = rcWindow.bottom - TITLE_BAR_HEIGHT - HEADER_HEIGHT - SEARCH_AREA_HEIGHT;
            int visibleRows = contentHeight / ROW_HEIGHT;
            
            if (mx < COL_BUTTON && my < TITLE_BAR_HEIGHT + HEADER_HEIGHT + contentHeight)
            {
                // Get the row index
                int rowOffset = (my - TITLE_BAR_HEIGHT - HEADER_HEIGHT) / ROW_HEIGHT;
                
                std::vector<std::pair<std::string,int>> articles;
                std::vector<std::tuple<std::string,std::string,int,int,COLORREF,bool>> outputs;
                int scrollOffset = 0;
                
                {
                    std::lock_guard<std::mutex> lock(g_state.mtx);
                    articles = g_state.articles;
                    outputs = g_state.outputRecords;
                    scrollOffset = g_state.scrollOffset;
                }
                
                int rowIdx = scrollOffset + rowOffset;
                if (rowIdx < (int)articles.size())
                {
                    const auto& art = articles[rowIdx];
                    
                    // Look for output record with tooltip
                    for (const auto& rec : outputs)
                    {
                        if (std::get<1>(rec) == art.first)
                        {
                            COLORREF color = std::get<4>(rec);
                            int delivered = std::get<3>(rec);
                            int requested = std::get<2>(rec);
                            bool isOurs = std::get<5>(rec);
                            
                            std::wstring tooltipMsg;
                            if (color == CLR_BLUE)
                            {
                                // Our output - Queued
                                tooltipMsg = Localization::GetString(RowaPickupSlim::STR_OUR_REQUEST_LOADING);
                            }
                            else if (color == CLR_YELLOW)
                            {
                                // Other client's output - Queued
                                tooltipMsg = Localization::GetString(RowaPickupSlim::STR_OTHER_REQUEST_LOADING);
                            }
                            else if (color == CLR_ORANGE)
                            {
                                // InProcess
                                tooltipMsg = Localization::GetString(RowaPickupSlim::STR_PICKER_LOADING);
                            }
                            else if (color == CLR_RED)
                            {
                                // Incomplete
                                if (delivered == 0)
                                {
                                    tooltipMsg = Localization::GetString(RowaPickupSlim::STR_PICKER_FAILED);
                                }
                                else
                                {
                                    tooltipMsg = Localization::GetString(RowaPickupSlim::STR_PICKER_PARTIAL_FULL);
                                    size_t pos1 = tooltipMsg.find(L"{0}");
                                    if (pos1 != std::wstring::npos)
                                        tooltipMsg.replace(pos1, 3, utf8_to_wstring(std::to_string(delivered)));
                                    size_t pos2 = tooltipMsg.find(L"{1}");
                                    if (pos2 != std::wstring::npos)
                                        tooltipMsg.replace(pos2, 3, utf8_to_wstring(std::to_string(requested)));
                                }
                            }
                            
                            POINT pt = { mx + 15, my + 15 };
                            ClientToScreen(hWnd, &pt);
                            ShowTooltip(hWnd, pt.x, pt.y, tooltipMsg);
                            return 0;
                        }
                    }
                }
            }
            
            // Not hovering over anything with a tooltip
            HideTooltip();
        }
        else
        {
            // Hide tooltip when not hovering
            HideTooltip();
        }
        
        return 0;
    }

    case WM_MOUSELEAVE:
    {
        // Hide tooltip when mouse leaves window
        return 0;
    }

    case WM_LBUTTONDOWN:
    {
        int mx = GET_X_LPARAM(lParam);
        int my = GET_Y_LPARAM(lParam);
        
        RECT rcWindow;
        GetClientRect(hWnd, &rcWindow);
        int wnd_width = rcWindow.right - rcWindow.left;
        int wnd_height = rcWindow.bottom - rcWindow.top;
        int scrollbarLeft = wnd_width - SCROLLBAR_WIDTH;
        
        // Check if click is on scrollbar
        if (mx >= scrollbarLeft)
        {
            int contentHeight = wnd_height - TITLE_BAR_HEIGHT - HEADER_HEIGHT - SEARCH_AREA_HEIGHT - STATUS_BAR_HEIGHT;
            int scrollbarTop = TITLE_BAR_HEIGHT + HEADER_HEIGHT;
            int scrollbarHeight = contentHeight;
            
            std::lock_guard<std::mutex> lock(g_state.mtx);
            int totalRows = (int)g_state.articles.size();
            int visibleRows = contentHeight / ROW_HEIGHT;
            int maxScroll = (totalRows > visibleRows) ? (totalRows - visibleRows) : 0;
            
            if (maxScroll > 0 && my >= scrollbarTop && my < scrollbarTop + scrollbarHeight)
            {
                // Calculate thumb dimensions
                int thumbHeight = (visibleRows > 0) ? ((visibleRows * scrollbarHeight) / totalRows) : 20;
                if (thumbHeight < SCROLLBAR_THUMB_MIN_HEIGHT) thumbHeight = SCROLLBAR_THUMB_MIN_HEIGHT;
                
                // Calculate current thumb position
                int scrollbarTrackHeight = scrollbarHeight - thumbHeight;
                int thumbTop = scrollbarTop + (g_state.scrollOffset * scrollbarTrackHeight) / maxScroll;
                
                // Check if click is on the thumb
                if (my >= thumbTop && my < thumbTop + thumbHeight)
                {
                    // Start dragging the thumb
                    g_scrollbarState.isDragging = true;
                    g_scrollbarState.thumbStartY = my;
                    g_scrollbarState.scrollStartOffset = g_state.scrollOffset;
                    SetCapture(hWnd);  // Capture mouse even outside window
                    
                    LogMessage("Scrollbar thumb drag started");
                    InvalidateRect(hWnd, NULL, FALSE);
                    return 0;
                }
                else if (my < thumbTop)
                {
                    // Page up
                    g_state.scrollOffset -= (visibleRows - 1);
                    if (g_state.scrollOffset < 0) g_state.scrollOffset = 0;
                    
                    // Redraw content area with background erased
                    RECT contentRect = { 0, TITLE_BAR_HEIGHT + HEADER_HEIGHT, wnd_width, wnd_height - SEARCH_AREA_HEIGHT - STATUS_BAR_HEIGHT };
                    InvalidateRect(hWnd, &contentRect, TRUE);
                    return 0;
                }
                else
                {
                    // Page down
                    g_state.scrollOffset += (visibleRows - 1);
                    if (g_state.scrollOffset > maxScroll) g_state.scrollOffset = maxScroll;
                    
                    // Redraw content area with background erased
                    RECT contentRect = { 0, TITLE_BAR_HEIGHT + HEADER_HEIGHT, wnd_width, wnd_height - SEARCH_AREA_HEIGHT - STATUS_BAR_HEIGHT };
                    InvalidateRect(hWnd, &contentRect, TRUE);
                    return 0;
                }
            }
        }
        
        // Regular article row click (not on scrollbar)
        int contentHeight = rcWindow.bottom - TITLE_BAR_HEIGHT - HEADER_HEIGHT - SEARCH_AREA_HEIGHT;
        
        if (my >= TITLE_BAR_HEIGHT + HEADER_HEIGHT && my < TITLE_BAR_HEIGHT + HEADER_HEIGHT + contentHeight)
        {
            int rowOffset = (my - TITLE_BAR_HEIGHT - HEADER_HEIGHT) / ROW_HEIGHT;
            {
                std::lock_guard<std::mutex> lock(g_state.mtx);
                int idx = g_state.scrollOffset + rowOffset;
                if (idx < (int)g_state.articles.size())
                {
                    g_state.selectedIndex = idx;
                }
                else
                {
                    g_state.selectedIndex = -1;
                }
            }
        }
        InvalidateRect(hWnd, NULL, TRUE);
        return 0;
    }

    case WM_LBUTTONDBLCLK:
    {
        int mx = GET_X_LPARAM(lParam);
        int my = GET_Y_LPARAM(lParam);
        
        RECT rcWindow;
        GetClientRect(hWnd, &rcWindow);
        
        // calculate clicked row
        int contentHeight = rcWindow.bottom - TITLE_BAR_HEIGHT - HEADER_HEIGHT - SEARCH_AREA_HEIGHT;
        
        {
            char debugMsg[256];
            snprintf(debugMsg, sizeof(debugMsg), "WM_LBUTTONDBLCLK: mx=%d, my=%d, contentTop=%d, contentBottom=%d", 
                mx, my, TITLE_BAR_HEIGHT + HEADER_HEIGHT, TITLE_BAR_HEIGHT + HEADER_HEIGHT + contentHeight);
            LogMessage(debugMsg);
        }
        
        if (my >= TITLE_BAR_HEIGHT + HEADER_HEIGHT && my < TITLE_BAR_HEIGHT + HEADER_HEIGHT + contentHeight)
        {
            int rowOffset = (my - TITLE_BAR_HEIGHT - HEADER_HEIGHT) / ROW_HEIGHT;
            
            std::string articleId;
            int articleQty = 0;
            {
                std::lock_guard<std::mutex> lock(g_state.mtx);
                int idx = g_state.scrollOffset + rowOffset;
                if (idx >= 0 && idx < (int)g_state.articles.size())
                {
                    articleId = g_state.articles[idx].first;
                    articleQty = g_state.articles[idx].second;
                    
                    char debugMsg[256];
                    snprintf(debugMsg, sizeof(debugMsg), "  Double-click on row %d: %s, qty=%d", idx, articleId.c_str(), articleQty);
                    LogMessage(debugMsg);
                }
            }
            
            if (!articleId.empty() && articleQty > 0)
            {
                LogMessage("  Proposing output...");
                propose_output_for_article(hWnd, articleId, articleQty);
            }
            else
            {
                LogMessage("  Article ID empty or qty <= 0");
            }
        }
        else
        {
            LogMessage("  Double-click outside content area");
        }
        return 0;
    }

    case WM_LBUTTONUP:
    {
        if (g_scrollbarState.isDragging)
        {
            g_scrollbarState.isDragging = false;
            ReleaseCapture();  // Release mouse capture
            LogMessage("Scrollbar thumb drag ended");
            
            // Redraw content area with background erased to ensure clean display
            RECT rcWindow;
            GetClientRect(hWnd, &rcWindow);
            RECT contentRect = { 0, TITLE_BAR_HEIGHT + HEADER_HEIGHT, rcWindow.right, rcWindow.bottom - SEARCH_AREA_HEIGHT - STATUS_BAR_HEIGHT };
            InvalidateRect(hWnd, &contentRect, TRUE);
            return 0;
        }
        return 0;
    }

    case WM_MOUSEWHEEL:
    {
        int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
        int scrollLines = zDelta > 0 ? -3 : 3;  // Negative = scroll up, positive = scroll down
        
        RECT rcWindow;
        GetClientRect(hWnd, &rcWindow);
        
        {
            std::lock_guard<std::mutex> lock(g_state.mtx);
            int contentHeight = rcWindow.bottom - TITLE_BAR_HEIGHT - HEADER_HEIGHT - SEARCH_AREA_HEIGHT;
            int visibleRows = contentHeight / ROW_HEIGHT;
            int totalRows = (int)g_state.articles.size();
            int maxScroll = (totalRows > visibleRows) ? (totalRows - visibleRows) : 0;
            
            g_state.scrollOffset += scrollLines;
            if (g_state.scrollOffset > maxScroll) g_state.scrollOffset = maxScroll;
            if (g_state.scrollOffset < 0) g_state.scrollOffset = 0;
        }
        
        InvalidateRect(hWnd, NULL, TRUE);
        return 0;
    }

    case WM_KEYDOWN:
    {
        bool handled = false;
        HWND hSearchEdit = GetDlgItem(hWnd, ID_SEARCH_EDIT);
        HWND focused = GetFocus();
        
        // Handle Tab and Shift+Tab for focus navigation
        if (wParam == VK_TAB)
        {
            bool shiftPressed = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
            handle_tab_navigation(hWnd, shiftPressed);
            return 0;
        }
        
        // Handle Enter in search field
        if (focused == hSearchEdit && wParam == VK_RETURN)
        {
            LogMessage("WM_KEYDOWN: Enter in search field detected");
            
            // Check if exactly 1 article is shown
            int articleCount = 0;
            std::string singleArticleId;
            int singleArticleQty = 0;
            {
                std::lock_guard<std::mutex> lock(g_state.mtx);
                articleCount = (int)g_state.articles.size();
                if (articleCount == 1)
                {
                    singleArticleId = g_state.articles[0].first;
                    singleArticleQty = g_state.articles[0].second;
                }
            }
            
            {
                char debugMsg[256];
                snprintf(debugMsg, sizeof(debugMsg), "  Article count: %d, qty: %d", articleCount, singleArticleQty);
                LogMessage(debugMsg);
            }
            
            if (articleCount == 1 && singleArticleQty > 0)
            {
                LogMessage("  Proposing output for single article...");
                // Auto-propose output for this single article
                propose_output_for_article(hWnd, singleArticleId, singleArticleQty);
            }
            return 0;
        }
        
        // Handle ESC to clear search
        if (focused == hSearchEdit && wParam == VK_ESCAPE)
        {
            SetWindowTextW(hSearchEdit, L"");
            KillTimer(hWnd, ID_SEARCH_DEBOUNCE_TIMER);
            perform_search_filter(hWnd);
            return 0;
        }
        
        if (wParam == VK_UP || wParam == VK_DOWN || wParam == VK_PRIOR || wParam == VK_NEXT)
        {
            std::lock_guard<std::mutex> lock(g_state.mtx);
            int n = (int)g_state.articles.size();
            
            RECT rcWindow;
            GetClientRect(hWnd, &rcWindow);
            int contentHeight = rcWindow.bottom - TITLE_BAR_HEIGHT - HEADER_HEIGHT - SEARCH_AREA_HEIGHT;
            int visibleRows = contentHeight / ROW_HEIGHT;
            int maxScroll = (n > visibleRows) ? (n - visibleRows) : 0;
            
            if (n > 0)
            {
                if (wParam == VK_UP)
                {
                    if (g_state.selectedIndex > 0) g_state.selectedIndex--;
                    else g_state.selectedIndex = 0;
                    
                    // Scroll if selected row is above visible area
                    if (g_state.selectedIndex < g_state.scrollOffset)
                        g_state.scrollOffset = g_state.selectedIndex;
                    
                    handled = true;
                }
                else if (wParam == VK_DOWN)
                {
                    if (g_state.selectedIndex < n-1) g_state.selectedIndex++;
                    else g_state.selectedIndex = n-1;
                    
                    // Scroll if selected row is below visible area
                    if (g_state.selectedIndex >= g_state.scrollOffset + visibleRows)
                        g_state.scrollOffset = g_state.selectedIndex - visibleRows + 1;
                    
                    handled = true;
                }
                else if (wParam == VK_PRIOR)  // Page Up
                {
                    g_state.scrollOffset -= visibleRows;
                    if (g_state.scrollOffset < 0) g_state.scrollOffset = 0;
                    handled = true;
                }
                else if (wParam == VK_NEXT)  // Page Down
                {
                    g_state.scrollOffset += visibleRows;
                    if (g_state.scrollOffset > maxScroll) g_state.scrollOffset = maxScroll;
                    handled = true;
                }
            }
        }
        else if (wParam == VK_RETURN)
        {
            // Handle Enter on selected article (not in search field)
            std::lock_guard<std::mutex> lock(g_state.mtx);
            int n = (int)g_state.articles.size();
            int sel = g_state.selectedIndex;
            
            if (n > 0 && sel >= 0 && sel < n)
            {
                std::string id = g_state.articles[sel].first;
                int qty = g_state.articles[sel].second;
                propose_output_for_article(hWnd, id, qty);
                handled = true;
            }
        }
        
        if (handled) { InvalidateRect(hWnd, NULL, TRUE); return 0; }
        break;
    }

    case WM_COMMAND:
    {
        int control_id = LOWORD(wParam);

        // Handle File menu commands
        if (control_id == 1)  // Settings
        {
            SettingsDialog::Create(hWnd);
            return 0;
        }
        else if (control_id == 3)  // Exit
        {
            PostMessage(hWnd, WM_DESTROY, 0, 0);
            return 0;
        }

        switch (control_id)
        {
            case ID_REFRESH_BUTTON:
            {
                // Refresh button logic with state machine
                if (g_client)
                {
                    LogMessage("Refresh button pressed");
                    
                    NetworkConnectionState currentState = g_client->GetNetworkState();
                    
                    // If in state 3 (PausedWaitingForUser), reset to state 0 (ReadyToConnect)
                    if (currentState == NetworkConnectionState::Disconnected_PausedWaitingForUser)
                    {
                        LogMessage("  State 3 detected: Resetting to state 0 (ReadyToConnect) for reconnection attempt");
                        g_client->SetNetworkState(NetworkConnectionState::Disconnected_ReadyToConnect);
                        g_client->StartConnectionPolling(SharedVariables::ClientIpAddress, SharedVariables::ClientPort);
                        return 0;
                    }
                    
                    // If connected (state 1), just resend StockInfoRequest
                    if (g_client->IsConnected() && currentState == NetworkConnectionState::Connected_StayAlive)
                    {
                        LogMessage("  State 1 (Connected): Sending StockInfoRequest without reconnect");
                        
                        std::string id = make_unique_id();
                        std::ostringstream ss;
                        ss << "<WWKS Version=\"2.0\" TimeStamp=\"";
                        auto now = std::chrono::system_clock::now();
                        std::time_t t = std::chrono::system_clock::to_time_t(now);
                        std::tm tm{};
                        gmtime_s(&tm, &t);
                        char buf[64];
                        strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tm);
                        ss << buf << "\">";
                        ss << "<StockInfoRequest Id=\"" << id << "\" Source=\"" << SharedVariables::SourceNumber << "\" Destination=\"999\" IncludePacks=\"False\" IncludeArticleDetails=\"False\">";
                        ss << "<Criteria StockLocationId=\"" << SharedVariables::RobotStockLocation << "\" />";
                        ss << "</StockInfoRequest>";
                        ss << "</WWKS>";
                        
                        // Clear search field and reset to full list
                        HWND hEdit = GetDlgItem(hWnd, ID_SEARCH_EDIT);
                        if (hEdit) SetWindowTextW(hEdit, L"");
                        
                        {
                            std::lock_guard<std::mutex> lock(g_state.mtx);
                            g_state.selectedIndex = -1;
                            g_state.scrollOffset = 0;
                            g_state.articles = g_state.fullArticlesList;
                        }
                        
                        g_client->SendMessage(ss.str());
                        return 0;
                    }
                    
                    // Any other state, attempt reconnection
                    LogMessage("  Not connected: Starting connection polling...");
                    g_client->SetNetworkState(NetworkConnectionState::Disconnected_ReadyToConnect);
                    g_client->StartConnectionPolling(SharedVariables::ClientIpAddress, SharedVariables::ClientPort);
                }
                break;
            }
        }
        break;
    }

    case WM_DESTROY:
    {
        if (g_client)
        {
            g_client->Close();
            g_client.reset();
        }
        SettingsDialog::Destroy();
        PostQuitMessage(0);
        return 0;
    }

    case WM_RBUTTONDOWN:
    {
        // Right-click opens settings menu
        POINT pt;
        GetCursorPos(&pt);
        
        HMENU hMenu = CreatePopupMenu();
        AppendMenuW(hMenu, MF_STRING, 1, Localization::GetString(RowaPickupSlim::STR_MENU_SETTINGS).c_str());
        AppendMenuW(hMenu, MF_STRING, 4, Localization::GetString(RowaPickupSlim::STR_MENU_OUTPUT_SELECTED).c_str());
        AppendMenuW(hMenu, MF_STRING, 2, Localization::GetString(RowaPickupSlim::STR_MENU_REFRESH_STOCK).c_str());
        AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
        AppendMenuW(hMenu, MF_STRING, 3, Localization::GetString(RowaPickupSlim::STR_MENU_EXIT).c_str());

        UINT choice = TrackPopupMenuEx(hMenu, TPM_RETURNCMD, pt.x, pt.y, hWnd, NULL);
        DestroyMenu(hMenu);

        if (choice == 1)
        {
            // Open settings dialog
            SettingsDialog::Create(hWnd);
        }
        else if (choice == 4)
        {
            // Output selected pickup (all packs for this article)
            int sel = -1;
            std::string artId;
            int artQty = 0;
            {
                std::lock_guard<std::mutex> lock(g_state.mtx);
                sel = g_state.selectedIndex;
                if (sel >= 0 && sel < (int)g_state.articles.size())
                {
                    artId = g_state.articles[sel].first;
                    artQty = g_state.articles[sel].second;
                }
            }
            
            if (sel >= 0 && artQty > 0)
            {
                if (show_output_confirmation_dialog(hWnd, artId, artQty))
                {
                    std::string aid = artId;
                    int aq = artQty;
                    std::thread([aid, aq](){
                        send_output_request_for_article(aid, aq);
                    }).detach();
                }
            }
            else
            {
                MessageBoxW(hWnd, Localization::GetString(RowaPickupSlim::STR_NO_ARTICLE_SELECTED).c_str(), 
                           Localization::GetString(RowaPickupSlim::STR_NO_ARTICLE_INFO).c_str(), 
                           MB_OK | MB_ICONINFORMATION);
            }
        }
        else if (choice == 3)
        {
            PostMessage(hWnd, WM_DESTROY, 0, 0);
        }
        return 0;
    }

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Minimal WinMain: register class, create window and run message loop
int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    // Initialize common controls (required for tooltips)
    InitCommonControls();
    
    const wchar_t CLASS_NAME[] = L"RowaPickupMainWindowClass";

    // Load icon from resource
    HINSTANCE hInst = (HINSTANCE)GetModuleHandleW(NULL);
    HICON hIcon = LoadIconW(hInst, MAKEINTRESOURCEW(IDR_MAINFRAME));
    HICON hIconSmall = LoadIconW(hInst, MAKEINTRESOURCEW(IDI_MAIN_ICON));

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hIcon = hIcon;        // Set large icon
    wc.hIconSm = hIconSmall; // Set small icon

    RegisterClassExW(&wc);
    
    // Create menu bar
    // Note: Localization will be initialized in WM_CREATE, so we temporarily use English strings here
    // They will be properly localized after window creation
    HMENU hMenu = CreateMenu();
    HMENU hFileMenu = CreatePopupMenu();
    AppendMenuW(hFileMenu, MF_STRING, 1, L"&Settings");
    AppendMenuW(hFileMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hFileMenu, MF_STRING, 3, L"E&xit");
    AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hFileMenu, L"&File");

    HWND hwnd = CreateWindowExW(0, CLASS_NAME, L"Rowa Pickup Terminal",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 520, 480,
        NULL, hMenu, hInstance, NULL);

    if (!hwnd) return 0;

    // Set icons on the window itself for taskbar and title bar
    if (hIcon)
        SendMessageW(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
    if (hIconSmall)
        SendMessageW(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIconSmall);

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);
    
    // Force initial paint
    InvalidateRect(hwnd, NULL, FALSE);

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}
