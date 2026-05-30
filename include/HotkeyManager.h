#ifndef HOTKEY_MANAGER_H
#define HOTKEY_MANAGER_H

#include <X11/X.h>
#include <X11/Xlib.h>
#include <functional>
#include <unordered_map>
#include <string>
#include <atomic>
#include <thread>

struct Hotkey {
    KeyCode keycode;
    unsigned int modifiers;
    std::string name;
    std::function<void()> callback;
};

class HotkeyManager {
public:
    static HotkeyManager& getInstance();
    
    void registerHotkey(const std::string& name, const std::string& keysym, std::function<void()> callback);
    void unregisterHotkey(const std::string& name);
    void start();
    void stop();
    void setRecordingMode(bool enabled);
    bool isRecording() const { return recording; }
    std::string getLastRecordedKey() const { return last_recorded; }
    
private:
    HotkeyManager();
    ~HotkeyManager();
    void listenerLoop();
    KeyCode stringToKeycode(const std::string& keysym);
    std::string keycodeToString(KeyCode keycode);
    unsigned int parseModifiers(const std::string& mod_str);
    
    Display* display;
    Window root;
    std::atomic<bool> running;
    std::atomic<bool> recording;
    std::thread listener_thread;
    std::unordered_map<std::string, Hotkey> hotkeys;
    std::string last_recorded;
};

#endif
