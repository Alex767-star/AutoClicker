#include <iostream>
#include <cstdlib>
#include <csignal>
#include <string>
#include <thread>
#include <chrono>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include "ClickEngine.h"

static volatile bool keep_running = true;

void signal_handler(int) {
    keep_running = false;
}

bool kbhit() {
    struct timeval tv = { 0L, 0L };
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(0, &fds);
    return select(1, &fds, NULL, NULL, &tv) > 0;
}

void print_banner() {
    std::cout << "\033[32m";
    std::cout << "╔══════════════════════════════════════════════════════════╗\n";
    std::cout << "║      AutoClicker PRO v3.0 - Human-like Edition          ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════╝\n";
    std::cout << "\033[0m\n";
}

void print_help() {
    std::cout << "\033[36mCommands:\033[0m\n";
    std::cout << "  \033[33ms\033[0m           - Start clicking\n";
    std::cout << "  \033[33mt\033[0m           - Stop clicking\n";
    std::cout << "  \033[33md [1-1000]\033[0m  - Set base delay (ms)\n";
    std::cout << "  \033[33mm [1-7]\033[0m     - Mode: 1=single 2=double 3=triple 4=hold 5=random 6=burst 7=human\n";
    std::cout << "  \033[33mb [count]\033[0m   - Set burst count\n";
    std::cout << "  \033[33mv [ms]\033[0m      - Set human variance (randomness)\n";
    std::cout << "  \033[33mj [pixels]\033[0m  - Set human cursor jitter\n";
    std::cout << "  \033[33mc\033[0m           - Show click count\n";
    std::cout << "  \033[33mr\033[0m           - Reset counter\n";
    std::cout << "  \033[33mq\033[0m           - Quit\n";
    std::cout << "\n\033[36mHotkeys:\033[0m F6=Start F7=Stop F8=Toggle F9=SavePos ESC=Exit\n";
    std::cout << "\n\033[36mCurrent delay: \033[33m15ms\033[0m \033[36m(~67 CPS)\033[0m\n";
    std::cout << "\033[36mHuman mode: \033[33m8-25ms\033[0m \033[36m+ jitter\033[0m\n\033[0m\n";
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);
    
    try {
        auto& engine = ClickEngine::getInstance();
        engine.startHotkeyThread();
        
        print_banner();
        print_help();
        
        std::cout << "\033[32mReady! Press F6 to start (Human-like mode by default)\033[0m\n\n> " << std::flush;
        
        std::string line;
        while (keep_running) {
            if (kbhit()) {
                std::getline(std::cin, line);
                
                if (line == "s") {
                    engine.start();
                    std::cout << "\033[32m▶ Started\033[0m\n> " << std::flush;
                } else if (line == "t") {
                    engine.stop();
                    std::cout << "\033[31m■ Stopped. Clicks: " << engine.getClickCount() << "\033[0m\n> " << std::flush;
                } else if (line == "c") {
                    std::cout << "\033[36mClicks: " << engine.getClickCount() << "\033[0m\n> " << std::flush;
                } else if (line == "r") {
                    engine.resetClickCount();
                    std::cout << "\033[36mReset\033[0m\n> " << std::flush;
                } else if (line == "q") {
                    engine.stop();
                    break;
                } else if (line.rfind("d ", 0) == 0) {
                    try {
                        int delay = std::stoi(line.substr(2));
                        if (delay >= 1 && delay <= 1000) {
                            engine.setDelay(delay);
                            std::cout << "\033[36mDelay: " << delay << "ms (" << 1000/delay << " CPS)\033[0m\n> " << std::flush;
                        } else {
                            std::cout << "\033[31mDelay must be 1-1000ms\033[0m\n> " << std::flush;
                        }
                    } catch(...) {
                        std::cout << "\033[31mInvalid\033[0m\n> " << std::flush;
                    }
                } else if (line.rfind("m ", 0) == 0) {
                    try {
                        int mode = std::stoi(line.substr(2));
                        if (mode >= 1 && mode <= 7) {
                            engine.setMode(static_cast<ClickMode>(mode - 1));
                            const char* modes[] = {"Single", "Double", "Triple", "Hold", "Random", "Burst", "Human"};
                            std::cout << "\033[36mMode: " << modes[mode-1] << "\033[0m\n> " << std::flush;
                        }
                    } catch(...) {}
                } else if (line.rfind("b ", 0) == 0) {
                    try {
                        int count = std::stoi(line.substr(2));
                        engine.setBurstCount(count);
                        std::cout << "\033[36mBurst: " << count << "\033[0m\n> " << std::flush;
                    } catch(...) {}
                } else if (line.rfind("v ", 0) == 0) {
                    try {
                        int var = std::stoi(line.substr(2));
                        engine.setHumanVariance(var);
                        std::cout << "\033[36mVariance: ±" << var << "ms\033[0m\n> " << std::flush;
                    } catch(...) {}
                } else if (line.rfind("j ", 0) == 0) {
                    try {
                        int jitter = std::stoi(line.substr(2));
                        engine.setHumanJitter(jitter);
                        std::cout << "\033[36mJitter: " << jitter << "px\033[0m\n> " << std::flush;
                    } catch(...) {}
                } else if (!line.empty()) {
                    std::cout << "\033[31mUnknown. Commands: s t d N m N b N v N j N c r q\033[0m\n> " << std::flush;
                } else {
                    std::cout << "> " << std::flush;
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    std::cout << "\n\033[33mBye! Total clicks: " << ClickEngine::getInstance().getClickCount() << "\033[0m\n";
    return 0;
}
