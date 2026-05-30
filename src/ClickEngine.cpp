#include "ClickEngine.h"
#include <X11/extensions/XTest.h>
#include <chrono>
#include <thread>

ClickEngine& ClickEngine::getInstance() {
    static ClickEngine instance;
    return instance;
}

ClickEngine::ClickEngine() : running(false), active(false), delay_ms(100), button(1),
    click_mode(MODE_SINGLE), target_type(TARGET_CURRENT), saved_x(0), saved_y(0),
    move_radius(50), burst_count(10), click_count(0),
    rng(std::chrono::steady_clock::now().time_since_epoch().count()),
    pos_dist(-move_radius, move_radius), delay_dist(50, 200) {
    
    display = XOpenDisplay(nullptr);
    if (!display) throw std::runtime_error("Cannot open X display");
    root = DefaultRootWindow(display);
}

ClickEngine::~ClickEngine() {
    stop();
    if (display) XCloseDisplay(display);
}

void ClickEngine::start() {
    if (running) return;
    active = true;
    running = true;
    engine_thread = std::thread(&ClickEngine::engineLoop, this);
}

void ClickEngine::stop() {
    active = false;
    if (engine_thread.joinable()) engine_thread.join();
    running = false;
}

std::pair<int, int> ClickEngine::getCurrentPosition() {
    Window root_ret, child_ret;
    int root_x, root_y, win_x, win_y;
    unsigned int mask;
    XQueryPointer(display, root, &root_ret, &child_ret, &root_x, &root_y, &win_x, &win_y, &mask);
    return {root_x, root_y};
}

void ClickEngine::moveTo(int x, int y) {
    XWarpPointer(display, None, root, 0, 0, 0, 0, x, y);
    XFlush(display);
}

void ClickEngine::performClick(uint8_t btn) {
    uint8_t click_btn = (btn == 0) ? button.load() : btn;
    XTestFakeButtonEvent(display, click_btn, True, CurrentTime);
    XTestFakeButtonEvent(display, click_btn, False, CurrentTime);
    XFlush(display);
    click_count++;
}

void ClickEngine::performDoubleClick() {
    performClick();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    performClick();
}

void ClickEngine::performTripleClick() {
    performClick();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    performClick();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    performClick();
}

void ClickEngine::engineLoop() {
    while (active) {
        ClickMode current_mode = click_mode;
        TargetType current_target = target_type;
        
        switch (current_target) {
            case TARGET_SAVED:
                moveTo(saved_x, saved_y);
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
                break;
            case TARGET_MOVING: {
                auto [x, y] = getCurrentPosition();
                moveTo(x + pos_dist(rng), y + pos_dist(rng));
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
                break;
            }
            default:
                break;
        }
        
        switch (current_mode) {
            case MODE_SINGLE:
                performClick();
                break;
            case MODE_DOUBLE:
                performDoubleClick();
                break;
            case MODE_TRIPLE:
                performTripleClick();
                break;
            case MODE_RANDOM:
                performClick();
                std::this_thread::sleep_for(std::chrono::milliseconds(delay_dist(rng)));
                continue;
            default:
                performClick();
                break;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms.load()));
    }
}
