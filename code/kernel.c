
#define VGA_TEXT_DISPLAY 0xb8000
#define VGA_TEXT_COLUMNS_PER_LINE 80 // TODO(alexander): this is hardcoded for now

#define VGA_CURSOR_FIELD_PORT 0x3D4
#define VGA_CURSOR_VALUE_PORT 0x3D5

#define VGA_FG_BLACK 0x00
#define VGA_FG_BLUE 0x01
#define VGA_FG_GREEN 0x02
#define VGA_FG_CYAN 0x03
#define VGA_FG_RED 0x04
#define VGA_FG_MAGENTA 0x05
#define VGA_FG_BROWN 0x06
#define VGA_FG_LIGHTGRAY 0x07
#define VGA_FG_DARKGRAY 0x08
#define VGA_FG_LIGHTBLUE 0x09
#define VGA_FG_LIGHTGREEN 0x0A
#define VGA_FG_LIGHTCYAN 0x0B
#define VGA_FG_LIGHTRED 0x0C
#define VGA_FG_LIGHTMAGENTA 0x0D
#define VGA_FG_YELLOW 0x0E
#define VGA_FG_WHITE 0x0F

#define VGA_BG_BLACK 0x00
#define VGA_BG_BLUE 0x10
#define VGA_BG_GREEN 0x20
#define VGA_BG_CYAN 0x30
#define VGA_BG_RED 0x40
#define VGA_BG_MAGENTA 0x50
#define VGA_BG_BROWN 0x60
#define VGA_BG_LIGHTGRAY 0x70
#define VGA_BG_BLINKING_BLACK 0x80
#define VGA_BG_BLINKING_BLUE 0x90
#define VGA_BG_BLINKING_GREEN 0xA0
#define VGA_BG_BLINKING_CYAN 0xB0
#define VGA_BG_BLINKING_RED 0xC0
#define VGA_BG_BLINKING_MAGENTA 0xD0
#define VGA_BG_BLINKING_BROWN 0xE0
#define VGA_BG_BLINKING_LIGHTGRAY 0xF0

typedef unsigned int           uint;
typedef signed   char          s8;
typedef unsigned char          u8;
typedef signed   short         s16;
typedef unsigned short         u16;
typedef signed   int           s32;
typedef unsigned int           u32;
typedef signed   long long int s64;
typedef unsigned long long int u64;
typedef const char*            cstr; // TODO(alexander): str instead for safety
typedef char*                  str;

#define true 1
#define false 0

#define array_count(array) (sizeof(array) / sizeof((array)[0]))
#define str_count(s) *((u32*) s - 1)

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

#define puts(message, count) puts_formatted(message, count, VGA_BG_BLACK | VGA_FG_WHITE)

void
puts_formatted(cstr message, int count, char formatting) {
    char* vga = (char*) VGA_TEXT_DISPLAY + global_cursor_position*2;
    char* curr_character = (char*) message;
    for (int i = 0; i < count; i++) {
        char character = *curr_character++;
        switch (character) {
            case '\0': return;
            case '\r': continue;
            
            case '\n': {
                global_cursor_position += VGA_TEXT_COLUMNS_PER_LINE;
                global_cursor_position -= global_cursor_position % VGA_TEXT_COLUMNS_PER_LINE;
            } break;
            
            default: {
                *vga++ = character;
                *vga++ = formatting;
                //character = 'A';
                //*((char*) VGA_TEXT_DISPLAY + i*2) = character;
                //*((char*) VGA_TEXT_DISPLAY + global_cursor_position*2) = character; // print character
                //*((char*) VGA_TEXT_DISPLAY + global_cursor_position*2 + 1) = formatting; // text formatting
                global_cursor_position++;
            }
        }
    }
    
    cursor_set_position(global_cursor_position);
}

// NOTE(alexander): this is a very hacky implementation
char hex_to_string_buffer[32];
str hex_to_string(u64 value) {
    int buffer_count = 32 - 5; // null chr + u32 size.
    char* buffer = hex_to_string_buffer + 31;
    *buffer-- = '\0';
    u32 count = 0;
    for (int i = 0; i < buffer_count; i++) {
        char digit = (char) (value % 16);
        value /= 16;
        if (digit < 10) {
            *buffer-- = digit + '0';
        } else if (digit < 16)  {
            *buffer-- = (digit - 10) + 'A';
        }
        count++;
        
        if (value == 0) {
            break;
        }
    }
    buffer++;
    
    *((u32*) buffer - 1) = count;
    return (str) (buffer);
}

int
cstr_count(cstr s) {
    int count = 0;
    while (*s++ != 0) count++;
    return count;
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
    for (u64 index = 0; index < array_count(_idt); index++) {
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

void 
isr1_handler() {
    str scancode = hex_to_string(inb(0x60));
    puts(scancode, cstr_count(scancode));
    outb(0x20, 0x20);
    outb(0xa0, 0x20);
}

int main() {
    cursor_set_position(0);
    initialize_idt();
    
    const char formatting = VGA_BG_BLUE | VGA_FG_WHITE;
    cstr hello = "Hello Kernel from C\n\r\0";
    puts_formatted(hello, cstr_count(hello), formatting);
    
    
    //const char* message = "test";
    //u32 count = 4;
    //char* vga = (char*) VGA_TEXT_DISPLAY;
    //char* curr_character = (char*) message;
    //for (u32 i = 0; i < count; i++) {
    //char character = *curr_character++;
    //switch (character) {
    //case '\r': continue;
    //
    //case '\n': {
    //global_cursor_position += VGA_TEXT_COLUMNS_PER_LINE;
    //global_cursor_position -= global_cursor_position % VGA_TEXT_COLUMNS_PER_LINE;
    //} break;
    //
    //default: {
    //*vga++ = character;
    //*vga++ = formatting;
    //character = 'A';
    //*((char*) VGA_TEXT_DISPLAY + i*2) = character;
    //*((char*) VGA_TEXT_DISPLAY + global_cursor_position*2) = character; // print character
    //*((char*) VGA_TEXT_DISPLAY + global_cursor_position*2 + 1) = formatting; // text formatting
    //global_cursor_position++;
    //} break;
    //}
    //}
    
    cursor_set_position(80);
    
    //str s = hex_to_string(0x123456789ABCDEF);
    //puts(s, str_count(s));
    
    for (;;) {
    }
    
    return 0;
}
