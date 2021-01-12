#include "aluspointer.h"
#include "util.h"
#include "common.h"
#include <memory>
#include <iostream>
#include <algorithm>
#include <X11/Xlib.h>
#include <X11/extensions/XTest.h>

// TODO use enum
#define INIT_CONNECTION_ERR 0
#define INIT_SCREEN_ERR     1

namespace aluspointer
{
    struct DisplayDeleter
    {
        void operator()(Display *p) const
        {
            if(p) XCloseDisplay(p);
        }
    };

    using display_ptr = std::unique_ptr<Display, DisplayDeleter>;
    
    display_ptr scoped_display;

    Display *display_;
    Window root_;
    int screen_number;
    
    static int spy_error_handler(Display *dpy, XErrorEvent *ev)
    {
        if (ev->error_code == BadWindow || ev->error_code == BadMatch) {
            /* Window was destroyed */
            
        }

//        if (old_error_handler)
//            return old_error_handler(dpy, ev);

        return 0;
    }
    
    void initialize()
    {
        display_ = XOpenDisplay(NULL);
        if(!display_)
        {
            std::cerr << "Unable to open display default.\n";
            throw INIT_CONNECTION_ERR;
        }
        scoped_display = display_ptr(display_);
        
        root_ = DefaultRootWindow(display_);
        screen_number = DefaultScreen(display_);
        
        _NET_CLIENT_LIST           = locate_atom("_NET_CLIENT_LIST");
        _NET_WM_WINDOW_TYPE        = locate_atom("_NET_WM_WINDOW_TYPE");
        _NET_WM_WINDOW_TYPE_NORMAL = locate_atom("_NET_WM_WINDOW_TYPE_NORMAL");
//        _NET_WM_STATE              = locate_atom("_NET_WM_STATE");
//        _NET_WM_STATE_HIDDEN       = locate_atom("_NET_WM_STATE_HIDDEN");
        _NET_ACTIVE_WINDOW         = locate_atom("_NET_ACTIVE_WINDOW");
        
        XSetErrorHandler(spy_error_handler);
    }
    
//    inline Atom locate_atom(std::string name)
//    {
//        return XInternAtom(display_, name.c_str(), true);
//    }
    
    // WARNING: flush manually.
    inline void send_fake_key(KeyCode keycode, bool is_press)
    {
        // TODO use XSendEvent
        XTestFakeKeyEvent(display_, keycode, is_press, 0);
    }

//    enum XK_KeySym
//    {
//        XK_Shift_L          = 0xffe1,
//        XK_Control_L        = 0xffe3,
//        XK_Return           = 0xff0d,
//        XK_Backspace        = 0xff08,
//    //  XK_Space            = 0x0020
//    };
    
    inline KeyCode get_keycode(KeySym keysym)
    {
        return XKeysymToKeycode(display_, keysym);
    }

    // WARNING: release manually. If uses flags, release with the same flags.
    // WARNING: flush manually.
    void press_key(KeyCode keycode, int flags)
    {
        if(flags & MOD_SHIFT)
            send_fake_key(get_keycode(XK_Shift_L), true);
        
        if(flags & MOD_CTRL)
            send_fake_key(get_keycode(XK_Control_L), true);
        
        // TODO flags capslock
        
        send_fake_key(keycode, true);
    }

    // WARNING: flush manually.
    void release_key(KeyCode keycode, int flags)
    {
        if(flags & MOD_SHIFT)
            send_fake_key(get_keycode(XK_Shift_L), false);
        
        if(flags & MOD_CTRL)
            send_fake_key(get_keycode(XK_Control_L), false);
        
        // TODO flags capslock
        
        send_fake_key(keycode, false);
    }
    
    void tap_key(key_type type)
    {
        auto keycode = get_keycode(type);
        
        press_key(keycode, MOD_NONE);
        release_key(keycode, MOD_NONE);
        
        flush();
    }
    
    void tap_key(char ascii)
    {
        int flags = MOD_NONE;
        const std::string cap("~!@#$%^&*()_+{}|:\"<>?");

        if ((ascii >= 'A' && ascii <= 'Z') || (cap.find(ascii) != std::string::npos))
            flags |= MOD_SHIFT;
        
        KeyCode keycode;
        
        if(ascii >= 32 && ascii <= 126)
            keycode = get_keycode(ascii);
        else if(ascii == 8) // backspace
            keycode = get_keycode(XK_BackSpace);
        else if(ascii == 15) // return
            keycode = get_keycode(XK_Return);
        
        press_key(keycode, flags);
        release_key(keycode, flags);
        
        flush();
    }
    
    void tap_key_in_utf8(char32_t codepoint)
    {
        // CTRL+SHIFT+U
        auto k_u = get_keycode('u');
        press_key(k_u, MOD_CTRL | MOD_SHIFT);
        release_key(k_u, MOD_CTRL | MOD_SHIFT);

        // Pressing hexes
        auto hex = int_to_hex(codepoint);
        std::for_each(hex.begin(), hex.end(), [&](char c)
        {
            auto keycode = get_keycode(c);
            press_key(keycode, MOD_NONE);
            release_key(keycode, MOD_NONE);
        });

        // Pressing return
        auto k_return = get_keycode(XK_Return);
        press_key(k_return, MOD_NONE);
        release_key(k_return, MOD_NONE);
        
        flush();
    }
    
    void type_string(const tiny_utf8::string &str)
    {
        std::for_each(str.begin(), str.end(), [&](char32_t codepoint)
        {
            if(codepoint < 0x80) tap_key(codepoint);
            else tap_key_in_utf8(codepoint);
        });
    }
    
    void move_mouse(int x, int y)
    {
        // TODO use the other way. maybe XWarpPointer?
        XTestFakeRelativeMotionEvent(display_, x, y, CurrentTime);
        flush();        
    }
    
    void move_mouse_to(int x, int y)
    {
        XTestFakeMotionEvent(display_, -1, x, y, CurrentTime);
        flush();
    }
    
    inline void fake_mouse(uint8_t button, bool is_press)
    {
        XTestFakeButtonEvent(display_, button, is_press, CurrentTime);
    }
    
    void press_mouse(mouse_btn_type type)
    {
        fake_mouse(type, true);
        flush();
    }
    
    void release_mouse(mouse_btn_type type)
    {
        fake_mouse(type, false);
        flush();
    }
    
    void click(mouse_btn_type type)
    {
        fake_mouse(type, true);
        fake_mouse(type, false);
        flush();
    }

    void wheel_up()
    {
        fake_mouse(Button4, true);
        fake_mouse(Button4, false);
        flush();
    }
    
    void wheel_down()
    {
        fake_mouse(Button5, true);
        fake_mouse(Button5, false);
        flush();
    }
}
