#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/extensions/XTest.h>
#include <X11/extensions/Xrandr.h>
#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>
#include <atomic>
#include <cstring>
#include <string>
#include <sstream>
#include <vector>
#include <random>
#include <chrono>
#include <cmath>
#include <map>
#include <algorithm>

enum ClickMode {
    SINGLE_CLICK,
    DOUBLE_CLICK,
    TRIPLE_CLICK,
    HOLD_CLICK,
    RANDOM_DELAY,
    BURST_MODE,
    SEQUENCE_MODE,
    MULTI_TARGET
};

enum TargetType {
    CURRENT_POSITION,
    SAVED_POSITION,
    MOVING_TARGET,
    PATTERN_MODE,
    GRID_MODE
};

struct ClickSequence {
    std::vector<int> delays;
    std::vector<int> buttons;
    int repeat_count;
    bool loop;
};

struct TargetPoint {
    int x, y;
    int delay;
};

class AutoClicker {
private:
    Display* display;
    Window root;
    Window window;
    GC gc;
    XFontStruct* font;
    
    std::atomic<bool> clicking;
    std::atomic<bool> running;
    int delay_ms;
    int button;
    std::atomic<int> click_count;
    
    int window_width;
    int window_height;
    
    std::string delay_input;
    bool delay_editing;
    
    int selected_button;
    std::string button_labels[5];
    
    ClickMode click_mode;
    TargetType target_type;
    
    int saved_x, saved_y;
    int move_radius;
    int burst_count;
    int burst_remaining;
    bool holding;
    
    std::vector<TargetPoint> target_points;
    int current_target_index;
    
    ClickSequence current_sequence;
    int sequence_step;
    
    int grid_rows, grid_cols;
    int grid_current_row, grid_current_col;
    
    std::chrono::steady_clock::time_point start_time;
    std::map<std::string, int> stats;
    
    std::thread click_thread;
    std::thread hotkey_thread;
    std::mt19937 rng;
    
    int profile_id;
    std::string profile_name;
    
    void draw_text(int x, int y, const std::string& text, unsigned long color = 0x00FF00) {
        XSetForeground(display, gc, color);
        XDrawString(display, window, gc, x, y, text.c_str(), text.length());
    }
    
    void draw_rect(int x, int y, int w, int h, unsigned long color, bool fill = true) {
        XSetForeground(display, gc, color);
        if (fill) {
            XFillRectangle(display, window, gc, x, y, w, h);
        } else {
            XDrawRectangle(display, window, gc, x, y, w, h);
        }
    }
    
    void draw_progress_bar(int x, int y, int w, int h, int percentage, unsigned long color) {
        int filled = (w * percentage) / 100;
        draw_rect(x, y, w, h, 0x333333, true);
        if (filled > 0) {
            draw_rect(x, y, filled, h, color, true);
        }
    }
    
    void draw_stats() {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start_time).count();
        
