# Contributing to RowaPickupSlim

Thank you for your interest in contributing! This document outlines guidelines and processes.

## Code of Conduct

- Be respectful and professional
- Assume good intent
- Keep discussions focused on technical merit

## Getting Started

1. **Fork** the repository
2. **Clone** your fork locally
3. **Create a feature branch**: `git checkout -b feature/your-feature-name`
4. **Build locally** to ensure no compilation errors

## Development Setup

### Requirements
- **Visual Studio 2026** (v145 Platform Toolset)
- **Windows SDK 10.0.26100+** (Windows 11 SDK or compatible)
- **C++17 or later** language standard
- Git

### Build
```bash
# Open RowaPickupSlim.sln in Visual Studio 2026
# Select x64 Release configuration
# Build → Build Solution
# Output: x64/Release/RowaPickupSlim.exe
```

### Platform Requirements
- **Minimum Visual Studio**: VS 2026 (v145)
- **Minimum Windows SDK**: 10.0.26100
- **Minimum C++ Standard**: C++17
- **Target Platform**: Windows 10 22H2 or later

## Code Style

### Naming Conventions
- **Classes/Structs**: `PascalCase` (e.g., `NetworkClient`, `AppState`)
- **Functions**: `snake_case` (e.g., `perform_search_filter()`)
- **Variables**: `snake_case` with `g_` prefix for globals (e.g., `g_state`, `g_client`)
- **Constants**: `UPPER_SNAKE_CASE` for macros, `PascalCase` for enum values

### File Organization
- **Headers (.h)**: Contain declarations only, no implementations
- **main.cpp**: Contains all implementations, organized by module sections:
  ```cpp
  // ========== ModuleName ==========
  // Group related functions together
  ```
- **Implementation files (.cpp)**: For standalone modules (networkclient, etc.)

### Comments
- Keep comments minimal – code should be self-explanatory
- Use comments for "why", not "what"
- Mark module sections clearly:
  ```cpp
  // ========== ArticleManagement ==========
  // Handles stock list operations
  ```

### Threading & Synchronization
- Always protect shared state with `std::lock_guard<std::mutex>`
- Never call blocking operations while holding a lock
- Document which mutex protects which state

## Making Changes

### Before You Start
1. **Check for existing issues** – don't duplicate work
2. **Discuss major changes** – open an issue to get feedback first
3. **Keep PRs focused** – one feature per PR, not multiple

### Commit Messages
Use clear, descriptive messages:
```
feat: Add language switching via WM_APP_LANGUAGE_CHANGED

- Implement message handler for language updates
- Update UI strings in real-time
- Refresh menu and button text

Fixes #42
```

### PR Description
Include:
- **What** – What does this change do?
- **Why** – Why is it needed?
- **How** – How does it work?
- **Testing** – How was it tested?

## Testing

### Manual Testing
1. Build in Debug mode
2. Test with local WWKS server (or mock)
3. Verify UI responsiveness
4. Check logs in `C:\ProgramData\RowaPickup\Protocol\`

### Protocol Compliance
- Ensure WWKS 2.0 messages are correctly formatted
- Verify handshake sequence (Hello → Status → StockInfo)
- Test KeepAlive responses

## Submitting a PR

1. **Push** your feature branch to your fork
2. **Open a Pull Request** against `main` branch
3. **Fill in the PR template** completely
4. **Respond** to review feedback promptly
5. **Rebase** if there are merge conflicts

### Review Process
- At least **1 maintainer approval** required
- **All CI checks** must pass (build succeeds on Windows)
- **No merge conflicts** allowed

## Areas for Contribution

### High Priority
- [ ] Performance optimizations (scrolling, rendering)
- [ ] Additional language translations
- [ ] Error handling improvements
- [ ] Unit test framework

### Medium Priority
- [ ] UI theme/skinning system
- [ ] Advanced filtering options
- [ ] Statistics/reporting dashboard
- [ ] Multi-warehouse support

### Low Priority
- [ ] Documentation improvements
- [ ] Code cleanup
- [ ] Example configurations

## License

By contributing, you agree that your contributions will be licensed under the **GNU General Public License v3.0**. See [LICENSE](../LICENSE).

## Acknowledgments

This project was created and is maintained by **Vincent Voois**, who originally developed the MAUI C# version and ported it to a more native Windows C++.

## Questions?

- **Bugs**: Open an issue with reproduction steps
- **Features**: Open an issue and tag `enhancement`
- **Questions**: Use GitHub Discussions

---

**Thank you for contributing!** 🎉
