
#define VGA_TEXT_DISPLAY 0xb8000
#define VGA_TEXT_COLUMNS_PER_LINE 80 // TODO(alexander): this is hardcoded for now

#define VGA_CURSOR_FIELD_PORT 0x3D4
#define VGA_CURSOR_VALUE_PORT 0x3D5

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

#define str_count(s) *((int*) s - 1)

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

void
puts(cstr message, int length, char formatting) {
    char* vga = (char*) VGA_TEXT_DISPLAY + global_cursor_position*2;
    char* curr_character = (char*) message;
    for (int i = 0; i < length; i++) {
        char character = *curr_character++;
        switch (character) {
            case '\0': return;
            case '\r': continue;
            
            case '\n': {
                global_cursor_position += VGA_TEXT_COLUMNS_PER_LINE;
                global_cursor_position -= global_cursor_position % VGA_TEXT_COLUMNS_PER_LINE;
            } break;
            
            default: {
                *vga++ = character; // print character
                *vga++ = formatting; // text formatting
                global_cursor_position++;
            }
        }
    }
    
    cursor_set_position(global_cursor_position);
}

int
cstr_count(cstr s) {
    int count = 0;
    while (*s++ != 0) count++;
    return count;
}

extern void halt(void);

int main() {
    const char color = 0x1f;
    cstr hello = "Hello Kernel from C\n\r";
    cursor_set_position(80);
    puts(hello, cstr_count(hello), color);
    cursor_disable();
    cursor_enable(0, 15);
    
    halt();
    return 0;
}