        draw_text(window_width - 150, 30, "STATS", 0xFFFF00);
        draw_text(window_width - 150, 50, "Time: " + std::to_string(elapsed) + "s", 0x888888);
        draw_text(window_width - 150, 70, "CPS: " + std::to_string(click_count / (elapsed > 0 ? elapsed : 1)), 0x888888);
        draw_text(window_width - 150, 90, "Total: " + std::to_string(click_count.load()), 0xFFFF00);
    }
    
    void draw_ui() {
        XClearWindow(display, window);
        
        draw_text(10, 20, "AutoClicker PRO v3.0", 0xFFFF00);
        draw_text(10, 40, "[CounterLib Integrated]", 0xFF6600);
        
        int current_y = 70;
        
        draw_text(30, current_y, "Delay (ms):", 0x00FF00);
        draw_rect(140, current_y - 15, 100, 25, 0x333333, true);
        draw_rect(140, current_y - 15, 100, 25, 0x888888, false);
        if (delay_editing && !delay_input.empty()) {
            draw_text(145, current_y, delay_input + "_", 0xFFFF00);
        } else {
            draw_text(145, current_y, std::to_string(delay_ms), 0x00FF00);
        }
        
        current_y += 40;
        
        draw_text(30, current_y, "Mouse Button:", 0x00FF00);
        for (int i = 0; i < 5; i++) {
            int btn_x = 160 + i * 65;
            if (selected_button == i) {
                draw_rect(btn_x, current_y - 15, 55, 22, 0x00AA00, true);
            } else {
                draw_rect(btn_x, current_y - 15, 55, 22, 0x333333, true);
                draw_rect(btn_x, current_y - 15, 55, 22, 0x888888, false);
            }
            draw_text(btn_x + 5, current_y, button_labels[i], 0xFFFFFF);
        }
        
        current_y += 40;
        
        draw_text(30, current_y, "Click Mode:", 0x00FF00);
        const char* modes[] = {"Single", "Double", "Triple", "Hold", "Random", "Burst", "Seq", "Multi"};
        for (int i = 0; i < 8; i++) {
            int mode_x = 140 + i * 45;
            if ((int)click_mode == i) {
                draw_rect(mode_x, current_y - 15, 40, 20, 0xAA6600, true);
            } else {
                draw_rect(mode_x, current_y - 15, 40, 20, 0x333333, true);
                draw_rect(mode_x, current_y - 15, 40, 20, 0x888888, false);
            }
            draw_text(mode_x + 5, current_y, modes[i], 0xFFFFFF);
        }
        
        current_y += 40;
        
        draw_text(30, current_y, "Target:", 0x00FF00);
        const char* targets[] = {"Current", "Saved", "Moving", "Pattern", "Grid"};
        for (int i = 0; i < 5; i++) {
            int target_x = 100 + i * 80;
            if ((int)target_type == i) {
                draw_rect(target_x, current_y - 15, 70, 20, 0xAA6600, true);
            } else {
                draw_rect(target_x, current_y - 15, 70, 20, 0x333333, true);
                draw_rect(target_x, current_y - 15, 70, 20, 0x888888, false);
            }
            draw_text(target_x + 10, current_y, targets[i], 0xFFFFFF);
        }
        
        current_y += 40;
        
        if (target_type == SAVED_POSITION) {
            draw_text(30, current_y, "Saved Pos:", 0x888888);
            draw_text(120, current_y, "X:" + std::to_string(saved_x) + " Y:" + std::to_string(saved_y), 0xFFFF00);
            draw_text(30, current_y + 20, "[F5 - Save | F6 - Start]", 0x666666);
            current_y += 50;
        } else if (target_type == MOVING_TARGET) {
            draw_text(30, current_y, "Move Radius:", 0x888888);
            draw_rect(140, current_y - 15, 60, 20, 0x333333, true);
            draw_text(145, current_y, std::to_string(move_radius), 0x00FF00);
            draw_text(30, current_y + 20, "[+/-] Adjust radius", 0x666666);
            current_y += 50;
        } else if (target_type == GRID_MODE) {
            draw_text(30, current_y, "Grid:", 0x888888);
            draw_text(80, current_y, std::to_string(grid_rows) + "x" + std::to_string(grid_cols), 0xFFFF00);
            draw_text(30, current_y + 20, "[G] Change grid size", 0x666666);
            current_y += 50;
        }
        
        draw_text(30, current_y, "Burst Count:", 0x888888);
        draw_rect(140, current_y - 15, 60, 20, 0x333333, true);
        draw_text(145, current_y, std::to_string(burst_count), 0x00FF00);
        draw_text(30, current_y + 20, "[B] Set burst count", 0x666666);
        
        current_y += 50;
        
        draw_text(30, current_y, "Profile:", 0x00FF00);
        draw_rect(100, current_y - 15, 150, 22, 0x333333, true);
        draw_text(105, current_y, profile_name, 0xFFFF00);
        draw_text(260, current_y, "[P] Load  [S] Save", 0x666666);
        
        current_y += 45;
        
        if (clicking) {
            draw_rect(140, current_y, 120, 35, 0xAA0000, true);
            draw_text(155, current_y + 23, "STOP (F6)", 0xFFFFFF);
        } else {
            draw_rect(140, current_y, 120, 35, 0x00AA00, true);
            draw_text(155, current_y + 23, "START (F6)", 0xFFFFFF);
        }
        
        draw_text(30, current_y + 50, "Status:", 0x00FF00);
        if (clicking) {
            draw_text(100, current_y + 50, "ACTIVE", 0xFF0000);
        } else {
            draw_text(100, current_y + 50, "STOPPED", 0x00FF00);
        }
        
        draw_text(200, current_y + 50, "Clicks: " + std::to_string(click_count.load()), 0xFFFF00);
        
        if (click_mode == BURST_MODE && burst_remaining > 0 && clicking) {
            draw_text(30, current_y + 75, "Burst Left:", 0xFF6600);
            draw_text(120, current_y + 75, std::to_string(burst_remaining), 0xFF6600);
            draw_progress_bar(30, current_y + 90, 200, 10, 
                            (burst_count - burst_remaining) * 100 / burst_count, 0xFF6600);
        }
        
        draw_stats();
        
        draw_text(10, window_height - 60, "Hotkeys:", 0x444444);
        draw_text(10, window_height - 40, "F6:Start/Stop  ESC:Exit  F5:SavePos  F7:Single  F8:Double  F9:Hold", 0x444444);
        draw_text(10, window_height - 20, "F10:Burst  F11:Random  F12:Pattern  +|-:Radius  G:Grid  B:Burst  P:Profile  S:Save", 0x444444);
        
        XFlush(display);
    }
    
    int get_current_x() {
        Window root_return, child_return;
        int root_x, root_y, win_x, win_y;
        unsigned int mask;
        XQueryPointer(display, root, &root_return, &child_return,
                     &root_x, &root_y, &win_x, &win_y, &mask);
        return root_x;
    }
    
    int get_current_y() {
        Window root_return, child_return;
        int root_x, root_y, win_x, win_y;
        unsigned int mask;
        XQueryPointer(display, root, &root_return, &child_return,
                     &root_x, &root_y, &win_x, &win_y, &mask);
        return root_y;
    }
    
    void move_mouse_to(int x, int y) {
        XWarpPointer(display, None, root, 0, 0, 0, 0, x, y);
        XFlush(display);
    }
    
    void perform_click(int btn = -1) {
        int click_btn = (btn == -1) ? button : btn;
        XTestFakeButtonEvent(display, click_btn, True, CurrentTime);
        XTestFakeButtonEvent(display, click_btn, False, CurrentTime);
        XFlush(display);
        click_count++;
    }
    
    void perform_double_click() {
        perform_click();
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        perform_click();
    }
    
    void perform_triple_click() {
        perform_click();
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        perform_click();
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        perform_click();
    }
    
    void click_loop() {
        std::uniform_int_distribution<int> dist(-move_radius, move_radius);
        std::uniform_int_distribution<int> delay_dist(delay_ms / 2, delay_ms * 2);
        
        while (clicking && running) {
            int target_x, target_y;
            
            switch (target_type) {
                case SAVED_POSITION:
                    target_x = saved_x;
                    target_y = saved_y;
                    move_mouse_to(target_x, target_y);
                    std::this_thread::sleep_for(std::chrono::milliseconds(5));
                    break;
                case MOVING_TARGET:
                    target_x = get_current_x() + dist(rng);
                    target_y = get_current_y() + dist(rng);
                    move_mouse_to(target_x, target_y);
                    std::this_thread::sleep_for(std::chrono::milliseconds(5));
                    break;
                case PATTERN_MODE:
                    if (!target_points.empty()) {
                        target_x = target_points[current_target_index].x;
                        target_y = target_points[current_target_index].y;
                        move_mouse_to(target_x, target_y);
                        std::this_thread::sleep_for(std::chrono::milliseconds(
                            target_points[current_target_index].delay));
                        current_target_index = (current_target_index + 1) % target_points.size();
                    }
                    break;
                case GRID_MODE:
                    {
                        int screen_width = DisplayWidth(display, DefaultScreen(display));
                        int screen_height = DisplayHeight(display, DefaultScreen(display));
                        int cell_w = screen_width / grid_cols;
                        int cell_h = screen_height / grid_rows;
                        target_x = grid_current_col * cell_w + cell_w / 2;
                        target_y = grid_current_row * cell_h + cell_h / 2;
                        move_mouse_to(target_x, target_y);
                        std::this_thread::sleep_for(std::chrono::milliseconds(5));
                        grid_current_col++;
                        if (grid_current_col >= grid_cols) {
                            grid_current_col = 0;
                            grid_current_row++;
                            if (grid_current_row >= grid_rows) {
                                grid_current_row = 0;
                            }
                        }
                    }
                    break;
                default:
                    break;
            }
            
            switch (click_mode) {
                case SINGLE_CLICK:
                    perform_click();
                    break;
                case DOUBLE_CLICK:
                    perform_double_click();
                    break;
                case TRIPLE_CLICK:
                    perform_triple_click();
                    break;
                case HOLD_CLICK:
                    if (!holding) {
                        XTestFakeButtonEvent(display, button, True, CurrentTime);
                        XFlush(display);
                        holding = true;
                    }
                    break;
                case RANDOM_DELAY:
                    perform_click();
                    std::this_thread::sleep_for(std::chrono::milliseconds(delay_dist(rng)));
                    break;
                case BURST_MODE:
                    if (burst_remaining > 0) {
                        perform_click();
                        burst_remaining--;
                        if (burst_remaining == 0) {
                            clicking = false;
                        }
                        std::this_thread::sleep_for(std::chrono::milliseconds(50));
                    }
                    break;
                case SEQUENCE_MODE:
                    if (sequence_step < current_sequence.delays.size()) {
                        int seq_btn = (sequence_step < current_sequence.buttons.size()) ? 
                                     current_sequence.buttons[sequence_step] : button;
                        perform_click(seq_btn);
                        std::this_thread::sleep_for(std::chrono::milliseconds(
                            current_sequence.delays[sequence_step]));
                        sequence_step++;
                        if (sequence_step >= current_sequence.delays.size()) {
                            if (current_sequence.loop) {
                                sequence_step = 0;
                            } else {
                                clicking = false;
                            }
                        }
                    }
                    break;
                case MULTI_TARGET:
                    if (!target_points.empty()) {
                        perform_click();
                        std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
                    }
                    break;
            }
            
            if (click_mode != HOLD_CLICK && click_mode != BURST_MODE && 
                click_mode != SEQUENCE_MODE && click_mode != PATTERN_MODE) {
                std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
            }
        }
        
        if (holding) {
            XTestFakeButtonEvent(display, button, False, CurrentTime);
            XFlush(display);
            holding = false;
        }
    }
    
    void save_profile() {
        std::string filename = "autoclicker_profile_" + std::to_string(profile_id) + ".cfg";
        std::ofstream file(filename);
        if (file.is_open()) {
            file << delay_ms << "\n";
            file << button << "\n";
            file << (int)click_mode << "\n";
            file << (int)target_type << "\n";
            file << saved_x << "\n";
            file << saved_y << "\n";
            file << move_radius << "\n";
            file << burst_count << "\n";
            file << grid_rows << "\n";
            file << grid_cols << "\n";
            file.close();
        }
    }
    
    void load_profile() {
        std::string filename = "autoclicker_profile_" + std::to_string(profile_id) + ".cfg";
        std::ifstream file(filename);
        if (file.is_open()) {
            int mode, target;
            file >> delay_ms >> button >> mode >> target;
            file >> saved_x >> saved_y >> move_radius >> burst_count;
            file >> grid_rows >> grid_cols;
            click_mode = (ClickMode)mode;
            target_type = (TargetType)target;
            selected_button = button - 1;
            file.close();
        }
    }
    
    void hotkey_listener() {
        Display* dpy = XOpenDisplay(NULL);
        if (!dpy) return;
        
        Window root_win = DefaultRootWindow(dpy);
        XSelectInput(dpy, root_win, KeyPressMask);
        
        XEvent ev;
        while (running) {
            XNextEvent(dpy, &ev);
            if (ev.type == KeyPress) {
                KeySym keysym = XLookupKeysym(&ev.xkey, 0);
                if (keysym == XK_F6) {
                    toggle_clicking();
                    draw_ui();
                } else if (keysym == XK_F5) {
                    saved_x = get_current_x();
                    saved_y = get_current_y();
                    draw_ui();
                } else if (keysym == XK_F7) {
                    click_mode = SINGLE_CLICK;
                    draw_ui();
                } else if (keysym == XK_F8) {
                    click_mode = DOUBLE_CLICK;
                    draw_ui();
                } else if (keysym == XK_F9) {
                    click_mode = HOLD_CLICK;
                    draw_ui();
                } else if (keysym == XK_F10) {
                    click_mode = BURST_MODE;
                    burst_remaining = burst_count;
                    draw_ui();
                } else if (keysym == XK_F11) {
                    click_mode = RANDOM_DELAY;
                    draw_ui();
                } else if (keysym == XK_F12) {
                    click_mode = MULTI_TARGET;
                    draw_ui();
                } else if (keysym == XK_KP_Add || keysym == XK_plus) {
                    move_radius = std::min(move_radius + 10, 200);
                    draw_ui();
                } else if (keysym == XK_KP_Subtract || keysym == XK_minus) {
                    move_radius = std::max(move_radius - 10, 0);
                    draw_ui();
                } else if (keysym == XK_g || keysym == XK_G) {
                    grid_rows = (grid_rows % 10) + 1;
                    grid_cols = (grid_cols % 10) + 1;
                    draw_ui();
                } else if (keysym == XK_b || keysym == XK_B) {
                    delay_editing = true;
                    delay_input = std::to_string(burst_count);
                    draw_ui();
                } else if (keysym == XK_p || keysym == XK_P) {
                    profile_id = (profile_id % 5) + 1;
                    profile_name = "Profile " + std::to_string(profile_id);
                    load_profile();
                    draw_ui();
                } else if (keysym == XK_s || keysym == XK_S) {
                    save_profile();
                    draw_ui();
                } else if (keysym == XK_Escape) {
                    running = false;
                    clicking = false;
                    XDestroyWindow(display, window);
                    XCloseDisplay(display);
                    exit(0);
                }
            }
        }
        XCloseDisplay(dpy);
    }
    
    void handle_mouse_click(int x, int y) {
        int current_y = 70;
        
        if (y >= current_y - 15 && y <= current_y + 10 && x >= 140 && x <= 240) {
            delay_editing = true;
            delay_input = std::to_string(delay_ms);
            draw_ui();
            return;
        }
        
        current_y += 40;
        if (y >= current_y - 15 && y <= current_y + 7) {
            for (int i = 0; i < 5; i++) {
                int btn_x = 160 + i * 65;
                if (x >= btn_x && x <= btn_x + 55) {
                    selected_button = i;
                    button = i + 1;
                    draw_ui();
                    return;
                }
            }
        }
        
        current_y += 40;
        if (y >= current_y - 15 && y <= current_y + 5) {
            for (int i = 0; i < 8; i++) {
                int mode_x = 140 + i * 45;
                if (x >= mode_x && x <= mode_x + 40) {
                    click_mode = (ClickMode)i;
                    if (click_mode == BURST_MODE) {
                        burst_remaining = burst_count;
                    }
                    draw_ui();
                    return;
                }
            }
        }
        
        current_y += 40;
        if (y >= current_y - 15 && y <= current_y + 5) {
            for (int i = 0; i < 5; i++) {
                int target_x = 100 + i * 80;
                if (x >= target_x && x <= target_x + 70) {
                    target_type = (TargetType)i;
                    draw_ui();
                    return;
                }
            }
        }
        
        current_y += 95;
        if (y >= current_y && y <= current_y + 35 && x >= 140 && x <= 260) {
            toggle_clicking();
            draw_ui();
        }
        
        if (y >= 295 && y <= 315 && x >= 140 && x <= 200) {
            delay_editing = true;
            delay_input = std::to_string(burst_count);
            draw_ui();
        }
    }
    
    void handle_keyboard(char key) {
        if (!delay_editing) return;
        
        if (key >= '0' && key <= '9') {
            delay_input += key;
            draw_ui();
        } else if (key == '\b' && !delay_input.empty()) {
            delay_input.pop_back();
            draw_ui();
        } else if (key == '\r') {
            try {
                int new_value = std::stoi(delay_input);
                if (new_value >= 1) {
                    if (delay_editing) {
                        burst_count = new_value;
                    } else {
                        delay_ms = new_value;
                    }
                }
            } catch (...) {}
            delay_editing = false;
            delay_input.clear();
            draw_ui();
        }
    }
    
