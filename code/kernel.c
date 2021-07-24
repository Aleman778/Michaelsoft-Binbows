
#define VGA_TEXT_DISPLAY 0xb8000
#define VGA_TEXT_COLS_PER_LINE 80 // TODO(alexander): this is hardcoded for now

typedef const char* cstr;
typedef char* str;

#define str_count(s) *((int*) s - 1)

void
puts(cstr message, int length, char formatting) {
    char* vga = (char*) (VGA_TEXT_DISPLAY + 80*2);
    char* curr_character = (char*) message;
    for (int i = 0; i < length; i++) {
        *vga++ = *curr_character++; // print character
        *vga++ = formatting; // text formatting
    }
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
    cstr hello = "Hello Kernel from C";
    puts(hello, cstr_count(hello), color);
    
    halt();
    return 0;
}