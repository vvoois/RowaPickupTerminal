#pragma once
// networkclient.h
// Declaration matching the implementation in networkclient.cpp

#include <winsock2.h>
#include <ws2tcpip.h>
#undef SendMessage  // Undefine Windows macro that conflicts with our method name
#include <thread>
#include <atomic>
#include <functional>
#include <string>
#include <vector>
#include <mutex>
#include "SharedVariables.h"  // For NetworkConnectionState enum









namespace RowaPickupSlim
{
    // Connection state machine for network management
    enum class NetworkConnectionState
    {
        Disconnected_ReadyToConnect = 0,    // Ready to connect (attempt up to 10 times)
        Connected_StayAlive = 1,             // Connected, maintain with KeepAlive
        ConnectionIssue_Terminate = 2,       // Connection error, gracefully terminate
        Disconnected_PausedWaitingForUser = 3 // Paused, waiting for user action (Refresh)
    };

    // Connection state enumeration
    enum class ConnectionState
    {
        NotConnected,           // No connection, not attempting
        Attempting,             // Currently trying to connect
        Connected,              // Successfully connected
        Failed                  // Connection failed
    };

    // Connection error type enumeration
    enum class ConnectionError
    {
        None,                   // No error
        Timeout,                // Connection timed out
        ConnectionRefused,      // Server refused connection
        ConnectionReset,        // Connection was reset by peer
        NetworkUnreachable,     // Network is unreachable
        HostUnreachable,        // Host is unreachable
        Other                   // Other error
    };

    class NetworkClient
    {
    public:
        // Callback invoked when a complete WWKS message is received.
        // Parameters: messageType, completeXml
        std::function<void(const std::string&, const std::string&)> MessageReceived;

        // Logging callback: receives log messages
        std::function<void(const std::string&)> LogMessage;
        
        // Connection status callback: receives state and error information
        // Parameters: newState, errorType, errorDescription
        std::function<void(ConnectionState, ConnectionError, const std::string&)> ConnectionStateChanged;

        NetworkClient();
        ~NetworkClient();

        // Connect to server (blocking). After successful connect a receive thread is started.
        bool Connect(const std::string& serverIp, int port);

        // Send a single message (WriteLine-like behaviour)
        bool SendMessage(const std::string& message);

        // Close connection and stop background thread
        void Close();
        
        // Start automatic reconnection polling (tries every 5 seconds, max 10 attempts)
        // Call this when connection fails or settings change
        void StartConnectionPolling(const std::string& serverIp, int port);
        
        // Stop automatic reconnection polling
        void StopConnectionPolling();
        
        // Check if currently connected
        bool IsConnected() const;
        
        // Get current connection state
        ConnectionState GetConnectionState() const;
        
        // Get last error
        ConnectionError GetLastError() const;

        // Get/Set network connection state machine
        NetworkConnectionState GetNetworkState() const;
        void SetNetworkState(NetworkConnectionState newState);
        
        // Get handshake complete state
        bool IsHandshakeComplete() const;

        static bool IsValidIpAddress(const std::string& ipAddress);
        static bool IsValidPort(int port);

    private:
        SOCKET _sock;
        std::thread _recvThread;
        std::atomic<bool> _running;
        std::mutex _mtx;
        
        // Connection state tracking
        ConnectionState _currentState;
        ConnectionError _lastError;
        NetworkConnectionState _networkState;  // State machine for connection management
        
        // Handshake state tracking
        bool _handshakeComplete;
        std::atomic<bool> _helloResponseReceived;
        std::atomic<bool> _statusResponseReceived;
        
        // Connection polling
        std::thread _pollingThread;
        std::atomic<bool> _pollingActive;
        std::string _pollIp;
        int _pollPort;

        // Private methods
        void CloseLocked();
        void NotifyStateChange(ConnectionState newState, ConnectionError error, const std::string& description);
        static std::string RemoveIllegalCharacters(const std::string& input);
        static std::string GetMessageResponseType(const std::string& wwksMessage);
        static ConnectionError GetErrorType(int wsaError);
        void SendHelloRequest();
        void SendStatusRequest();
        void SendStockInfoRequest();
        void ReceiveLoop();
        void PollingLoop();  // Automatic reconnection polling thread

        // non-copyable
        NetworkClient(const NetworkClient&) = delete;
        NetworkClient& operator=(const NetworkClient&) = delete;
    };
}