#include "ClickEngine.h"
#include <X11/extensions/XTest.h>
#include <X11/keysym.h>
#include <chrono>
#include <thread>
#include <stdexcept>
#include <iostream>
#include <cmath>

ClickEngine& ClickEngine::getInstance() {
    static ClickEngine instance;
    return instance;
}

ClickEngine::ClickEngine() : display(nullptr), root(0), active(false), running(false),
    hotkey_running(false), delay_ms(15), button(1), click_mode(MODE_HUMAN),
    target_type(TARGET_CURRENT), saved_x(0), saved_y(0), move_radius(50),
    burst_count(10), click_count(0), holding(false),
    human_variance(5), human_jitter(3), human_min_delay(8), human_max_delay(25),
    rng(std::chrono::steady_clock::now().time_since_epoch().count()),
    pos_dist(-move_radius, move_radius), delay_dist(8, 25), human_dist(15, 3) {
    
    display = XOpenDisplay(nullptr);
    if (!display) throw std::runtime_error("Cannot open X display");
    root = DefaultRootWindow(display);
    updateHumanParams();
}

ClickEngine::~ClickEngine() {
    stop();
    if (display) XCloseDisplay(display);
}

void ClickEngine::updateHumanParams() {
    uint32_t base = delay_ms.load();
    uint32_t var = human_variance.load();
    human_min_delay = (base > var) ? base - var : 1;
    human_max_delay = base + var;
    human_dist = std::normal_distribution<>((double)base, (double)var / 3.0);
}

int ClickEngine::getHumanDelay() {
    double val = human_dist(rng);
    int delay = (int)std::round(val);
    if (delay < (int)human_min_delay) delay = human_min_delay;
    if (delay > (int)human_max_delay) delay = human_max_delay;
    return delay;
}

std::pair<int, int> ClickEngine::getHumanJitter(int x, int y) {
    uint8_t jitter = human_jitter.load();
    if (jitter == 0) return {x, y};
    std::uniform_int_distribution<int> jitter_dist(-jitter, jitter);
    return {x + jitter_dist(rng), y + jitter_dist(rng)};
}

void ClickEngine::startHotkeyThread() {
    hotkey_running = true;
    hotkey_thread = std::thread(&ClickEngine::hotkeyLoop, this);
}

void ClickEngine::hotkeyLoop() {
    Display* dpy = XOpenDisplay(nullptr);
    if (!dpy) return;
    
    Window root_win = DefaultRootWindow(dpy);
    KeyCode f6 = XKeysymToKeycode(dpy, XK_F6);
    KeyCode f7 = XKeysymToKeycode(dpy, XK_F7);
    KeyCode f8 = XKeysymToKeycode(dpy, XK_F8);
    KeyCode f9 = XKeysymToKeycode(dpy, XK_F9);
    KeyCode esc = XKeysymToKeycode(dpy, XK_Escape);
    
    XSelectInput(dpy, root_win, KeyPressMask);
    
    while (hotkey_running) {
        XEvent ev;
        XNextEvent(dpy, &ev);
        
        if (ev.type == KeyPress) {
            KeyCode code = ev.xkey.keycode;
            
            if (code == f6) {
                start();
                std::cout << "\n\033[32m[F6] Clicking STARTED\033[0m\n> " << std::flush;
            } else if (code == f7) {
                stop();
                std::cout << "\n\033[31m[F7] Clicking STOPPED. Total clicks: " << click_count << "\033[0m\n> " << std::flush;
            } else if (code == f8) {
                toggle();
                std::cout << "\n\033[33m[F8] Clicking " << (active ? "STARTED" : "STOPPED") << "\033[0m\n> " << std::flush;
            } else if (code == f9) {
                auto [x, y] = getCurrentPosition();
                saved_x = x;
                saved_y = y;
                std::cout << "\n\033[36m[F9] Position saved: X=" << x << " Y=" << y << "\033[0m\n> " << std::flush;
            } else if (code == esc) {
                std::cout << "\n\033[33m[ESC] Exiting...\033[0m\n" << std::flush;
                stop();
                hotkey_running = false;
                exit(0);
            }
        }
    }
    XCloseDisplay(dpy);
}

void ClickEngine::start() {
    if (active) return;
    active = true;
    running = true;
    engine_thread = std::thread(&ClickEngine::engineLoop, this);
    engine_thread.detach();
}

void ClickEngine::stop() {
    active = false;
    running = false;
}

void ClickEngine::toggle() {
    if (active) stop();
    else start();
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
    uint16_t burst_remaining = burst_count.load();
    bool hold_active = false;
    
    while (active) {
        ClickMode current_mode = click_mode;
        TargetType current_target = target_type;
        
        int target_x, target_y;
        auto [cur_x, cur_y] = getCurrentPosition();
        
        switch (current_target) {
            case TARGET_SAVED:
                target_x = saved_x;
                target_y = saved_y;
                if (current_mode == MODE_HUMAN) {
                    auto [jx, jy] = getHumanJitter(target_x, target_y);
                    moveTo(jx, jy);
                } else {
                    moveTo(target_x, target_y);
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
                break;
            case TARGET_MOVING:
                if (current_mode == MODE_HUMAN) {
                    auto [jx, jy] = getHumanJitter(cur_x + pos_dist(rng), cur_y + pos_dist(rng));
                    moveTo(jx, jy);
                } else {
                    moveTo(cur_x + pos_dist(rng), cur_y + pos_dist(rng));
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
                break;
            default:
                if (current_mode == MODE_HUMAN && current_target != TARGET_SAVED) {
                    auto [jx, jy] = getHumanJitter(cur_x, cur_y);
                    if (jx != cur_x || jy != cur_y) moveTo(jx, jy);
                }
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
            case MODE_HOLD:
                if (!hold_active) {
                    XTestFakeButtonEvent(display, button, True, CurrentTime);
                    XFlush(display);
                    hold_active = true;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            case MODE_RANDOM:
                performClick();
                std::this_thread::sleep_for(std::chrono::milliseconds(delay_dist(rng)));
                continue;
            case MODE_BURST:
                if (burst_remaining > 0) {
                    performClick();
                    burst_remaining--;
                    if (burst_remaining == 0) {
                        active = false;
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                    continue;
                }
                break;
            case MODE_HUMAN:
                performClick();
                std::this_thread::sleep_for(std::chrono::milliseconds(getHumanDelay()));
                continue;
            default:
                performClick();
                break;
        }
        
        if (current_mode != MODE_HOLD && current_mode != MODE_HUMAN && current_mode != MODE_BURST && current_mode != MODE_RANDOM) {
            std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms.load()));
        }
    }
    
    if (hold_active) {
        XTestFakeButtonEvent(display, button, False, CurrentTime);
        XFlush(display);
    }
}
