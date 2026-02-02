# GitHub Release Checklist ?

## Files Created for Release

? **LICENSE** – GNU General Public License v3.0  
? **README.md** – Comprehensive project documentation  
? **.gitignore** – Exclude build artifacts & config  
? **CONTRIBUTING.md** – Contributor guidelines  
? **.github/workflows/build.yml** – CI/CD pipeline  

## Pre-Release Steps

Before pushing to GitHub:

### 1. **Clean Build**
```bash
# In Visual Studio
Build ? Clean Solution
Build ? Build Solution
```
Verify no errors in `x64/Release/`.

### 2. **Verify Project Files**
- [ ] `RowaPickupSlim.sln` loads without issues
- [ ] `RowaPickupSlim.vcxproj` references all source files
- [ ] `RowaPickupSlim.vcxproj.filters` organized logically

### 3. **Check Source Files**
- [ ] All `.cpp` and `.h` files are present
- [ ] No `.obj`, `.exe`, or build artifacts in repo
- [ ] `networkclient_fixed.cpp` vs `networkclient.cpp` – keep only working version
- [ ] `pugixml.cpp` included (or referenced correctly)

### 4. **Documentation**
- [ ] README.md is up-to-date
- [ ] Architecture documented
- [ ] Build instructions clear
- [ ] License headers in source files (optional but good practice)

### 5. **Initialize Git Repository**
```bash
# If not already git repo
git init
git add .
git commit -m "Initial commit: RowaPickupSlim C++ WWKS client"
```

## Recommended GitHub Repository Setup

### About Section
```
Lightweight Windows C++ WWKS client for pharmacy warehouse operations.
Replaces MAUI cross-platform app with native GDI UI.
Communicates with Baxter Rowa robot systems.
License: GNU GPL v3.0
Language: C++ (C++17)
```

### Topics
- `wwks`
- `warehouse-automation`
- `windows-desktop`
- `cpp17`
- `gdi`
- `pharmacy`

### Release Setup
1. **Create Release** on GitHub
2. **Tag**: `v1.0.0`
3. **Title**: `Initial Release - WWKS Client`
4. **Description**:
```markdown
## RowaPickupSlim v1.0.0

First public release of RowaPickupSlim C++ WWKS client.

### Features
- WWKS 2.0 protocol support
- Real-time stock visualization
- Auto-reconnection with backoff
- Multi-language support (EN/NL)
- Audit logging

### Downloads
- [Windows x64 Release](../../releases/download/v1.0.0/RowaPickupSlim-x64-Release.exe)

### Building from Source
See [README.md](README.md) for build instructions.

### License
GNU General Public License v3.0
```

## File Structure on GitHub
```
RowaPickupSlim/
??? LICENSE                          (GNU GPL v3.0)
??? README.md                        (Project documentation)
??? CONTRIBUTING.md                  (Contributor guide)
??? .gitignore                       (Git ignore rules)
??? .github/
?   ??? workflows/
?       ??? build.yml                (CI pipeline)
??? RowaPickupSlim/                  (Visual Studio project)
    ??? RowaPickupSlim.sln
    ??? RowaPickupSlim.vcxproj
    ??? RowaPickupSlim.vcxproj.filters
    ??? main.cpp
    ??? networkclient.h/cpp
    ??? SettingsDialog.h/cpp
    ??? SharedVariables.h/cpp
    ??? Localization.h/cpp
    ??? UIHelpers.h/cpp
    ??? LoggingSystem.h/cpp
    ??? XmlDefinitions.h/cpp
    ??? ArticleManagement.h/cpp
    ??? OutputManagement.h/cpp
    ??? DeviceManagement.h/cpp
    ??? Shared.h
    ??? RowaPickupSlim.rc
    ??? Resource.h
    ??? pugixml.cpp/hpp
    ??? locale/
    ?   ??? English.lng
    ?   ??? Dutch.lng
    ??? RowaPickupMAUI/               (Reference C# source)
        ??? *.cs files
```

## Final Checklist Before Push

- [ ] All source files compile cleanly
- [ ] No binary/build artifacts in `.git` history
- [ ] LICENSE file present and correct
- [ ] README.md complete with setup instructions
- [ ] .gitignore configured to exclude build outputs
- [ ] CONTRIBUTING.md guidelines clear
- [ ] CI workflow will trigger on push
- [ ] No proprietary or sensitive data in repo
- [ ] All dependencies are source-included or documented
- [ ] Version number documented (suggest `1.0.0`)

## Publishing Steps

1. **Create GitHub Repository**
   - Go to https://github.com/new
   - Name: `RowaPickupSlim`
   - Description: "Lightweight Windows C++ WWKS client for warehouse operations"
   - License: `GNU General Public License v3.0` (auto-populates with LICENSE file)
   - Visibility: **Public**

2. **Add Remote & Push**
   ```bash
   git remote add origin https://github.com/YOUR_USERNAME/RowaPickupSlim.git
   git branch -M main
   git push -u origin main
   ```

3. **Enable GitHub Features**
   - ? Enable Discussions (for questions)
   - ? Enable Issues (for bug reports)
   - ? Add Topics (wwks, warehouse-automation, etc.)
   - ? Setup Branch Protection (require PR reviews)

4. **Create First Release**
   - Go to Releases tab
   - Click "Create a new release"
   - Tag: `v1.0.0`
   - Generate release notes from PR/commit history

## Future Maintenance

### For Each Update
```bash
git add .
git commit -m "feat: Description of change"
git tag -a v1.0.1 -m "Release v1.0.1"
git push origin main --tags
```

### Monitor
- Watch GitHub Issues for bug reports
- Respond to Discussions
- Keep dependencies updated (pugixml)
- Maintain CI/CD pipeline

---

**You're ready to release!** ??

All documentation and setup files have been created. You can now push to GitHub with confidence.
