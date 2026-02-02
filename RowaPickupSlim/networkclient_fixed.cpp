// networkclient.cpp
// Implementation of NetworkClient declared in networkclient.h
// Minimal Winsock-based TCP client that mirrors the C# NetworkClient behavior.

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

#include "networkclient.h"
#include <iostream>
#include <chrono>
#include <sstream>
#include "pugixml.hpp"

namespace RowaPickupSlim
{
    // Constructor
    NetworkClient::NetworkClient()
        : _sock(INVALID_SOCKET), _running(false), _currentState(ConnectionState::NotConnected), 
          _lastError(ConnectionError::None), _networkState(NetworkConnectionState::Disconnected_ReadyToConnect),
          _handshakeComplete(false), _helloResponseReceived(false), _statusResponseReceived(false),
          _pollingActive(false), _pollPort(0)
    {
    }

    // Destructor
    NetworkClient::~NetworkClient()
    {
        Close();
    }

    // Connect to server
    bool NetworkClient::Connect(const std::string& serverIp, int port)
    {
        if (!IsValidIpAddress(serverIp) || !IsValidPort(port))
            return false;

        NotifyStateChange(ConnectionState::Attempting, ConnectionError::None, "Connecting to " + serverIp + ":" + std::to_string(port));

        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
        {
            NotifyStateChange(ConnectionState::Failed, ConnectionError::Other, "WSAStartup failed");
            return false;
        }

        // Close existing connection first (outside mutex lock to avoid deadlock with ReceiveLoop)
        {
            std::lock_guard<std::mutex> lk(_mtx);
            if (_sock != INVALID_SOCKET)
            {
                // Mark as not running to stop ReceiveLoop
                _running.store(false);
                
                // Close the socket
                DWORD noTimeout = 0;
                setsockopt(_sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&noTimeout, sizeof(noTimeout));
                ::shutdown(_sock, SD_BOTH);
                closesocket(_sock);
                _sock = INVALID_SOCKET;
            }
        } // Release mutex here before joining thread
        
        // Join the receive thread OUTSIDE the mutex to avoid deadlock
        if (_recvThread.joinable())
        {
            _recvThread.join();
        }

        std::lock_guard<std::mutex> lk(_mtx);

        addrinfo hints{};
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;

        addrinfo* result = nullptr;
        std::string portStr = std::to_string(port);
        if (getaddrinfo(serverIp.c_str(), portStr.c_str(), &hints, &result) != 0)
        {
            WSACleanup();
            NotifyStateChange(ConnectionState::Failed, ConnectionError::HostUnreachable, "Unable to resolve host: " + serverIp);
            return false;
        }

        SOCKET s = INVALID_SOCKET;
        for (addrinfo* ptr = result; ptr != nullptr; ptr = ptr->ai_next)
        {
            s = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
            if (s == INVALID_SOCKET) continue;

            if (connect(s, ptr->ai_addr, (int)ptr->ai_addrlen) == 0)
            {
                _sock = s;
                break;
            }
            
            int wsaError = WSAGetLastError();
            ConnectionError error = GetErrorType(wsaError);
            closesocket(s);
            s = INVALID_SOCKET;
        }
        freeaddrinfo(result);

        if (_sock == INVALID_SOCKET)
        {
            WSACleanup();
            NotifyStateChange(ConnectionState::Failed, ConnectionError::ConnectionRefused, "Connection refused by server at " + serverIp);
            return false;
        }

        // Set socket receive timeout to 10 seconds so we can detect disconnections
        DWORD recvTimeout = 10000;  // 10 seconds in milliseconds
        if (setsockopt(_sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&recvTimeout, sizeof(recvTimeout)) == SOCKET_ERROR)
        {
            closesocket(_sock);
            _sock = INVALID_SOCKET;
            WSACleanup();
            NotifyStateChange(ConnectionState::Failed, ConnectionError::Other, "Failed to set socket timeout");
            return false;
        }

        // Enable TCP keepalive to detect dead connections
        BOOL keepAlive = TRUE;
        if (setsockopt(_sock, SOL_SOCKET, SO_KEEPALIVE, (const char*)&keepAlive, sizeof(keepAlive)) == SOCKET_ERROR)
        {
            closesocket(_sock);
            _sock = INVALID_SOCKET;
            WSACleanup();
            NotifyStateChange(ConnectionState::Failed, ConnectionError::Other, "Failed to enable TCP keepalive");
            return false;
        }

        // Start receive thread
        _running.store(true);
        _handshakeComplete = false;
        _helloResponseReceived.store(false);
        _statusResponseReceived.store(false);
        _recvThread = std::thread(&NetworkClient::ReceiveLoop, this);

        // Set network state to Connected_StayAlive (state 1)
        SetNetworkState(NetworkConnectionState::Connected_StayAlive);

        // Notify connection established
        NotifyStateChange(ConnectionState::Connected, ConnectionError::None, "Connected successfully");
        
        // Send handshake sequence from main thread (not from ReceiveLoop to avoid deadlock)
        std::thread([this]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            if (_running.load()) SendHelloRequest();
            
            std::this_thread::sleep_for(std::chrono::milliseconds(600));
            if (_running.load() && _helloResponseReceived.load()) SendStatusRequest();
            
            std::this_thread::sleep_for(std::chrono::milliseconds(600));
            if (_running.load() && _statusResponseReceived.load()) SendStockInfoRequest();
        }).detach();

