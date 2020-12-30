# aluspointer
aluspointer is an input simulation library written in c++, originally made for a dependency of my other project [Mobile Cursor](https://github.com/azmy60/mobile-cursor). Currently, it only supports linux platform.

### Features
- Basic mouse and keyboard tasks (clicking, typing, etc.)
- Supports UTF-8 characters typing

### Dependencies
- xcb, xcb-xtest, and xcb-keysyms
- tinyutf8 (https://github.com/DuffsDevice/tiny-utf8)

### Usage
```C++
#include <aluspointer.h>

int main()
{
  // Initialize before use
  aluspointer::initialize();
  
  // Move the mouse pointer in (x, y) relative to its position
  aluspointer::move_mouse(3, 4);
  
  // Perform a left click 
  aluspointer::click(MOUSE_LEFT);
  
  // Type UTF-8 string
  aluspointer::type_string(u8"I type you these emojis! 🔥🔥💯💯😂");
  
  // Tap ASCII character
  aluspointer::tap_key(15); // ASCII code for return
  
  return 0;
}
```

### TODO
- Adds window management support. Such as activating, minimizing, capturing, and other actions on windows
- Windows and Mac support
