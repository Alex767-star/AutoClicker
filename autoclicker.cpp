#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/extensions/XTest.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <cstring>
#include <string>
#include <sstream>
#include <vector>
#include <random>

enum ClickMode {
    SINGLE_CLICK,
    DOUBLE_CLICK,
    HOLD_CLICK,
    RANDOM_DELAY,
    BURST_MODE
};

enum TargetType {
    CURRENT_POSITION,
    SAVED_POSITION,
    MOVING_TARGET
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
    int click_count;
    
    int window_width;
    int window_height;
    
    int delay_entry_x, delay_entry_y;
    std::string delay_input;
    bool delay_editing;
    
    int selected_button;
    std::string button_labels[3];
    
    ClickMode click_mode;
    TargetType target_type;
    
    int saved_x, saved_y;
    int move_radius;
    int burst_count;
    int burst_remaining;
    bool holding;
    
    std::thread click_thread;
    std::thread hotkey_thread;
    std::mt19937 rng;
    
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
    
    void draw_ui() {
        XClearWindow(display, window);
        
        draw_text(150, 30, "AutoClicker PRO v2.0", 0xFFFF00);
        
        draw_text(30, 70, "Delay (ms):", 0x00FF00);
        draw_rect(140, 55, 100, 25, 0x333333, true);
        draw_rect(140, 55, 100, 25, 0x888888, false);
        if (delay_editing) {
            std::string display_text = delay_input + "_";
            draw_text(145, 73, display_text, 0xFFFF00);
        } else {
            draw_text(145, 73, std::to_string(delay_ms), 0x00FF00);
        }
        
        draw_text(30, 105, "Mouse Button:", 0x00FF00);
        for (int i = 0; i < 3; i++) {
            int btn_x = 150 + i * 80;
            if (selected_button == i) {
                draw_rect(btn_x, 90, 70, 25, 0x00AA00, true);
            } else {
                draw_rect(btn_x, 90, 70, 25, 0x333333, true);
                draw_rect(btn_x, 90, 70, 25, 0x888888, false);
            }
            draw_text(btn_x + 15, 108, button_labels[i], 0xFFFFFF);
        }
        
        draw_text(30, 140, "Click Mode:", 0x00FF00);
        const char* modes[] = {"Single", "Double", "Hold", "Random", "Burst"};
        for (int i = 0; i < 5; i++) {
            int mode_x = 150 + i * 55;
            if ((int)click_mode == i) {
                draw_rect(mode_x, 125, 50, 20, 0xAA6600, true);
            } else {
                draw_rect(mode_x, 125, 50, 20, 0x333333, true);
                draw_rect(mode_x, 125, 50, 20, 0x888888, false);
            }
            draw_text(mode_x + 5, 140, modes[i], 0xFFFFFF);
        }
        
        draw_text(30, 175, "Target:", 0x00FF00);
        const char* targets[] = {"Current", "Saved", "Moving"};
        for (int i = 0; i < 3; i++) {
            int target_x = 150 + i * 80;
            if ((int)target_type == i) {
                draw_rect(target_x, 160, 70, 20, 0xAA6600, true);
            } else {
                draw_rect(target_x, 160, 70, 20, 0x333333, true);
                draw_rect(target_x, 160, 70, 20, 0x888888, false);
            }
            draw_text(target_x + 10, 175, targets[i], 0xFFFFFF);
        }
        
        if (target_type == SAVED_POSITION) {
            draw_text(30, 210, "Saved Pos:", 0x888888);
            draw_text(120, 210, "X:" + std::to_string(saved_x) + " Y:" + std::to_string(saved_y), 0xFFFF00);
            draw_text(30, 230, "[Press F5 to save position]", 0x888888);
        } else if (target_type == MOVING_TARGET) {
            draw_text(30, 210, "Move Radius:", 0x888888);
            draw_rect(140, 195, 60, 20, 0x333333, true);
            draw_text(145, 210, std::to_string(move_radius), 0x00FF00);
            draw_text(30, 230, "[+/-] to adjust radius", 0x888888);
        }
        
        draw_text(30, 265, "Status:", 0x00FF00);
        if (clicking) {
            draw_text(100, 265, "ACTIVE", 0xFF0000);
        } else {
            draw_text(100, 265, "STOPPED", 0x00FF00);
        }
        
        draw_text(30, 290, "Clicks:", 0x00FF00);
        draw_text(100, 290, std::to_string(click_count), 0xFFFF00);
        
        if (click_mode == BURST_MODE && burst_remaining > 0) {
            draw_text(30, 315, "Burst Left:", 0x888888);
            draw_text(120, 315, std::to_string(burst_remaining), 0xFF6600);
        }
        
        if (clicking) {
            draw_rect(140, 340, 120, 35, 0xAA0000, true);
            draw_text(155, 363, "STOP (F6)", 0xFFFFFF);
        } else {
            draw_rect(140, 340, 120, 35, 0x00AA00, true);
            draw_text(155, 363, "START (F6)", 0xFFFFFF);
        }
        
        draw_text(30, 390, "Hotkeys:", 0x666666);
        draw_text(30, 410, "F6 - Start/Stop  |  ESC - Exit  |  F5 - Save Pos", 0x666666);
        draw_text(30, 430, "F7 - Single Click |  F8 - Double Click |  F9 - Hold Mode", 0x666666);
        
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
    
    void perform_click() {
        XTestFakeButtonEvent(display, button, True, CurrentTime);
        XTestFakeButtonEvent(display, button, False, CurrentTime);
        XFlush(display);
        click_count++;
    }
    
    void perform_double_click() {
        perform_click();
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        perform_click();
    }
    
    void click_loop() {
        std::uniform_int_distribution<int> dist(-move_radius, move_radius);
        
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
                case HOLD_CLICK:
                    if (!holding) {
                        XTestFakeButtonEvent(display, button, True, CurrentTime);
                        XFlush(display);
                        holding = true;
                    }
                    break;
                case RANDOM_DELAY:
                    {
                        std::uniform_int_distribution<int> delay_dist(delay_ms / 2, delay_ms * 2);
                        int random_delay = delay_dist(rng);
                        perform_click();
                        std::this_thread::sleep_for(std::chrono::milliseconds(random_delay));
                    }
                    break;
                case BURST_MODE:
                    if (burst_remaining > 0) {
                        perform_click();
                        burst_remaining--;
                        if (burst_remaining == 0) {
                            clicking = false;
                        }
                    }
                    break;
            }
            
            if (click_mode != HOLD_CLICK && click_mode != BURST_MODE) {
                std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
            }
        }
        
