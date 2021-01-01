#include "aluspointer.h"
#include "util.h"
#include <string>
#include <memory>
#include <iostream>
#include <algorithm>
#include <xcb/xcb.h>
#include <xcb/xtest.h>

// TODO use enum
#define INIT_CONNECTION_ERR 0
#define INIT_SCREEN_ERR     1

namespace aluspointer
{
    struct ConnectionDeleter
    {
        void operator()(xcb_connection_t *p) const
        {
            if(p) xcb_disconnect(p);
        }
    };
    
    struct KeySymbolsDeleter
    {
        void operator()(xcb_key_symbols_t *p) const
        {
            if(p) xcb_key_symbols_free(p);
        }
    };
    
    /*
    using keysyms_ptr = std::unique_ptr<xcb_keysym_t[]>;
    using keymap_reply_ptr = std::unique_ptr<xcb_get_keyboard_mapping_reply_t>;
    using setup_ptr = std::unique_ptr<const xcb_setup_t>;
    */
    using connection_ptr = std::unique_ptr<xcb_connection_t, ConnectionDeleter>;
    using screen_ptr = std::unique_ptr<xcb_screen_t>;
    using key_symbols_ptr = std::unique_ptr<xcb_key_symbols_t, KeySymbolsDeleter>;
    
    connection_ptr scoped_connection;
    screen_ptr screen;
    key_symbols_ptr key_symbols;
    
    int n_screen_default;
    xcb_connection_t *connection;
    
    
    /*
    setup_ptr setup;

    keysyms_ptr get_keyboard_mapping(int &keysyms_per_keycode)
    {
        xcb_get_keyboard_mapping_reply_t *keymap = xcb_get_keyboard_mapping_reply(
            connection,
            xcb_get_keyboard_mapping(connection, setup->min_keycode, setup->max_keycode - setup->min_keycode),
            NULL
        );
        keysyms_per_keycode = keymap->keysyms_per_keycode;
        return keysyms_ptr(xcb_get_keyboard_mapping_keysymskeymap));
    }
    
    
    xcb_keysym_iterator_t get_keyboard_mapping_iterator()
    {
        xcb_get_keyboard_mapping_reply_t *keymap = xcb_get_keyboard_mapping_reply(
            connection,
            xcb_get_keyboard_mapping(connection, setup->min_keycode, setup->max_keycode - setup->min_keycode),
            NULL
        );
        return {xcb_get_keyboard_mapping_keysyms(keymap), setup->max_keycode, setup->min_keycode};
    }
    */
    
    inline xcb_screen_t *screen_of_display(xcb_connection_t *c, int screen)
    {
        for (auto it = xcb_setup_roots_iterator(xcb_get_setup(c)); 
            it.rem;
            --screen, xcb_screen_next(&it))
            if (screen == 0)
                return it.data;

        return NULL;
    }
    
    void initialize()
    {
        connection = xcb_connect(NULL, &n_screen_default);
        if(connection == NULL)
        {
            std::cerr << "Unable to open display default.\n";
            throw INIT_CONNECTION_ERR;
        }
        scoped_connection = connection_ptr(connection);
        
        screen = screen_ptr(screen_of_display(connection, n_screen_default));
        if(!screen)
        {
            std::cerr << "Unable to get default screen.\n";
            throw INIT_SCREEN_ERR;
        } 
        
        key_symbols = key_symbols_ptr(xcb_key_symbols_alloc(connection));
        
        /*
        setup = setup_ptr(xcb_get_setup(connection));
        
        keysyms = keysyms_ptr((xcb_keysym_t*)(keymap.get() + 1));

        std::unique_ptr<xcb_mapping_notify_event_t> event(new xcb_mapping_notify_event_t());
        event->request = XCB_MAPPING_KEYBOARD;
        xcb_refresh_keyboard_mapping(key_symbols.get(), event.get());
        */
    }
    
    inline void flush()
    {
        xcb_flush(connection);
    }
    
    // WARNING: flush manually.
    inline void send_fake_input(uint8_t type, xcb_keycode_t keycode)
    {
        /*
        auto cookie = xcb_test_fake_input_checked(connection, type, keycode, XCB_CURRENT_TIME, XCB_NONE, 0, 0, 0);
        std::unique_ptr<xcb_generic_error_t> err(xcb_request_check(connection, cookie));
        if(err)
            std::cerr << "Error sending fake input: " << err->error_code << std::endl;
        */
        
        xcb_test_fake_input(connection, type, keycode, XCB_CURRENT_TIME, XCB_NONE, 0, 0, 0);
    }

    enum XK_KeySym
    {
        XK_Shift_L          = 0xffe1,
        XK_Control_L        = 0xffe3,
        XK_Return           = 0xff0d,
        XK_Backspace        = 0xff08,
    //  XK_Space            = 0x0020
    };
    
    inline xcb_keycode_t get_keycode(xcb_keysym_t keysym)
    {
        return *xcb_key_symbols_get_keycode(key_symbols.get(), keysym);
    }

    // WARNING: release manually. If uses flags, release with the same flags.
    // WARNING: flush manually.
    void press_key(xcb_keycode_t keycode, int flags)
    {
        if(flags & MOD_SHIFT)
            send_fake_input(XCB_KEY_PRESS, get_keycode(XK_Shift_L));
        
        if(flags & MOD_CTRL)
            send_fake_input(XCB_KEY_PRESS, get_keycode(XK_Control_L));
        
        // TODO flags capslock
        
        send_fake_input(XCB_KEY_PRESS, keycode);
    }

