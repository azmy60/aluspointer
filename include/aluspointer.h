#ifndef ALUSPOINTER_H
#define ALUSPOINTER_H

#include <xcb/xcb_keysyms.h>
#include <tinyutf8/tinyutf8.h>

namespace aluspointer
{
    enum mouse_btn_type
    {
        MOUSE_LEFT          = XCB_BUTTON_INDEX_1,
        MOUSE_RIGHT         = XCB_BUTTON_INDEX_3,
        MOUSE_MIDDLE        = XCB_BUTTON_INDEX_2
    };
    
    enum flags_type
    {
        MOD_NONE            = 0x00, 
        MOD_SHIFT           = 0x01,
        MOD_CAPSLOCK        = 0x02,
        MOD_CTRL            = 0x04
    };
    
    enum key_type : xcb_keysym_t
    {
        XK_Left             = 0xff51,
        XK_Up               = 0xff52,
        XK_Right            = 0xff53,
        XK_Down             = 0xff54
    };
    
    void initialize();
    
    // Keyboard Control
    void tap_key(key_type /*type*/);
    void tap_key(char /*ascii*/);
    void tap_key_in_utf8(char32_t /*codepoint*/);
    void type_string(const tiny_utf8::string& /*str*/);
    
    // Mouse Control
    void move_mouse(int /*x*/, int /*y*/);
    void press_mouse(mouse_btn_type /*type*/);
    void release_mouse(mouse_btn_type /*type*/);
    void click(mouse_btn_type /*type*/);
    void wheel_up();
    void wheel_down();
}

#endif // ALUSPOINTER_H
