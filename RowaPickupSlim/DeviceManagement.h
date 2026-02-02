#pragma once
// DeviceManagement.h
// Centralized device status tracking and display management

#include <string>
#include <vector>
#include <tuple>

namespace RowaPickupSlim::DeviceManagement
{
    /// Device information tuple: (type, description, state, stateText)
    using DeviceInfo = std::tuple<std::string, std::string, std::string, std::string>;

    /// Update device status from StatusResponse message
    /// Updates both robot state and connected device list
    /// @param deviceList Vector of device info tuples
    void UpdateDeviceStatus(const std::vector<DeviceInfo>& deviceList);

    /// Get current robot state display text
    /// E.g., "(Mosaic [Gereed])" or "(ROB1 [Inactief])"
    /// @return Robot state string
    std::string GetRobotStateDisplay();

    /// Get current list of connected devices
    /// @return Vector of device information tuples
    std::vector<DeviceInfo> GetDeviceList();

    /// Format state for display (converts "Ready" to localized text, etc.)
    /// @param state The state value from server
    /// @return Formatted display text
    std::string FormatStateForDisplay(const std::string& state);

} // namespace RowaPickupSlim::DeviceManagement