    // WARNING: call this when pressing the keycode previously.
    // WARNING: flush manually.
    void release_key(xcb_keycode_t keycode, int flags)
    {
        if(flags & MOD_SHIFT)
            send_fake_input(XCB_KEY_RELEASE, get_keycode(XK_Shift_L));
        
        if(flags & MOD_CTRL)
            send_fake_input(XCB_KEY_RELEASE, get_keycode(XK_Control_L));
        
        // TODO flags capslock
        
        send_fake_input(XCB_KEY_RELEASE, keycode);
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
        
        xcb_keysym_t keycode;
        
        if(ascii >= 32 && ascii <= 126)
            keycode = get_keycode(ascii);
        else if(ascii == 8) // backspace
            keycode = get_keycode(XK_Backspace);
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
        // Alternatively: xcb_warp_pointer(connection, XCB_NONE, XCB_NONE, 0, 0, 0, 0, x, y);
        // (connection, XCB_MOTION_NOTIFY, is_relative?, time, root, x, y, device_id)
        xcb_test_fake_input(connection, XCB_MOTION_NOTIFY, 1, XCB_CURRENT_TIME, XCB_NONE, x, y, 0);
        flush();        
    }
    
    void move_mouse_to(int x, int y)
    {
        xcb_test_fake_input(connection, XCB_MOTION_NOTIFY, 0, XCB_CURRENT_TIME, XCB_NONE, x, y, 0);
        flush();        
    }
    
    inline void fake_mouse(uint8_t state, uint8_t index)
    {
        xcb_test_fake_input(connection, state, index, XCB_CURRENT_TIME, XCB_NONE, 0, 0, 0);
    }
    
    void press_mouse(mouse_btn_type type)
    {
        fake_mouse(XCB_BUTTON_PRESS, type);
        flush();
    }
    
    void release_mouse(mouse_btn_type type)
    {
        fake_mouse(XCB_BUTTON_RELEASE, type);
        flush();
    }
    
    void click(mouse_btn_type type)
    {
        fake_mouse(XCB_BUTTON_PRESS, type);
        fake_mouse(XCB_BUTTON_RELEASE, type);
        flush();
    }

    void wheel_up()
    {
        fake_mouse(XCB_BUTTON_PRESS, XCB_BUTTON_INDEX_4);
        fake_mouse(XCB_BUTTON_RELEASE, XCB_BUTTON_INDEX_4);
        flush();
    }
    
    void wheel_down()
    {
        fake_mouse(XCB_BUTTON_PRESS, XCB_BUTTON_INDEX_5);
        fake_mouse(XCB_BUTTON_RELEASE, XCB_BUTTON_INDEX_5);
        flush();
    }
    
    
    /* Steps to type in unicode with keymapping:
     * 1) Get the keyboard mapping.
     * 2) Find an unused keycode. 
     * 3) Make a list of 1 KeySym containing the codepoint.
     * 3) Change the keyboard mapping of the unused keycode for the list of the KeySym.
     * 4) Flush the connection.
     * 5) Send the key with the keycode.
     * 6) Flush it!
     * 7) Don't forget to revert the keyboard mapping with a list of one 0.
     * 
     * If you type a string, you only need to repeat from step 3 to 6. Use the same keycode
     * to map all different keysyms, so you only send the same keycode.
     * 
    // SLOW n BROKEN! (i sent 2 emojis and it only wrote 1 ..) (CTRL+SHIFT+U is faster but not guaranteed to work on top of all apps)
    void tap_string_with_keymap(const tiny_utf8::string &str)
    {
        // Got from https://github.com/jordansissel/xdotool/blob/master/xdo.c

        bool keymap_is_changed = false;
        int scratch_keycode = 0;
        
        for(auto it = get_keyboard_mapping_iterator(); it.rem; xcb_keysym_next(&it))
        {
            if(*it.data == 0)
            {
                scratch_keycode = it.index;
                break;
            }
        }
        
        std::for_each(str.begin(), str.end(), [&](char32_t codepoint)
        {
            xcb_keycode_t keycode;
            int flags = ALUS_MOD_NONE;
            bool not_ascii = false;
            
            if(codepoint < 0x80)
            {
                const std::string cap("~!@#$%^&*()_+{}|:\"<>?");
                
                if ((codepoint >= 'A' && codepoint <= 'Z') || (cap.find(codepoint) != std::string::npos))
                    flags |= ALUS_MOD_SHIFT;
                    
                keycode = *xcb_key_symbols_get_keycode(key_symbols.get(), (xcb_keysym_t) codepoint);
            }
            else
            {
                keysyms_ptr keysyms(new xcb_keysym_t[1]{ codepoint | 0x01000000 }); // http://git.yoctoproject.org/cgit/cgit.cgi/libfakekey/tree/src/libfakekey.c#n424
                
                xcb_void_cookie_t cookie = xcb_change_keyboard_mapping_checked(connection, 1, scratch_keycode, 1, keysyms.get());
                xcb_flush(connection);
                
                std::unique_ptr<xcb_generic_error_t> err(xcb_request_check(connection, cookie));
                if(err)
                {
                    std::cerr << "Error changing keyboard map: " << err->error_code << std::endl;
                    return;
                }
                
                keycode = scratch_keycode;
                keymap_is_changed = true;
                not_ascii = true;
            }
            
            press_key(keycode, flags);
            xcb_flush(connection);

            release_key(keycode, flags);
            xcb_flush(connection);
            
            if(not_ascii) xcb_flush(connection);
        });
        
        if(keymap_is_changed)
        {
            keysyms_ptr keysyms(new xcb_keysym_t[1]{ 0 });
            auto cookie = xcb_change_keyboard_mapping_checked(connection, 1, scratch_keycode, 1, keysyms.get());
            std::unique_ptr<xcb_generic_error_t> err(xcb_request_check(connection, cookie));
            if(err)
                std::cerr << "Error reverting keyboard map: " << err->error_code << std::endl;
        }
        
        xcb_flush(connection);
    }
    */
}
