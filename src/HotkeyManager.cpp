#include "HotkeyManager.h"
#include <X11/keysym.h>
#include <X11/extensions/XTest.h>
#include <iostream>
#include <sstream>
#include <algorithm>

HotkeyManager& HotkeyManager::getInstance() {
    static HotkeyManager instance;
    return instance;
}

HotkeyManager::HotkeyManager() : running(false), recording(false) {
    display = XOpenDisplay(nullptr);
    if (!display) throw std::runtime_error("Cannot open X display for hotkeys");
    root = DefaultRootWindow(display);
}

HotkeyManager::~HotkeyManager() {
    stop();
    if (display) XCloseDisplay(display);
}

KeyCode HotkeyManager::stringToKeycode(const std::string& keysym) {
    std::string upper = keysym;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
    
    KeySym ks = XStringToKeysym(upper.c_str());
    if (ks == NoSymbol) {
        if (keysym == "F1") ks = XK_F1;
        else if (keysym == "F2") ks = XK_F2;
        else if (keysym == "F3") ks = XK_F3;
        else if (keysym == "F4") ks = XK_F4;
        else if (keysym == "F5") ks = XK_F5;
        else if (keysym == "F6") ks = XK_F6;
        else if (keysym == "F7") ks = XK_F7;
        else if (keysym == "F8") ks = XK_F8;
        else if (keysym == "F9") ks = XK_F9;
        else if (keysym == "F10") ks = XK_F10;
        else if (keysym == "F11") ks = XK_F11;
        else if (keysym == "F12") ks = XK_F12;
        else return 0;
    }
    
    return XKeysymToKeycode(display, ks);
}

std::string HotkeyManager::keycodeToString(KeyCode keycode) {
    KeySym ks = XKeycodeToKeysym(display, keycode, 0);
    if (ks == NoSymbol) return "Unknown";
    
    char* name = XKeysymToString(ks);
    if (name) return std::string(name);
    return "Unknown";
}

unsigned int HotkeyManager::parseModifiers(const std::string& mod_str) {
    unsigned int mods = 0;
    std::string str = mod_str;
    std::transform(str.begin(), str.end(), str.begin(), ::toupper);
    
    if (str.find("CTRL") != std::string::npos || str.find("CONTROL") != std::string::npos)
        mods |= ControlMask;
    if (str.find("ALT") != std::string::npos)
        mods |= Mod1Mask;
    if (str.find("SHIFT") != std::string::npos)
        mods |= ShiftMask;
    if (str.find("SUPER") != std::string::npos || str.find("WIN") != std::string::npos)
        mods |= Mod4Mask;
    
    return mods;
}

void HotkeyManager::registerHotkey(const std::string& name, const std::string& hotkey_str, 
                                    std::function<void()> callback) {
    std::string keyname = hotkey_str;
    unsigned int modifiers = 0;
    
    size_t plus_pos = hotkey_str.find('+');
    if (plus_pos != std::string::npos) {
        modifiers = parseModifiers(hotkey_str.substr(0, plus_pos));
        keyname = hotkey_str.substr(plus_pos + 1);
    }
    
    KeyCode keycode = stringToKeycode(keyname);
    if (keycode == 0) {
        std::cerr << "Unknown key: " << keyname << std::endl;
        return;
    }
    
    Hotkey hk;
    hk.keycode = keycode;
    hk.modifiers = modifiers;
    hk.name = name;
    hk.callback = callback;
    
    hotkeys[name] = hk;
    
    XGrabKey(display, keycode, modifiers, root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(display, keycode, modifiers | Mod2Mask, root, True, GrabModeAsync, GrabModeAsync);
    XSync(display, False);
    
    std::cout << "[Hotkey] Registered: " << name << " -> " << hotkey_str << std::endl;
}

void HotkeyManager::unregisterHotkey(const std::string& name) {
    auto it = hotkeys.find(name);
    if (it != hotkeys.end()) {
        XUngrabKey(display, it->second.keycode, it->second.modifiers, root);
        XUngrabKey(display, it->second.keycode, it->second.modifiers | Mod2Mask, root);
        hotkeys.erase(it);
    }
}

void HotkeyManager::start() {
    if (running) return;
    running = true;
    listener_thread = std::thread(&HotkeyManager::listenerLoop, this);
}

void HotkeyManager::stop() {
    running = false;
    if (listener_thread.joinable()) listener_thread.join();
}

void HotkeyManager::setRecordingMode(bool enabled) {
    recording = enabled;
    if (!enabled) last_recorded.clear();
    std::cout << "[Hotkey] Recording mode: " << (enabled ? "ON" : "OFF") << std::endl;
}

void HotkeyManager::listenerLoop() {
    XSelectInput(display, root, KeyPressMask);
    
    while (running) {
        XEvent ev;
        XNextEvent(display, &ev);
        
        if (ev.type == KeyPress) {
            KeyCode keycode = ev.xkey.keycode;
            unsigned int modifiers = ev.xkey.state & (ControlMask | ShiftMask | Mod1Mask | Mod4Mask);
            
            if (recording) {
                std::string mod_str;
                if (modifiers & ControlMask) mod_str += "Ctrl+";
                if (modifiers & ShiftMask) mod_str += "Shift+";
                if (modifiers & Mod1Mask) mod_str += "Alt+";
                if (modifiers & Mod4Mask) mod_str += "Super+";
                
                last_recorded = mod_str + keycodeToString(keycode);
                std::cout << "[Hotkey] Recorded: " << last_recorded << std::endl;
                recording = false;
                continue;
            }
            
            for (auto& [name, hk] : hotkeys) {
                if (hk.keycode == keycode && (hk.modifiers == 0 || (modifiers & hk.modifiers) == hk.modifiers)) {
                    if (hk.callback) {
                        hk.callback();
                    }
                    break;
                }
            }
        }
    }
}
