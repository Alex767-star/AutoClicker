#include <windows.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <random>
#include <string>
#include <cmath>
#include <conio.h>

enum ClickMode {
    MODE_SINGLE = 0,
    MODE_DOUBLE = 1,
    MODE_TRIPLE = 2,
    MODE_HOLD = 3,
    MODE_RANDOM = 4,
    MODE_BURST = 5,
    MODE_HUMAN = 6
};

class AutoClicker {
private:
    std::atomic<bool> active;
    std::atomic<bool> running;
    std::atomic<int> delay_ms;
    std::atomic<int> button;
    std::atomic<ClickMode> click_mode;
    std::atomic<int> burst_count;
    std::atomic<long long> click_count;
    std::atomic<int> human_variance;
    std::atomic<int> human_jitter;
    
    std::mt19937 rng;
    std::normal_distribution<> human_dist;
    int human_min_delay, human_max_delay;
    
    void updateHumanParams() {
        int base = delay_ms.load();
        int var = human_variance.load();
        human_min_delay = (base > var) ? base - var : 1;
        human_max_delay = base + var;
        human_dist = std::normal_distribution<>((double)base, (double)var / 3.0);
    }
    
    int getHumanDelay() {
        double val = human_dist(rng);
        int delay = (int)std::round(val);
        if (delay < human_min_delay) delay = human_min_delay;
        if (delay > human_max_delay) delay = human_max_delay;
        return delay;
    }
    
    void click() {
        DWORD flags = MOUSEEVENTF_LEFTDOWN;
        if (button == 1) flags = MOUSEEVENTF_LEFTDOWN;
        else if (button == 2) flags = MOUSEEVENTF_RIGHTDOWN;
        else if (button == 3) flags = MOUSEEVENTF_MIDDLEDOWN;
        
        mouse_event(flags, 0, 0, 0, 0);
        mouse_event(flags << 1, 0, 0, 0, 0);
        click_count++;
    }
    
    void doubleClick() {
        click();
        Sleep(50);
        click();
    }
    
    void tripleClick() {
        click();
        Sleep(50);
        click();
        Sleep(50);
        click();
    }
    
    POINT getPos() {
        POINT pt;
        GetCursorPos(&pt);
        return pt;
    }
    
    void setPos(int x, int y) {
        SetCursorPos(x, y);
    }
    
public:
    AutoClicker() : active(false), running(false), delay_ms(15), button(1),
        click_mode(MODE_HUMAN), burst_count(10), click_count(0),
        human_variance(5), human_jitter(3),
        rng(std::chrono::steady_clock::now().time_since_epoch().count()),
        human_dist(15, 3) {
        updateHumanParams();
    }
    
    void start() {
        if (active) return;
        active = true;
        running = true;
        std::thread(&AutoClicker::run, this).detach();
    }
    
    void stop() {
        active = false;
        running = false;
    }
    
    void toggle() {
        if (active) stop();
        else start();
    }
    
    void setDelay(int ms) {
        delay_ms = (ms >= 1) ? ms : 1;
        updateHumanParams();
    }
    
    void setButton(int btn) { button = btn; }
    void setMode(ClickMode mode) { click_mode = mode; updateHumanParams(); }
    void setBurstCount(int count) { burst_count = count; }
    void setHumanVariance(int var) { human_variance = var; updateHumanParams(); }
    void setHumanJitter(int jitter) { human_jitter = jitter; }
    long long getClickCount() const { return click_count; }
    void resetClickCount() { click_count = 0; }
    
    void run() {
        int burst_remaining = burst_count.load();
        bool hold_active = false;
        
        while (active) {
            ClickMode mode = click_mode.load();
            int current_delay = delay_ms.load();
            
            if (mode == MODE_HUMAN) {
                POINT pt = getPos();
                int jitter = human_jitter.load();
                if (jitter > 0) {
                    std::uniform_int_distribution<int> jitter_dist(-jitter, jitter);
                    setPos(pt.x + jitter_dist(rng), pt.y + jitter_dist(rng));
                    Sleep(1);
                }
            }
            
            switch (mode) {
                case MODE_SINGLE:
                    click();
                    break;
                case MODE_DOUBLE:
                    doubleClick();
                    break;
                case MODE_TRIPLE:
                    tripleClick();
                    break;
                case MODE_HOLD:
                    if (!hold_active) {
                        DWORD flags = (button == 1) ? MOUSEEVENTF_LEFTDOWN :
                                     (button == 2) ? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_MIDDLEDOWN;
                        mouse_event(flags, 0, 0, 0, 0);
                        hold_active = true;
                    }
                    Sleep(10);
                    continue;
                case MODE_RANDOM: {
                    std::uniform_int_distribution<int> delay_dist(current_delay / 2, current_delay * 2);
                    click();
                    Sleep(delay_dist(rng));
                    continue;
                }
                case MODE_BURST:
                    if (burst_remaining > 0) {
                        click();
                        burst_remaining--;
                        if (burst_remaining == 0) active = false;
                        Sleep(50);
                        continue;
                    }
                    break;
                case MODE_HUMAN:
                    click();
                    Sleep(getHumanDelay());
                    continue;
                default:
                    click();
                    break;
            }
            
            if (mode != MODE_HOLD && mode != MODE_HUMAN && mode != MODE_BURST && mode != MODE_RANDOM) {
                Sleep(current_delay);
            }
        }
        
        if (hold_active) {
            DWORD flags = (button == 1) ? MOUSEEVENTF_LEFTUP :
                         (button == 2) ? MOUSEEVENTF_RIGHTUP : MOUSEEVENTF_MIDDLEUP;
            mouse_event(flags, 0, 0, 0, 0);
        }
    }
};