        if (holding) {
            XTestFakeButtonEvent(display, button, False, CurrentTime);
            XFlush(display);
            holding = false;
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
                } else if (keysym == XK_KP_Add || keysym == XK_plus) {
                    move_radius = std::min(move_radius + 10, 200);
                    draw_ui();
                } else if (keysym == XK_KP_Subtract || keysym == XK_minus) {
                    move_radius = std::max(move_radius - 10, 0);
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
        if (y >= 90 && y <= 115) {
            for (int i = 0; i < 3; i++) {
                int btn_x = 150 + i * 80;
                if (x >= btn_x && x <= btn_x + 70) {
                    selected_button = i;
                    button = i + 1;
                    draw_ui();
                    break;
                }
            }
        }
        
        if (y >= 125 && y <= 145) {
            for (int i = 0; i < 5; i++) {
                int mode_x = 150 + i * 55;
                if (x >= mode_x && x <= mode_x + 50) {
                    click_mode = (ClickMode)i;
                    if (click_mode == BURST_MODE) {
                        burst_remaining = 10;
                    }
                    draw_ui();
                    break;
                }
            }
        }
        
        if (y >= 160 && y <= 180) {
            for (int i = 0; i < 3; i++) {
                int target_x = 150 + i * 80;
                if (x >= target_x && x <= target_x + 70) {
                    target_type = (TargetType)i;
                    draw_ui();
                    break;
                }
            }
        }
        
        if (y >= 340 && y <= 375 && x >= 140 && x <= 260) {
            toggle_clicking();
            draw_ui();
        }
        
        if (y >= 55 && y <= 80 && x >= 140 && x <= 240) {
            delay_editing = true;
            delay_input = std::to_string(delay_ms);
            draw_ui();
        }
        
        if (target_type == MOVING_TARGET && y >= 195 && y <= 215 && x >= 140 && x <= 200) {
            delay_editing = true;
            delay_input = std::to_string(move_radius);
            delay_editing = true;
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
                int new_delay = std::stoi(delay_input);
                if (new_delay >= 1) {
                    if (target_type == MOVING_TARGET && delay_editing) {
                        move_radius = new_delay;
                    } else {
                        delay_ms = new_delay;
                    }
                }
            } catch (...) {}
            delay_editing = false;
            draw_ui();
        }
    }
    
public:
    AutoClicker() : clicking(false), running(true), delay_ms(100), button(1), click_count(0),
                    window_width(500), window_height(460), delay_editing(false), selected_button(0),
                    click_mode(SINGLE_CLICK), target_type(CURRENT_POSITION),
                    saved_x(0), saved_y(0), move_radius(50), burst_count(10), burst_remaining(0), holding(false),
                    rng(std::chrono::steady_clock::now().time_since_epoch().count()) {
        
        button_labels[0] = "Left";
        button_labels[1] = "Right";
        button_labels[2] = "Middle";
        
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
        
        XStoreName(display, window, "AutoClicker PRO");
        
        XSelectInput(display, window, ExposureMask | KeyPressMask | ButtonPressMask);
        
        gc = XCreateGC(display, window, 0, NULL);
        font = XLoadQueryFont(display, "fixed");
        if (font) XSetFont(display, gc, font->fid);
        
        XMapWindow(display, window);
        
        hotkey_thread = std::thread(&AutoClicker::hotkey_listener, this);
        
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
            if (click_mode == BURST_MODE) {
                burst_remaining = burst_count;
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
