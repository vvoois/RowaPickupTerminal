# Copilot Instructions

## General Guidelines
- First general instruction
- Second general instruction

## Code Style
- Use specific formatting rules
- Follow naming conventions

## Project-Specific Rules
- Migrate the MAUI C# project to a lightweight C++ Windows GDI app.
- Create `SharedVariables.h` and `SharedVariables.cpp` to port configuration settings.
- Load settings from `C:\ProgramData\RowaPickupMAUI\RowaPickupMaui.config` in INI format.
- Call `SettingsLoader::LoadSettings()` at app startup (WM_CREATE).
- Access all static settings via `SharedVariables::` (e.g., `SharedVariables::ClientIpAddress`, `SharedVariables::SourceNumber`).
- Include `SharedVariables.h` in files that need config access.
- Translate code from the C# project (XmlDefinitions and NetworkClient) using pugixml and Winsock.
- Place generated C++ files in the RowaPickupSlim project.
- Create `SettingsDialog.h` and `SettingsDialog.cpp` for a modeless dialog featuring 9 settings fields (IP, port, location, checkboxes, priority combo, password).
- Implement real-time password validation to enable controls when "ibib" is entered.
- Persist settings to `C:\ProgramData\RowaPickupMAUI\RowaPickupMaui.config`.
- Right-clicking the main window opens the settings dialog.
- Create all UI elements using the Windows API (L"EDIT", L"BUTTON", L"COMBOBOX") with unicode conversion via `utf8_to_wstring()`.