void printBanner() {
    system("cls");
    std::cout << "\033[32m";
    std::cout << "╔══════════════════════════════════════════════════════════╗\n";
    std::cout << "║     AutoClicker PRO v3.0 - Windows Edition              ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════╝\n";
    std::cout << "\033[0m\n";
}

void printHelp() {
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

int main() {
    SetConsoleOutputCP(CP_UTF8);
    
    AutoClicker clicker;
    
    printBanner();
    printHelp();
    
    std::cout << "\033[32mReady! Press F6 to start (Human-like mode by default)\033[0m\n\n> " << std::flush;
    
    std::string line;
    while (true) {
        if (_kbhit()) {
            int key = _getch();
            if (key == 0 || key == 0xE0) {
                key = _getch();
                if (key == 64) { // F6
                    clicker.start();
                    std::cout << "\n\033[32m[F6] Started\033[0m\n> " << std::flush;
                } else if (key == 65) { // F7
                    clicker.stop();
                    std::cout << "\n\033[31m[F7] Stopped. Clicks: " << clicker.getClickCount() << "\033[0m\n> " << std::flush;
                } else if (key == 66) { // F8
                    clicker.toggle();
                    std::cout << "\n\033[33m[F8] Toggled\033[0m\n> " << std::flush;
                } else if (key == 67) { // F9
                    POINT pt;
                    GetCursorPos(&pt);
                    std::cout << "\n\033[36m[F9] Pos saved: " << pt.x << "," << pt.y << "\033[0m\n> " << std::flush;
                } else if (key == 1) { // ESC
                    clicker.stop();
                    break;
                }
                continue;
            }
            
            if (key == '\r' || key == '\n') {
                std::cout << "> " << std::flush;
                continue;
            }
            
            std::cin >> line;
            
            if (line == "s") {
                clicker.start();
                std::cout << "\033[32m▶ Started\033[0m\n> " << std::flush;
            } else if (line == "t") {
                clicker.stop();
                std::cout << "\033[31m■ Stopped. Clicks: " << clicker.getClickCount() << "\033[0m\n> " << std::flush;
            } else if (line == "c") {
                std::cout << "\033[36mClicks: " << clicker.getClickCount() << "\033[0m\n> " << std::flush;
            } else if (line == "r") {
                clicker.resetClickCount();
                std::cout << "\033[36mReset\033[0m\n> " << std::flush;
            } else if (line == "q") {
                clicker.stop();
                break;
            } else if (line.rfind("d", 0) == 0) {
                int delay;
                std::cin >> delay;
                if (delay >= 1 && delay <= 1000) {
                    clicker.setDelay(delay);
                    std::cout << "\033[36mDelay: " << delay << "ms (" << 1000/delay << " CPS)\033[0m\n> " << std::flush;
                }
            } else if (line.rfind("m", 0) == 0) {
                int mode;
                std::cin >> mode;
                if (mode >= 1 && mode <= 7) {
                    clicker.setMode(static_cast<ClickMode>(mode - 1));
                    const char* modes[] = {"Single", "Double", "Triple", "Hold", "Random", "Burst", "Human"};
                    std::cout << "\033[36mMode: " << modes[mode-1] << "\033[0m\n> " << std::flush;
                }
            } else if (line.rfind("b", 0) == 0) {
                int count;
                std::cin >> count;
                clicker.setBurstCount(count);
                std::cout << "\033[36mBurst: " << count << "\033[0m\n> " << std::flush;
            } else if (line.rfind("v", 0) == 0) {
                int var;
                std::cin >> var;
                clicker.setHumanVariance(var);
                std::cout << "\033[36mVariance: ±" << var << "ms\033[0m\n> " << std::flush;
            } else if (line.rfind("j", 0) == 0) {
                int jitter;
                std::cin >> jitter;
                clicker.setHumanJitter(jitter);
                std::cout << "\033[36mJitter: " << jitter << "px\033[0m\n> " << std::flush;
            }
        }
        Sleep(50);
    }
    
    std::cout << "\n\033[33mBye! Total clicks: " << clicker.getClickCount() << "\033[0m\n";
    return 0;
}
