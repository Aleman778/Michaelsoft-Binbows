
#include "kernel.h"
#include <stdarg.h>


// TODO(alexander): intrinsics should be moved into platform specific codebase
void
outb(u16 port, u8 value) {
    asm volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

u8
inb(u16 port) {
    u8 result;
    asm volatile ("inb %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

// TODO(alexander): move into cursor file maybe?
static u16 global_cursor_position;
static char global_formatting = VGA_BG_BLACK | VGA_FG_WHITE;

void
cursor_enable(u8 begin_scanline, u8 end_scanline) {
    outb(VGA_CURSOR_FIELD_PORT, 0x0A);
	outb(VGA_CURSOR_VALUE_PORT, (inb(VGA_CURSOR_VALUE_PORT) & 0xC0) | begin_scanline);
    
	outb(VGA_CURSOR_FIELD_PORT, 0x0B);
	outb(VGA_CURSOR_VALUE_PORT, (inb(VGA_CURSOR_VALUE_PORT) & 0xE0) | end_scanline);
}

void
cursor_disable() {
    outb(VGA_CURSOR_FIELD_PORT, 0x0A);
    outb(VGA_CURSOR_VALUE_PORT, 0x20);
}

void
cursor_set_position(u16 position) {
    outb(VGA_CURSOR_FIELD_PORT, 0x0F);
    outb(VGA_CURSOR_VALUE_PORT, position & 0xFF);
    outb(VGA_CURSOR_FIELD_PORT, 0x0E);
    outb(VGA_CURSOR_VALUE_PORT, (position >> 8) & 0xFF);
    global_cursor_position = position;
}

void
cursor_set_position_xy(u16 x, u16 y) {
    u16 position = VGA_TEXT_COLUMNS_PER_LINE * y + x;
    cursor_set_position(position);
}

#define puts(message, count) puts_formatted(message, count, global_formatting)


internal inline void
put_newline() {
    global_cursor_position += VGA_TEXT_COLUMNS_PER_LINE;
    global_cursor_position -= global_cursor_position % VGA_TEXT_COLUMNS_PER_LINE;
}

void
puts_formatted(cstring message, int count, char formatting) {
    char* vga = (char*) VGA_TEXT_DISPLAY + global_cursor_position*2;
    char* curr_character = (char*) message;
    for (int i = 0; i < count; i++) {
        char character = *curr_character++;
        switch (character) {
            case '\0': return;
            case '\r': continue;
            
            case '\n': {
                put_newline();
            } break;
            
            default: {
                *vga++ = character;
                *vga++ = formatting;
                global_cursor_position++;
            }
        }
    }
    
    cursor_set_position(global_cursor_position);
}

#define putc(c) putc_formatted(c, global_formatting);

void
putc_formatted(char character, char formatting) {
    if (character == '\0' || character == '\r') {
        return;
    }
    
    if (character == '\n') {
        global_cursor_position += VGA_TEXT_COLUMNS_PER_LINE;
        global_cursor_position -= global_cursor_position % VGA_TEXT_COLUMNS_PER_LINE;
    } else {
        char* vga = (char*) VGA_TEXT_DISPLAY + global_cursor_position*2;
        *vga++ = character;
        *vga++ = formatting;
        global_cursor_position++;
    }
}


char
to_uppercase_char(char c) {
    if (c >= 'a' && c <= 'z') c = 'A' + (c - 'a');
    return c;
}

char hex_to_string_buffer[32];
char hex_to_char_lower[] = "0123456789abcdf";
char hex_to_char_upper[] = "0123456789ABCDF";

// TODO(alexander): this is a very hacky implementation
string
format_u64(u64 value, u64 radix, char* digit_to_char) {
    string result;
    result.count = 0;
    
    int buffer_count = 32 - 5; // null chr + u32 size.
    char* buffer = hex_to_string_buffer + 31;
    *buffer-- = '\0';
    for (int i = 0; i < buffer_count; i++) {
        u64 digit = value % radix;
        value /= radix;
        *buffer-- = digit_to_char[digit];
        result.count++;
        
        if (value == 0) {
            break;
        }
    }
    buffer++;
    
    result.data = (u8*) buffer;
    return result;
}

void
print_u64(u64 value, u64 radix, char* digit_to_char) {
    string s = format_u64(value, radix, digit_to_char);
    puts((char*) s.data, s.count);
}

typedef enum {
    PrintfState_Normal,
    PrintfState_Length,
    PrintfState_Length_Short,
    PrintfState_Length_Long,
    PrintfState_Specifier,
} Printf_State;

typedef enum {
    PrintfLength_Default,
    PrintfLength_Short,
    PrintfLength_Short_Short,
    PrintfLength_Long,
    PrintfLength_Long_Long,
    PrintfLength_Arch,
} Printf_Length;

void
printf(cstring fmt, ...) {
    va_list args;
    va_start(args, fmt);
    
    Printf_State state = PrintfState_Normal;
    Printf_Length length = PrintfLength_Default;
    
    char* curr = (char*) fmt;
    while (*curr) {
        char c = *curr;
        
        switch (state) {
            case PrintfState_Normal: {
                if (c == '%') {
                    if (*(curr + 1) == '%') {
                        curr++;
                        putc('%');
                    } else {
                        state = PrintfState_Length;
                    }
                } else if (c == '\\') {
                    char c2 = *curr + 1;
                    switch (c2) {
                        case 'n': {
                            put_newline();
                            curr++;
                        } break;
                        
                        case 'r': {
                            curr++;
                        } break;
                        
                        case '0': {
                            *(curr + 1) = '\0';
                        } break;
                        
                        default: {
                            assert(0 && "invalid escape character");
                        } break;
                    }
                } else {
                    putc(c);
                }
            } break;
            
            case PrintfState_Length: {
                switch (c) {
                    // Bit size modifiers
                    case 'h': {
                        length = PrintfLength_Short;
                    } break;
                    
                    case 'l': {
                        length = PrintfLength_Long;
                    } break;
                    
                    case 'z': {
                        length = PrintfLength_Arch;
                        state = PrintfState_Specifier;
                    } break;
                    
                    default: {
                        state = PrintfState_Specifier;
                    } continue;
                } break;
            } break;
            
            case PrintfState_Length_Short: {
                
                state = PrintfState_Specifier;
                if (c == 'h') {
                    length = PrintfLength_Short_Short;
                } else {
                    continue;
                }
            } break;
            
            case PrintfState_Length_Long: {
                
                state = PrintfState_Specifier;
                if (c == 'l') {
                    length = PrintfLength_Long_Long;
                } else {
                    continue;
                }
            } break;
            
            case PrintfState_Specifier: {
                u64 radix = 10;
                char* digits = hex_to_char_lower;
                
                switch (c) {
                    case 'i':
                    case 'd': {
                        //puts("specifier", 10);
                        s64 value = 0;
                        switch (length) {
                            case PrintfLength_Short_Short: {
                                case PrintfLength_Default:
                                case PrintfLength_Short:
                                case PrintfLength_Long: {
                                    value = va_arg(args, s32);
                                } break;
                                
                                case PrintfLength_Long_Long: {
                                    value = va_arg(args, s64);
                                } break;
                                
                                case PrintfLength_Arch: {
                                    value = va_arg(args, smm);
                                } break;
                            }
                        }
                        
                        if (value < 0) {
                            putc('-');
                            print_u64((u64) -value, radix, digits);
                        } else {
                            print_u64((u64) value, radix, digits);
                        }
                    } break;
                    
                    case 'X': digits = hex_to_char_upper;
                    // fall through
                    case 'x': radix = 16;
                    // fall through
                    case 'u': {
                        s64 value = 0;
                        switch (length) {
                            case PrintfLength_Short_Short: {
                                case PrintfLength_Default:
                                case PrintfLength_Short:
                                case PrintfLength_Long: {
                                    value = va_arg(args, u32);
                                } break;
                                
                                case PrintfLength_Long_Long: {
                                    value = va_arg(args, u64);
                                } break;
                                
                                case PrintfLength_Arch: {
                                    value = va_arg(args, umm);
                                } break;
                            }
                        }
                        
                        print_u64(value, radix, digits);
                    } break;
                    
                    case 's': {
                        cstring s = va_arg(args, cstring);
                        puts(s, cstring_count(s));
                    } break;
                    
                    case 'c': {
                        putc(va_arg(args, int));
                    } break;
                    
                    case 'C': {
                        putc(to_uppercase_char(va_arg(args, int)));
                    } break;
                }
                
                state = PrintfState_Normal;
                length = PrintfLength_Default;
            } break;
            
        }
        
        curr++;
    }
    
    va_end(args);
}

typedef struct {
    u16 offset_low;
    u16 selector;
    u8 ist;
    u8 types_attr;
    u16 offset_mid;
    u32 offset_high;
    u32 zero;
} IDT64;

extern IDT64 _idt[256];

// declared in kernel_loader.asm
extern u64 isr1;
extern void load_idt();
extern void halt();

void
initialize_idt() {
    for (u64 index = 0; index < fixed_array_count(_idt); index++) {
        _idt[index].zero = 0;
        _idt[index].ist = 0;
        _idt[index].selector = 0x08;
        _idt[index].types_attr = 0x8E;
        _idt[index].offset_low  = (u16) (((u64) &isr1 & 0x000000000000FFFF));
        _idt[index].offset_mid  = (u16) (((u64) &isr1 & 0x00000000FFFF0000) >> 16);
        _idt[index].offset_high = (u32) (((u64) &isr1 & 0xFFFFFFFF00000000) >> 32);
    }
    
    // NOTE(alexander): for masking the PIC chip
    outb(0x21, 0xFD);
    outb(0xA1, 0xFF);
    load_idt();
}

const char ps2_keyboard_scancode_set1[] ={
    0, 0, '1', '2',
    '3', '4', '5', '6',
    '7', '8', '9', '0',
    '-', '=', 0, 0,
    'q', 'w', 'e', 'r',
    't', 'y', 'u', 'i',
    'o', 'p', '[', ']',
    0, 0, 'a', 's',
    'd', 'f', 'g', 'h',
    'j', 'k', 'l', ';',
    '\'', '`', 0, '\\',
    'z', 'x', 'c', 'v',
    'b', 'n', 'm', ',',
    '.', '/', 0, '*',
    0, ' '
};

void 
isr1_handler() {
    u8 scancode = inb(0x60);
    if (scancode < fixed_array_count(ps2_keyboard_scancode_set1)) {
        char chr = ps2_keyboard_scancode_set1[scancode];
        const char formatting = VGA_BG_BLUE | VGA_FG_WHITE;
        puts_formatted(&chr, 1, formatting);
    }
    
    outb(0x20, 0x20);
    outb(0xa0, 0x20);
}

int 
main() {
    cursor_set_position(0);
    initialize_idt();
    
    //const char formatting = VGA_BG_BLUE | VGA_FG_WHITE;
    //cstring hello = "Hello Kernel from C\n\r";
    //puts_formatted(hello, cstring_count(hello), formatting);
    
    // Just a test case for printf
    printf("Characters: %c %c \n", 'a', 65);
    printf("String: %s \n", "Hello World!");
    printf("Decimals: %d %ld\n", 1977, 650000L);
    printf("Some different radices: %d %x 0x%x \n", 100, 100, 100);
    
    
    //cursor_set_position(80*2);
    
    //str s = hex_to_string(0x123456789ABCDEF);
    //puts(s, str_count(s));
    
    // Prevent the computer from exiting
    for (;;) {
    }
    
    return 0;
}
