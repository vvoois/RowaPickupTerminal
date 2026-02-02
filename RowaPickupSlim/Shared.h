#pragma once
// Shared.h
// Application-wide shared definitions: state, constants, enums
// NO system headers - this keeps it portable and app-focused
// Included by main.cpp and shared between app-specific modules

#include <string>
#include <vector>
#include <tuple>
#include <set>
#include <mutex>
#include <memory>

namespace RowaPickupSlim
{
    // Forward declaration
    class NetworkClient;

    // ========================================================================
    // Global Application State
    // ========================================================================
    struct AppState
    {
        // Articles list
        std::vector<std::pair<std::string, int>> articles;           // Filtered articles (ID, qty)
        std::vector<std::pair<std::string, int>> fullArticlesList;   // Full unfiltered list
        
        // Output tracking
        std::vector<std::tuple<std::string, std::string, int, int, int, bool>> outputRecords;
        // Tuple: (orderId, articleId, quantityRequested, packsDelivered, color, isOurOutput)
        std::set<std::string> ourOutputRequestIds;
        
        // UI state
        int selectedIndex = -1;
        int scrollOffset = 0;
        
        // Robot/Device state
        std::string robotState;
        std::vector<std::tuple<std::string, std::string, std::string, std::string>> devices;
        // Device tuple: (type, description, state, stateText)
        
        // Connection state display text
        std::string connectionState = "Not connected";
        
        // Message tracking
        std::string lastMessageType;
        
        // Synchronization
        std::mutex mtx;
    };
    
    extern AppState g_state;
    extern std::unique_ptr<class NetworkClient> g_client;

    // ========================================================================
    // Control IDs (for UI elements)
    // ========================================================================
    enum ControlID
    {
        ID_SEARCH_EDIT = 1001,
        ID_REFRESH_BUTTON = 1003,
        ID_SEARCH_DEBOUNCE_TIMER = 9001
    };

    // ========================================================================
    // Program Constants
    // ========================================================================
    enum class ConnectionState
    {
        NotConnected,
        Attempting,
        Connected,
        Failed
    };

    enum class ConnectionError
    {
        None,
        HostUnreachable,
        ConnectionRefused,
        Timeout,
        Unknown
    };

    enum class NetworkConnectionState
    {
        Disconnected_ReadyToConnect = 0,
        Connected_StayAlive = 1,
        ConnectionIssue_Terminate = 2,
        Disconnected_PausedWaitingForUser = 3
    };

} // namespace RowaPickupSlim
