#include <iostream>
#include <cstdlib>
#include <csignal>
#include "ClickEngine.h"

static volatile bool keep_running = true;

void signal_handler(int) {
    keep_running = false;
}

int main(int argc, char** argv) {
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);
    
    try {
        auto& engine = ClickEngine::getInstance();
        
        std::cout << "AutoClicker v3.0 - CounterLib Edition" << std::endl;
        std::cout << "=====================================" << std::endl;
        std::cout << "Commands:" << std::endl;
        std::cout << "  s - Start" << std::endl;
        std::cout << "  t - Stop" << std::endl;
        std::cout << "  d [ms] - Set delay" << std::endl;
        std::cout << "  q - Quit" << std::endl;
        std::cout << std::endl;
        
        std::string cmd;
        while (keep_running) {
            std::cout << "> ";
            std::getline(std::cin, cmd);
            
            if (cmd == "s") {
                engine.start();
                std::cout << "Clicking started" << std::endl;
            } else if (cmd == "t") {
                engine.stop();
                std::cout << "Clicking stopped. Total clicks: " << engine.getClickCount() << std::endl;
            } else if (cmd[0] == 'd' && cmd.size() > 2) {
                int delay = std::stoi(cmd.substr(2));
                engine.setDelay(delay);
                std::cout << "Delay set to " << delay << "ms" << std::endl;
            } else if (cmd == "q") {
                engine.stop();
                break;
            } else if (cmd == "c") {
                std::cout << "Clicks: " << engine.getClickCount() << std::endl;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
