# Win32-KeyState-Monitor
A Windows POC that demonstrates global keyboard input capture using the Win32 API `GetAsyncKeyState`, without relying on traditional low-level keyboard hooks (`SetWindowsHookEx`).

This project is intended for **educational and research purposes** to explore how keyboard state polling works at a low level in Windows.

---

## Overview

Unlike classic keyloggers that install a keyboard hook (`WH_KEYBOARD_LL`), this project continuously polls key states using `GetAsyncKeyState` from a dedicated thread.  
It tracks:

- Printable characters
- Modifier keys (Ctrl, Alt, Shift, Caps Lock)
- Special keys (Enter, Esc, arrows, function keys, etc.)
- Key combinations (e.g. `Ctrl+C`, `Alt+Tab`)

Input is captured globally, meaning keystrokes are detected even when the console window is not focused.

---

## How It Works

- Uses `GetAsyncKeyState` to poll virtual key codes
- Maintains internal key state tracking to detect key press events
- Handles shift/caps logic to properly resolve characters
- Runs on a separate thread to avoid blocking the main loop
- Outputs captured input directly to the console

No drivers, DLL injection, or kernel components are used.

---

## Build & Run

### Requirements
- Windows
- Visual Studio (MSVC)
- C++17 or newer

### Build
1. Create a new **Console Application** in Visual Studio
2. Add the source file to the project
3. Build in **Release** or **Debug**
4. Run the executable

---

## Usage

Once running:
- Type anywhere on the system
- Captured input will be printed to the console
- Press **Ctrl + C** to stop the program

---

## Disclaimer

This project is provided **strictly for educational purposes**.

Do **not** use this code:
- To violate privacy
- For malicious activity
- On systems you do not own or have explicit permission to test

The author assumes no responsibility for misuse.

## License

Make sure to read the license before use.
