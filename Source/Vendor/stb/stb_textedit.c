#define STB_TEXTEDIT_IMPLEMENTATION

#define STB_TEXTEDIT_CHARTYPE             char
#define STB_TEXTEDIT_POSITIONTYPE         int
#define STB_TEXTEDIT_UNDOSTATECOUNT       16
#define STB_TEXTEDIT_UNDOCHARCOUNT        16

// Symbols you must define for implementation mode:

#define STB_TEXTEDIT_STRING               std::string

#define STB_TEXTEDIT_STRINGLEN(obj)       obj.size()
#define STB_TEXTEDIT_LAYOUTROW(&r,obj,n)  returns the results of laying out a line of characters
//                                        starting from character #n (see discussion below)
#define STB_TEXTEDIT_GETWIDTH(obj,n,i)    returns the pixel delta from the xpos of the i-1'th
//                                        character to the i'th char for a line of characters
//                                        starting at character #n (i.e. accounts for kerning
//                                        with previous char)
#define STB_TEXTEDIT_KEYTOTEXT(k)         maps a keyboard input to an insertable character
//                                        (return type is int, -1 means not valid to insert)
#define STB_TEXTEDIT_GETCHAR(obj,i)       obj[i]
#define STB_TEXTEDIT_NEWLINE              '\n'
//
#define STB_TEXTEDIT_DELETECHARS(obj,i,n)      (obj = obj.substr(0, i) + obj.substr(i+n))
#define STB_TEXTEDIT_INSERTCHARS(obj,i,c*,n)   (obj = obj.substr(0, i) + c + std::string(i+n))
//
#define STB_TEXTEDIT_K_SHIFT       0x8000
//
#define STB_TEXTEDIT_K_LEFT        VK_LEFT
#define STB_TEXTEDIT_K_RIGHT       VK_RIGHT
#define STB_TEXTEDIT_K_UP          VK_UP
#define STB_TEXTEDIT_K_DOWN        VK_DOWN
#define STB_TEXTEDIT_K_LINESTART   VK_HOME
#define STB_TEXTEDIT_K_LINEEND     VK_END
#define STB_TEXTEDIT_K_TEXTSTART   (VK_CTRL | VK_HOME)
#define STB_TEXTEDIT_K_TEXTEND     (VK_CTRL | VK_END)
#define STB_TEXTEDIT_K_DELETE      VK_DELETE
#define STB_TEXTEDIT_K_BACKSPACE   VK_BACKSPACE
#define STB_TEXTEDIT_K_UNDO        (VK_CTRL | VK_Z)
#define STB_TEXTEDIT_K_REDO        (VK_CTRL | VK_Y)

// Optional:
#define STB_TEXTEDIT_K_INSERT      VK_INSERT
#define STB_TEXTEDIT_IS_SPACE(ch)  (ch == ' ')

#define STB_TEXTEDIT_K_WORDLEFT    (VK_CTRL | VK_LEFT)
#define STB_TEXTEDIT_K_WORDRIGHT   (VK_CTRL | VK_RIGHT)

#include "stb_textedit.h"