        return true;
    }

    // Send message
    bool NetworkClient::SendMessage(const std::string& message)
    {
        std::lock_guard<std::mutex> lk(_mtx);
        if (_sock == INVALID_SOCKET) return false;

        // Log outgoing message
        if (LogMessage)
        {
            LogMessage("\n>>> OUTGOING MESSAGE <<<");
            LogMessage(message);
            LogMessage(">>> END OUTGOING <<<\n");
        }

        std::string filtered = RemoveIllegalCharacters(message);
        if (filtered.empty()) return false;

        filtered.push_back('\n');

        int total = 0;
        int toSend = static_cast<int>(filtered.size());
        const char* buf = filtered.c_str();

        while (total < toSend)
        {
            int sent = ::send(_sock, buf + total, toSend - total, 0);
            if (sent == SOCKET_ERROR)
                return false;
            total += sent;
        }
        return true;
    }

    // Close connection
    void NetworkClient::Close()
    {
        // Stop polling first (must be before locking to avoid deadlock)
        StopConnectionPolling();
        
        {
            std::lock_guard<std::mutex> lk(_mtx);
            CloseLocked();
        } // Release mutex before joining thread
        
        // Join the receive thread OUTSIDE the mutex to avoid deadlock
        if (_recvThread.joinable())
        {
            _recvThread.join();
        }
        
        WSACleanup();
    }

    // Private: Close locked - must be called with mutex held
    void NetworkClient::CloseLocked()
    {
        _running.store(false);
        if (_sock != INVALID_SOCKET)
        {
            // Disable recv timeout before closing so socket operations complete quickly
            DWORD noTimeout = 0;
            setsockopt(_sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&noTimeout, sizeof(noTimeout));
            
            ::shutdown(_sock, SD_BOTH);
            closesocket(_sock);
            _sock = INVALID_SOCKET;
        }
        // Note: Don't join thread here - it will be done in Close() after releasing mutex
    }
    bool NetworkClient::IsValidIpAddress(const std::string& ipAddress)
    {
        sockaddr_in sa{};
        return inet_pton(AF_INET, ipAddress.c_str(), &(sa.sin_addr)) == 1
            || inet_pton(AF_INET6, ipAddress.c_str(), &(sa.sin_addr)) == 1;
    }

    // Validate port
    bool NetworkClient::IsValidPort(int port)
    {
        return port > 0 && port <= 65535;
    }

    // Private: Remove illegal XML characters
    std::string NetworkClient::RemoveIllegalCharacters(const std::string& input)
    {
        std::string out;
        out.reserve(input.size());
        for (unsigned char c : input)
        {
            if (c == 0x09 || c == 0x0A || c == 0x0D || c >= 0x20)
                out.push_back(static_cast<char>(c));
        }
        return out;
    }

    // Private: Get message type from XML
    std::string NetworkClient::GetMessageResponseType(const std::string& wwksMessage)
    {
        pugi::xml_document doc;
        pugi::xml_parse_result res = doc.load_string(wwksMessage.c_str());
        if (!res) return std::string();

        pugi::xml_node root = doc.child("WWKS");
        if (!root) return std::string();

        if (root.child("HelloResponse")) return "HelloResponse";
        if (root.child("StatusResponse")) return "StatusResponse";
        if (root.child("StockInfoResponse")) return "StockInfoResponse";
        if (root.child("OutputResponse")) return "OutputResponse";
        if (root.child("OutputMessage")) return "OutputMessage";
        if (root.child("InputMessage")) return "InputMessage";
        if (root.child("TaskInfoResponse")) return "TaskInfoResponse";

        return std::string();
    }

    // Private: Receive loop
    void NetworkClient::ReceiveLoop()
    {
        constexpr size_t BUFFER_SIZE = 8192;
        std::vector<char> buffer(BUFFER_SIZE);
        std::string messageBuilder;
        const std::string endTag = "</WWKS>";
        int consecutiveTimeouts = 0;
        const int MAX_CONSECUTIVE_TIMEOUTS = 12; // 2 minutes of no data = disconnect

        while (_running.load())
        {
            int bytesRead = ::recv(_sock, buffer.data(), static_cast<int>(buffer.size()), 0);
            
            // Handle timeout (WSAETIMEDOUT) - connection is still alive, just no data
            if (bytesRead == SOCKET_ERROR)
            {
                int error = WSAGetLastError();
                if (error == WSAETIMEDOUT)
                {
                    // Increment timeout counter
                    consecutiveTimeouts++;
                    
                    // If we have too many consecutive timeouts, consider connection dead
                    if (consecutiveTimeouts >= MAX_CONSECUTIVE_TIMEOUTS)
                    {
                        if (LogMessage)
                        {
                            LogMessage("Connection lost: No data received for " + 
                                     std::to_string(MAX_CONSECUTIVE_TIMEOUTS * 10) + " seconds");
                        }
                        _running.store(false);
                        break;
                    }
                    
                    // Normal timeout - connection is still alive
                    continue;
                }
                // Actual error - connection is broken
                if (LogMessage)
                {
                    std::string errorDesc = "Socket error: " + std::to_string(error);
                    LogMessage(errorDesc);
                }
                _running.store(false);
                break;
            }
            
            // Reset timeout counter on successful receive
            if (bytesRead > 0)
            {
                consecutiveTimeouts = 0;
            }
            
            if (bytesRead == 0)
            {
                // Connection closed gracefully by server
                if (LogMessage)
                {
                    LogMessage("Connection closed by server (recv returned 0)");
                }
                _running.store(false);
                break;
            }

            messageBuilder.append(buffer.data(), buffer.data() + bytesRead);

            size_t pos = messageBuilder.find(endTag);
            while (pos != std::string::npos)
            {
                size_t msgEnd = pos + endTag.length();
                std::string completeMessage = messageBuilder.substr(0, msgEnd);

                // Log incoming message
                if (LogMessage)
                {
                    LogMessage("\n<<< INCOMING MESSAGE <<<");
                    LogMessage(completeMessage);
                    LogMessage("<<< END INCOMING <<<\n");
                }

                // Check if this is a KeepAliveRequest and automatically respond
                if (completeMessage.find("<KeepAliveRequest") != std::string::npos)
                {
                    // Extract the Id, Source, and Destination attributes
                    size_t idPos = completeMessage.find("Id=\"");
                    size_t sourcePos = completeMessage.find("Source=\"");
                    size_t destPos = completeMessage.find("Destination=\"");
                    
                    if (idPos != std::string::npos && sourcePos != std::string::npos && destPos != std::string::npos)
                    {
                        // Extract Id
                        idPos += 4;  // Move past 'Id="'
                        size_t idEnd = completeMessage.find("\"", idPos);
                        std::string keepaliveId = completeMessage.substr(idPos, idEnd - idPos);
                        
                        // Extract Source (server ID)
                        sourcePos += 8;  // Move past 'Source="'
                        size_t sourceEnd = completeMessage.find("\"", sourcePos);
                        std::string serverId = completeMessage.substr(sourcePos, sourceEnd - sourcePos);
                        
                        // Extract Destination (our client ID)
                        destPos += 13;  // Move past 'Destination="'
                        size_t destEnd = completeMessage.find("\"", destPos);
                        std::string clientId = completeMessage.substr(destPos, destEnd - destPos);
                        
                        // Build and send KeepAliveResponse
                        // Swap Source and Destination: we become the Source, server becomes Destination
                        std::ostringstream ss;
                        ss << "<WWKS Version=\"2.0\" TimeStamp=\"";
                        auto now = std::chrono::system_clock::now();
                        std::time_t t = std::chrono::system_clock::to_time_t(now);
                        std::tm tm{};
                        gmtime_s(&tm, &t);
                        char buf[64];
                        strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tm);
                        ss << buf << "\">";
                        ss << "<KeepAliveResponse Id=\"" << keepaliveId << "\" Source=\"" << clientId << "\" Destination=\"" << serverId << "\" />";
                        ss << "</WWKS>";
                        
                        if (LogMessage)
                        {
                            LogMessage("[KEEPALIVE] Received KeepAliveRequest from server, sending KeepAliveResponse");
                        }
                        
                        SendMessage(ss.str());
                    }
                }

                // Dispatch message to message handler
                std::thread dispatchThread([completeMessage, this]() {
                    std::string messageType = GetMessageResponseType(completeMessage);
                    
                    // Handle handshake sequence by setting flags
                    // (actual sending will be done in main receive loop to avoid deadlock)
                    if (messageType == "HelloResponse" && !_helloResponseReceived.load())
                    {
                        if (LogMessage)
                        {
                            LogMessage("[HANDSHAKE] Received HelloResponse");
                        }
                        _helloResponseReceived.store(true);
                    }
                    else if (messageType == "StatusResponse" && !_statusResponseReceived.load())
                    {
                        if (LogMessage)
                        {
                            LogMessage("[HANDSHAKE] Received StatusResponse");
                        }
                        _statusResponseReceived.store(true);
                    }
                    else if (messageType == "StockInfoResponse" && !_handshakeComplete)
                    {
                        if (LogMessage)
                        {
                            LogMessage("[HANDSHAKE] Received StockInfoResponse, handshake complete!");
                        }
                        _handshakeComplete = true;
                    }
                    
                    // Don't dispatch KeepAliveRequest or internal handshake responses during setup
                    if (messageType != "KeepAliveRequest" && this->MessageReceived)
                    {
                        try
                        {
                            this->MessageReceived(messageType, completeMessage);
                        }
                        catch (...)
                        {
                            // swallow exceptions
                        }
                    }
                });




                dispatchThread.detach();

                messageBuilder.erase(0, msgEnd);
                pos = messageBuilder.find(endTag);
            }
        }

        // Clean up socket when loop exits
        {
            std::lock_guard<std::mutex> lk(_mtx);
            if (_sock != INVALID_SOCKET)
            {
                closesocket(_sock);
                _sock = INVALID_SOCKET;
            }
        }

        // Notify disconnection
        if (MessageReceived)
        {
            try { MessageReceived(std::string(), std::string()); } catch (...) {}
        }
        
        // Set state machine to ConnectionIssue_Terminate (state 2)
        // This will trigger cleanup and transition to state 3
        SetNetworkState(NetworkConnectionState::ConnectionIssue_Terminate);
        
        // Notify connection lost with error details
        NotifyStateChange(ConnectionState::NotConnected, ConnectionError::ConnectionReset, "Connection was closed by server");
    }

    // Start automatic reconnection polling
    void NetworkClient::StartConnectionPolling(const std::string& serverIp, int port)
    {
        if (!IsValidIpAddress(serverIp) || !IsValidPort(port))
            return;

        StopConnectionPolling();  // Stop any existing polling first

        _pollIp = serverIp;
        _pollPort = port;
        _pollingActive.store(true);
        _pollingThread = std::thread(&NetworkClient::PollingLoop, this);
    }

    // Stop automatic reconnection polling
    void NetworkClient::StopConnectionPolling()
    {
        _pollingActive.store(false);
        if (_pollingThread.joinable())
        {
            _pollingThread.join();
        }
    }

    // Check if connected
    bool NetworkClient::IsConnected() const
    {
        return _sock != INVALID_SOCKET;
    }

    // Get current connection state
    ConnectionState NetworkClient::GetConnectionState() const
    {
        return _currentState;
    }

    // Get last error
    ConnectionError NetworkClient::GetLastError() const
    {
        return _lastError;
    }

    // Convert WSA error code to ConnectionError
    ConnectionError NetworkClient::GetErrorType(int wsaError)
    {
        switch (wsaError)
        {
        case WSAETIMEDOUT:
            return ConnectionError::Timeout;
        case WSAECONNREFUSED:
            return ConnectionError::ConnectionRefused;
        case WSAECONNRESET:
            return ConnectionError::ConnectionReset;
        case WSAENETUNREACH:
            return ConnectionError::NetworkUnreachable;
        case WSAEHOSTUNREACH:
            return ConnectionError::HostUnreachable;
        default:
            return ConnectionError::Other;
        }
    }

    // Notify state change with error details
    void NetworkClient::NotifyStateChange(ConnectionState newState, ConnectionError error, const std::string& description)
    {
        _currentState = newState;
        _lastError = error;

        if (ConnectionStateChanged)
        {
            try
            {
                ConnectionStateChanged(newState, error, description);
            }
            catch (...)
            {
                // Ignore callback exceptions
            }
        }
    }

    // Automatic reconnection polling thread (tries every 5 seconds, max 10 attempts)
    void NetworkClient::PollingLoop()
    {
        const int MAX_ATTEMPTS = 10;
        const int RETRY_INTERVAL_MS = 5000;  // 5 seconds
        int attemptCount = 0;

        if (LogMessage)
        {
            LogMessage("Connection polling started: will attempt " + std::to_string(MAX_ATTEMPTS) + " times, 5 seconds apart");
        }

        while (_pollingActive.load() && attemptCount < MAX_ATTEMPTS)
        {
            // Only poll if NOT connected
            {
                std::lock_guard<std::mutex> lk(_mtx);
                if (_sock != INVALID_SOCKET)
                {
                    // Already connected, stop polling
                    if (LogMessage)
                    {
                        LogMessage("Connection polling succeeded - connection established!");
                    }
                    _pollingActive.store(false);
                    return;
                }
            }

            attemptCount++;

            // Log polling attempt with clear status
            if (LogMessage)
            {
                std::string msg = "Polling attempt " + std::to_string(attemptCount) + "/" + std::to_string(MAX_ATTEMPTS) + 
                                " - Trying to connect to " + _pollIp + ":" + std::to_string(_pollPort);
                LogMessage(msg);
            }

            // Try to connect
            if (Connect(_pollIp, _pollPort))
            {
                if (LogMessage)
                {
                    LogMessage("SUCCESS: Connection established during polling attempt " + std::to_string(attemptCount));
                }
                // ConnectionStatusChanged callback will be called by Connect()
                _pollingActive.store(false);
                return;
            }

            // Log failed attempt
            if (LogMessage)
            {
                LogMessage("  Attempt " + std::to_string(attemptCount) + " failed, waiting 5 seconds before retry...");
            }

            // Wait before next attempt, checking if polling was stopped every 100ms
            for (int i = 0; i < RETRY_INTERVAL_MS / 100 && _pollingActive.load(); i++)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }

        // Max attempts reached or polling was stopped
        if (LogMessage)
        {
            if (attemptCount >= MAX_ATTEMPTS)
            {
                LogMessage("Connection polling stopped: maximum " + std::to_string(MAX_ATTEMPTS) + " attempts reached without success");
                LogMessage("Setting network state to Disconnected_PausedWaitingForUser (state 3)");
            }
            else
            {
                LogMessage("Connection polling was stopped");
            }
        }

        // Set state to 3 (PausedWaitingForUser) if max attempts reached
        if (attemptCount >= MAX_ATTEMPTS)
        {
            SetNetworkState(NetworkConnectionState::Disconnected_PausedWaitingForUser);
        }

        _pollingActive.store(false);
    }

    // Get network connection state
    NetworkConnectionState NetworkClient::GetNetworkState() const
    {
        return _networkState;
    }

    // Set network connection state
    void NetworkClient::SetNetworkState(NetworkConnectionState newState)
    {
        _networkState = newState;
        
        if (LogMessage)
        {
            std::string stateStr;
            switch (newState)
            {
            case NetworkConnectionState::Disconnected_ReadyToConnect:
                stateStr = "Disconnected_ReadyToConnect";
                break;
            case NetworkConnectionState::Connected_StayAlive:
                stateStr = "Connected_StayAlive";
                break;
            case NetworkConnectionState::ConnectionIssue_Terminate:
                stateStr = "ConnectionIssue_Terminate";
                break;
            case NetworkConnectionState::Disconnected_PausedWaitingForUser:
                stateStr = "Disconnected_PausedWaitingForUser";
                break;
            default:
                stateStr = "Unknown";
            }
            LogMessage("[STATE MACHINE] Network state changed to: " + stateStr);
        }
    }

    // Get handshake complete state
    bool NetworkClient::IsHandshakeComplete() const
    {
        return _handshakeComplete;
    }

    // Send HelloRequest to initiate protocol handshake
    void NetworkClient::SendHelloRequest()
    {
        std::string id = std::to_string(std::chrono::system_clock::now().time_since_epoch().count() % 1000000);
        std::ostringstream ss;
        ss << "<WWKS Version=\"2.0\" TimeStamp=\"";
        auto now = std::chrono::system_clock::now();
        std::time_t t = std::chrono::system_clock::to_time_t(now);
        std::tm tm{};
        gmtime_s(&tm, &t);
        char buf[64];
        strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tm);
        ss << buf << "\">";
        ss << "<HelloRequest Id=\"" << id << "\"";
        

        
        ss << ">";
        ss << "<Subscriber Id=\"" << SharedVariables::SourceNumber << "\" Type=\"IMS\" Manufacturer=\"Becton Dickenson Netherlands\" ProductInfo=\"RowaPickupSlim\" VersionInfo=\"1.0\"";
        // Add TenantId if provided (optional for multi-tenant systems)
        if (!SharedVariables::TenantId.empty())
        {
            ss << " TenantId=\"" << SharedVariables::TenantId << "\"";
        }
        ss << ">";
        ss << "<Capability Name=\"KeepAlive\" /><Capability Name=\"Status\" />";
        ss << "<Capability Name=\"StockInfo\" /><Capability Name=\"Output\" />";
        ss << "<Capability Name=\"TaskInfo\" />";
        ss << "</Subscriber>";
        ss << "</HelloRequest>";
        ss << "</WWKS>";
        
        if (LogMessage)
        {
            LogMessage("[HANDSHAKE] Sending HelloRequest");
        }
        SendMessage(ss.str());
    }

    // Send StatusRequest as part of handshake
    void NetworkClient::SendStatusRequest()
    {
        std::string id = std::to_string(std::chrono::system_clock::now().time_since_epoch().count() % 1000000);
        std::ostringstream ss;
        ss << "<WWKS Version=\"2.0\" TimeStamp=\"";
        auto now = std::chrono::system_clock::now();
        std::time_t t = std::chrono::system_clock::to_time_t(now);
        std::tm tm{};
        gmtime_s(&tm, &t);
        char buf[64];
        strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tm);
        ss << buf << "\">";
        ss << "<StatusRequest Id=\"" << id << "\" Source=\"" << SharedVariables::SourceNumber << "\" IncludeDetails=\"True\" />";
        ss << "</WWKS>";
        
        if (LogMessage)
        {
            LogMessage("[HANDSHAKE] Sending StatusRequest");
        }
        SendMessage(ss.str());
    }

    // Send StockInfoRequest as final handshake step
    void NetworkClient::SendStockInfoRequest()
    {
        std::string id = std::to_string(std::chrono::system_clock::now().time_since_epoch().count() % 1000000);
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
        
        if (LogMessage)
        {
            LogMessage("[HANDSHAKE] Sending StockInfoRequest");
        }
        SendMessage(ss.str());
    }

} // namespace RowaPickupSlim