public:
    AutoClicker() : clicking(false), running(true), delay_ms(100), button(1), click_count(0),
                    window_width(800), window_height(600), delay_editing(false), selected_button(0),
                    click_mode(SINGLE_CLICK), target_type(CURRENT_POSITION),
                    saved_x(0), saved_y(0), move_radius(50), burst_count(10), burst_remaining(0), 
                    holding(false), current_target_index(0), sequence_step(0),
                    grid_rows(3), grid_cols(3), grid_current_row(0), grid_current_col(0),
                    profile_id(1), profile_name("Profile 1"),
                    rng(std::chrono::steady_clock::now().time_since_epoch().count()) {
        
        button_labels[0] = "Left";
        button_labels[1] = "Right";
        button_labels[2] = "Middle";
        button_labels[3] = "X1";
        button_labels[4] = "X2";
        
        current_sequence.delays = {100, 200, 150};
        current_sequence.buttons = {1, 2, 1};
        current_sequence.repeat_count = 3;
        current_sequence.loop = true;
        
        target_points = {{500, 300, 100}, {600, 400, 200}, {400, 500, 150}};
        
        display = XOpenDisplay(NULL);
        if (!display) {
            std::cerr << "Cannot open display" << std::endl;
            exit(1);
        }
        
        root = DefaultRootWindow(display);
        
        XSetWindowAttributes attrs;
        attrs.background_pixel = BlackPixel(display, DefaultScreen(display));
        attrs.event_mask = ExposureMask | KeyPressMask | ButtonPressMask | StructureNotifyMask;
        
        window = XCreateWindow(display, root, 100, 100, window_width, window_height, 0,
                               CopyFromParent, InputOutput, CopyFromParent,
                               CWBackPixel | CWEventMask, &attrs);
        
        XStoreName(display, window, "AutoClicker PRO v3.0 - CounterLib Edition");
        
        XSelectInput(display, window, ExposureMask | KeyPressMask | ButtonPressMask);
        
        gc = XCreateGC(display, window, 0, NULL);
        font = XLoadQueryFont(display, "fixed");
        if (font) XSetFont(display, gc, font->fid);
        
        XMapWindow(display, window);
        
        start_time = std::chrono::steady_clock::now();
        
        hotkey_thread = std::thread(&AutoClicker::hotkey_listener, this);
        
        load_profile();
        
        XEvent ev;
        while (running) {
            XNextEvent(display, &ev);
            if (ev.type == Expose) {
                draw_ui();
            } else if (ev.type == ButtonPress) {
                int x = ev.xbutton.x;
                int y = ev.xbutton.y;
                handle_mouse_click(x, y);
            } else if (ev.type == KeyPress) {
                char buffer[32];
                KeySym keysym;
                XLookupString(&ev.xkey, buffer, sizeof(buffer), &keysym, NULL);
                if (keysym == XK_Return) {
                    handle_keyboard('\r');
                } else if (keysym == XK_BackSpace) {
                    handle_keyboard('\b');
                } else if (buffer[0] >= 32 && buffer[0] <= 126) {
                    handle_keyboard(buffer[0]);
                }
            }
        }
    }
    
    void toggle_clicking() {
        if (!clicking) {
            clicking = true;
            start_time = std::chrono::steady_clock::now();
            if (click_mode == BURST_MODE) {
                burst_remaining = burst_count;
            }
            if (click_mode == SEQUENCE_MODE) {
                sequence_step = 0;
            }
            if (target_type == PATTERN_MODE) {
                current_target_index = 0;
            }
            click_thread = std::thread(&AutoClicker::click_loop, this);
        } else {
            clicking = false;
            if (click_thread.joinable()) {
                click_thread.join();
            }
        }
    }
    
    ~AutoClicker() {
        running = false;
        clicking = false;
        if (click_thread.joinable()) click_thread.join();
        if (hotkey_thread.joinable()) hotkey_thread.join();
        XFreeGC(display, gc);
        if (font) XFreeFont(display, font);
        XDestroyWindow(display, window);
        XCloseDisplay(display);
    }
};

int main() {
    AutoClicker app;
    return 0;
}
