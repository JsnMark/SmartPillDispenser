# Development Instructions

### See Everything 

- Open up VSCode
- File > Open Workspace from File > double click on the "project.code-workspace" file
- You will see the root directory, and the specified folders on the Explorer sidebar. For example /firmware appears in both the root directory and on its own. This is so that the PlatformIO extension works without only opening the firmware folder via PlatformIO.
- Install the recommended extensions when VS Code prompts you (PlatformIO for `firmware`; Qt, CMake Tools, clangd and C/C++ for `qtApplication`).

### See what you need
- Open up VSCode
- FIle > Open Folder of either /firmware or /qtApplication

## Qt application (`qtApplication`)

### Prerequisites
- **Qt 6.8 or newer** with the Qt Quick module, installed via the [Qt Online Installer](https://www.qt.io/download-qt-installer). The installer also provides CMake and Ninja.
- A C++ compiler: Xcode Command Line Tools on macOS (`xcode-select --install`), MSVC or MinGW on Windows, GCC/Clang on Linux.

### First-time setup in VS Code
1. Command Palette → **Qt: Register Qt Installation** → select your Qt install folder (e.g. `~/Qt` or `C:\Qt`).
2. In the CMake status bar, make sure the active project is **qtApplication**.
3. Command Palette → **CMake: Select a Kit** → choose the Qt kit (e.g. `Qt-6.x.x-macos-...`). Don't pick a plain compiler kit; it won't know where Qt is.
4. Run **CMake: Configure** (or just build once). This generates `build/Debug/compile_commands.json`, which IntelliSense needs.

### IntelliSense (clangd)
IntelliSense for `qtApplication` comes from the **clangd** extension, not the Microsoft C/C++ extension (its IntelliSense is turned off for this folder because it can't resolve Qt's macOS framework headers). The C/C++ extension is still required for debugging.
- **macOS:** clangd ships with the Xcode Command Line Tools; the extension finds it automatically.
- **Windows/Linux:** when the clangd extension asks to download clangd, click **Install**.
- clangd reads `build/Debug/compile_commands.json` (configured in `qtApplication/.clangd`), so configure a **Debug** build at least once.
- Open C++ files through the **qtApplication** folder in the Explorer.

### Build & run
- **Build:** F7 (or the Build button in the status bar). Output goes to `qtApplication/build/<BuildType>/`.
- **Run:** Shift+F5, or the ▶ button in the status bar.
- **Debug:** Run and Debug panel → "Debug Qt Application with cppdbg" (or "C++/QML" to also debug QML) → F5.

### Windows notes
- **Clone to a short path** without spaces or parentheses, e.g. `C:\dev\SmartPillDispenser`. Qt's QML build creates deeply nested files and can hit Windows' 260-character path limit. Avoid OneDrive-synced folders like `Documents`.
- **Pick a compiler in the Qt installer:**
  - **MinGW** (easiest): the installer includes the compiler. Select the Qt MinGW kit in VS Code. Debug with "Debug Qt Application with cppdbg" or "C++/QML".
  - **MSVC 2022**: also install Visual Studio 2022 or the VS Build Tools with the "Desktop development with C++" workload. Select the Qt MSVC kit in VS Code. Debug with "Debug Qt Application with cppvsdbg (Windows MSVC)" or "C++/QML (Windows MSVC)".
  - The kit must match the Qt build you installed (MinGW Qt ↔ MinGW kit, MSVC Qt ↔ MSVC kit).
- **"Qt6Core.dll was not found" when running:** Windows can't find the Qt DLLs. Launch through one of the debug configurations above (they add Qt to `PATH`), or add `C:\Qt\6.x.x\<compiler>\bin` to your `PATH`.

### Troubleshooting
- **Red squiggles on Qt includes:**
  - Check the error's source. `clang(...)` → run **CMake: Configure** (Debug), then **clangd: Restart language server**. `C/C++(1696)` → the Microsoft parser is still active; run **Developer: Reload Window** and reopen the file from the **qtApplication** folder.
  - MinGW users seeing errors on standard headers (e.g. `<type_traits>`): make sure your MinGW `g++` matches the `--query-driver` pattern in `project.code-workspace`, then restart clangd.
- **Strange configure/build errors after switching kits:** **CMake: Delete Cache and Reconfigure**.
- Avoid spaces and parentheses in the path to the project; some Qt-generated build steps don't handle them.
