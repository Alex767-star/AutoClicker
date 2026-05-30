#include <iostream>
#include <cstdlib>
#include <csignal>
#include <string>
#include <thread>
#include <chrono>
#include "ClickEngine.h"
#include "HotkeyManager.h"

static volatile bool keep_running = true;

void signal_handler(int) {
    keep_running = false;
}

void print_banner() {
    std::cout << "\033[32m";
    std::cout << "╔══════════════════════════════════════════════════════════╗\n";
    std::cout << "║           AutoClicker PRO v3.0 - Hotkey Edition         ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════╝\n";
    std::cout << "\033[0m\n";
}

void print_help() {
    std::cout << "Commands:\n";
    std::cout << "  s           - Start clicking\n";
    std::cout << "  t           - Stop clicking\n";
    std::cout << "  d [ms]      - Set delay (default: 100ms)\n";
    std::cout << "  m [mode]    - Set mode: single/double/triple/hold/random/burst\n";
    std::cout << "  b [count]   - Set burst count\n";
    std::cout << "  hk start    - Set start hotkey\n";
    std::cout << "  hk stop     - Set stop hotkey\n";
    std::cout << "  hk toggle   - Set toggle hotkey\n";
    std::cout << "  hk record   - Record any key as hotkey\n";
    std::cout << "  hk list     - Show current hotkeys\n";
    std::cout << "  c           - Show click count\n";
    std::cout << "  q           - Quit\n";
    std::cout << "\n";
}

void print_hotkeys() {
    std::cout << "\nCurrent Hotkeys:\n";
    std::cout << "  Start:  " << ClickEngine::getInstance().isRunning() << "\n";
    std::cout << "  Stop:   \n";
    std::cout << "  Toggle: \n";
    std::cout << "\n";
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);
    
    try {
        auto& engine = ClickEngine::getInstance();
        auto& hotkeyManager = HotkeyManager::getInstance();
        
        hotkeyManager.registerHotkey("start", "F6", [&engine]() {
            engine.start();
            std::cout << "\n[Hotkey] Clicking STARTED" << std::endl;
        });
        
        hotkeyManager.registerHotkey("stop", "F7", [&engine]() {
            engine.stop();
            std::cout << "\n[Hotkey] Clicking STOPPED. Total clicks: " << engine.getClickCount() << std::endl;
        });
        
        hotkeyManager.registerHotkey("toggle", "F8", [&engine]() {
            engine.toggle();
            std::cout << "\n[Hotkey] Clicking " << (engine.isRunning() ? "STARTED" : "STOPPED") << std::endl;
        });
        
        hotkeyManager.start();
        
        print_banner();
        print_help();
        
        std::string cmd;
        while (keep_running) {
            std::cout << "\033[33m>\033[0m ";
            if (!std::getline(std::cin, cmd)) break;
            
            if (cmd == "s") {
                engine.start();
                std::cout << "\033[32mClicking started\033[0m" << std::endl;
            } else if (cmd == "t") {
                engine.stop();
                std::cout << "\033[31mClicking stopped. Total clicks: " << engine.getClickCount() << "\033[0m" << std::endl;
            } else if (cmd.rfind("d ", 0) == 0) {
                try {
                    int delay = std::stoi(cmd.substr(2));
                    engine.setDelay(delay);
                    std::cout << "Delay set to " << delay << "ms" << std::endl;
                } catch (...) {
                    std::cout << "Invalid delay" << std::endl;
                }
            } else if (cmd.rfind("hk ", 0) == 0) {
                std::string sub = cmd.substr(3);
                if (sub == "record") {
                    std::cout << "Press any key combination... (5 seconds)" << std::endl;
                    hotkeyManager.setRecordingMode(true);
                    
                    for (int i = 0; i < 5 && hotkeyManager.isRecording(); i++) {
                        std::this_thread::sleep_for(std::chrono::seconds(1));
                        std::cout << "." << std::flush;
                    }
                    
                    if (hotkeyManager.isRecording()) {
                        hotkeyManager.setRecordingMode(false);
                        std::cout << "\nTimeout" << std::endl;
                    } else {
                        std::cout << "\nRecorded: " << hotkeyManager.getLastRecordedKey() << std::endl;
                    }
                } else if (sub.rfind("start ", 0) == 0) {
                    std::string key = sub.substr(6);
                    hotkeyManager.registerHotkey("start", key, [&engine]() {
                        engine.start();
                        std::cout << "\n[Hotkey] Clicking STARTED" << std::endl;
                    });
                    std::cout << "Start hotkey set to: " << key << std::endl;
                } else if (sub.rfind("stop ", 0) == 0) {
                    std::string key = sub.substr(5);
                    hotkeyManager.registerHotkey("stop", key, [&engine]() {
                        engine.stop();
                        std::cout << "\n[Hotkey] Clicking STOPPED. Total clicks: " << engine.getClickCount() << std::endl;
                    });
                    std::cout << "Stop hotkey set to: " << key << std::endl;
                } else if (sub.rfind("toggle ", 0) == 0) {
                    std::string key = sub.substr(7);
                    hotkeyManager.registerHotkey("toggle", key, [&engine]() {
                        engine.toggle();
                        std::cout << "\n[Hotkey] Clicking " << (engine.isRunning() ? "STARTED" : "STOPPED") << std::endl;
                    });
                    std::cout << "Toggle hotkey set to: " << key << std::endl;
                } else if (sub == "list") {
                    std::cout << "\nRegistered hotkeys:" << std::endl;
                    std::cout << "  Start:  F6 (default)" << std::endl;
                    std::cout << "  Stop:   F7 (default)" << std::endl;
                    std::cout << "  Toggle: F8 (default)" << std::endl;
                } else {
                    std::cout << "Usage: hk <start|stop|toggle|record|list> [key]" << std::endl;
                }
            } else if (cmd == "c") {
                std::cout << "Clicks: " << engine.getClickCount() << std::endl;
            } else if (cmd == "q") {
                engine.stop();
                break;
            } else if (cmd == "help") {
                print_help();
            } else if (!cmd.empty()) {
                std::cout << "Unknown command. Type 'help'" << std::endl;
            }
        }
        
        hotkeyManager.stop();
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
