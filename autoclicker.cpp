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
#include <functional>

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
    
    std::thread click_thread;
    std::thread hotkey_thread;
    
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
        
        draw_text(150, 40, "AutoClicker v1.0", 0xFFFF00);
        
        draw_text(50, 90, "Delay (ms):", 0x00FF00);
        draw_rect(160, 75, 100, 25, 0x333333, true);
        draw_rect(160, 75, 100, 25, 0x888888, false);
        if (delay_editing) {
            std::string display_text = delay_input + "_";
            draw_text(165, 93, display_text, 0xFFFF00);
        } else {
            draw_text(165, 93, std::to_string(delay_ms), 0x00FF00);
        }
        
        draw_text(50, 130, "Mouse Button:", 0x00FF00);
        for (int i = 0; i < 3; i++) {
            int btn_x = 170 + i * 80;
            if (selected_button == i) {
                draw_rect(btn_x, 115, 70, 25, 0x00AA00, true);
            } else {
                draw_rect(btn_x, 115, 70, 25, 0x333333, true);
                draw_rect(btn_x, 115, 70, 25, 0x888888, false);
            }
            draw_text(btn_x + 15, 133, button_labels[i], 0xFFFFFF);
        }
        
        draw_text(50, 170, "Status:", 0x00FF00);
        if (clicking) {
            draw_text(120, 170, "ACTIVE", 0xFF0000);
        } else {
            draw_text(120, 170, "STOPPED", 0x00FF00);
        }
        
        draw_text(50, 200, "Clicks:", 0x00FF00);
        draw_text(120, 200, std::to_string(click_count), 0xFFFF00);
        
        if (clicking) {
            draw_rect(140, 220, 120, 35, 0xAA0000, true);
            draw_text(150, 243, "STOP (F6)", 0xFFFFFF);
        } else {
            draw_rect(140, 220, 120, 35, 0x00AA00, true);
            draw_text(150, 243, "START (F6)", 0xFFFFFF);
        }
        
        draw_text(50, 280, "Hotkeys:", 0x888888);
        draw_text(50, 300, "F6 - Start/Stop", 0x888888);
        draw_text(50, 320, "ESC - Exit", 0x888888);
        
        XFlush(display);
    }
    
    void click_loop() {
        while (clicking && running) {
            XTestFakeButtonEvent(display, button, True, CurrentTime);
            XTestFakeButtonEvent(display, button, False, CurrentTime);
            XFlush(display);
            click_count++;
            std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
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
        if (y >= 115 && y <= 140) {
            for (int i = 0; i < 3; i++) {
                int btn_x = 170 + i * 80;
                if (x >= btn_x && x <= btn_x + 70) {
                    selected_button = i;
                    button = i + 1;
                    draw_ui();
                    break;
                }
            }
        }
        
        if (y >= 220 && y <= 255 && x >= 140 && x <= 260) {
            toggle_clicking();
            draw_ui();
        }
        
        if (y >= 75 && y <= 100 && x >= 160 && x <= 260) {
            delay_editing = true;
            delay_input = std::to_string(delay_ms);
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
                    delay_ms = new_delay;
                }
            } catch (...) {}
            delay_editing = false;
            draw_ui();
        }
    }
    
public:
    AutoClicker() : clicking(false), running(true), delay_ms(100), button(1), click_count(0),
                    window_width(400), window_height(360), delay_editing(false), selected_button(0) {
        
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
        
        XStoreName(display, window, "AutoClicker");
        
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
