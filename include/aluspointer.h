#ifndef ALUSPOINTER_H
#define ALUSPOINTER_H

#include <X11/Xutil.h>
#include <tinyutf8/tinyutf8.h>
#include <string>
#include <vector>
#include <thread>
#include <functional>

namespace aluspointer
{
    enum mouse_btn_type
    {
        MOUSE_LEFT          = Button1,
        MOUSE_RIGHT         = Button2,
        MOUSE_MIDDLE        = Button3
    };
    
    enum flags_type
    {
        MOD_NONE            = 0x00, 
        MOD_SHIFT           = 0x01,
        MOD_CAPSLOCK        = 0x02,
        MOD_CTRL            = 0x04
    };
    
    enum key_type : KeySym
    {
        Key_Left             = XK_Left,
        Key_Up               = XK_Up,
        Key_Right            = XK_Right,
        Key_Down             = XK_Down
    };
    
    // Call this before everything
    void initialize();
    
    // Keyboard Control
    void tap_key(key_type /*type*/);
    void tap_key(char /*ascii*/);
    void tap_key_in_utf8(char32_t /*codepoint*/);
    void type_string(const tiny_utf8::string& /*str*/);
    
    // Mouse Control
    void move_mouse(int /*x*/, int /*y*/);
    void move_mouse_to(int /*x*/, int /*y*/);
    void press_mouse(mouse_btn_type /*type*/);
    void release_mouse(mouse_btn_type /*type*/);
    void click(mouse_btn_type /*type*/);
    void wheel_up();
    void wheel_down();
    
    // Window Management
    struct window_client_t
    {
        int id = 0;
        std::string name;
    };
    std::vector<window_client_t> update_window_list();
    void focus_window(uint8_t id);
    void minimize_window(uint8_t id);
    void toggle_window(uint8_t id);
    const std::vector<unsigned char> get_window_image(uint8_t id);
    void start_listen_to_updated_windows(std::function<void(const int)> callback);
    void listen_to_window(int id);
}

#endif // ALUSPOINTER_H
