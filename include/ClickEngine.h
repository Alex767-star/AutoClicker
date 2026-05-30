#ifndef CLICK_ENGINE_H
#define CLICK_ENGINE_H

#include <atomic>
#include <thread>
#include <random>
#include <X11/Xlib.h>

enum ClickMode : uint8_t {
    MODE_SINGLE = 0,
    MODE_DOUBLE = 1,
    MODE_TRIPLE = 2,
    MODE_HOLD = 3,
    MODE_RANDOM = 4,
    MODE_BURST = 5,
    MODE_SEQUENCE = 6,
    MODE_MULTI_TARGET = 7,
    MODE_CUSTOM = 8
};

enum TargetType : uint8_t {
    TARGET_CURRENT = 0,
    TARGET_SAVED = 1,
    TARGET_MOVING = 2,
    TARGET_PATTERN = 3,
    TARGET_GRID = 4
};

class ClickEngine {
public:
    static ClickEngine& getInstance();
    void start();
    void stop();
    bool isRunning() const { return running; }
    void setDelay(uint32_t ms) { delay_ms = ms; }
    void setButton(uint8_t btn) { button = btn; }
    void setMode(ClickMode mode) { click_mode = mode; }
    void setTarget(TargetType target) { target_type = target; }
    void setSavedPosition(int x, int y) { saved_x = x; saved_y = y; }
    void setMoveRadius(uint16_t radius) { move_radius = radius; }
    void setBurstCount(uint16_t count) { burst_count = count; }
    uint64_t getClickCount() const { return click_count; }
    void resetClickCount() { click_count = 0; }
    
private:
    ClickEngine();
    ~ClickEngine();
    void engineLoop();
    void performClick(uint8_t btn = 0);
    void performDoubleClick();
    void performTripleClick();
    std::pair<int, int> getCurrentPosition();
    void moveTo(int x, int y);
    
    Display* display;
    Window root;
    std::atomic<bool> running;
    std::atomic<bool> active;
    std::thread engine_thread;
    
    std::atomic<uint32_t> delay_ms;
    std::atomic<uint8_t> button;
    std::atomic<ClickMode> click_mode;
    std::atomic<TargetType> target_type;
    std::atomic<int32_t> saved_x, saved_y;
    std::atomic<uint16_t> move_radius;
    std::atomic<uint16_t> burst_count;
    std::atomic<uint64_t> click_count;
    
    std::mt19937 rng;
    std::uniform_int_distribution<int> pos_dist;
    std::uniform_int_distribution<int> delay_dist;
};

#endif
