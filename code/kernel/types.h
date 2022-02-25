
// Basic types
#ifndef SMM_TYPE
#define SMM_TYPE s64
#endif


#ifndef UMM_TYPE
#define UMM_TYPE u64
#endif

typedef unsigned int           uint;
typedef signed   char          s8;
typedef signed   short         s16;
typedef signed   int           s32;
typedef signed   long long int s64;
typedef SMM_TYPE               smm;
typedef unsigned char          u8;
typedef unsigned short         u16;
typedef unsigned int           u32;
typedef unsigned long long int u64;
typedef UMM_TYPE               umm;
typedef const char*            cstring;

int
cstring_count(cstring s) {
    int count = 0;
    while (*s++ != 0) count++;
    return count;
}

typedef struct {
    umm count;
    u8* data;
} string;

// Boolean
typedef u8  bool;
typedef s32 b32;
#define true 1
#define false 0

// Renaem static
#define internal static
#define global static
#define local_persist static

// Asserts
#if BUILD_DEBUG
void
__assert(cstring expression, cstring file, int line) {
    // TODO(alexander): improve assertion printing,
    printf("%s:%d: Assertion failed: %s\n", file, line, expression);
    *(int *)0 = 0; // NOTE(alexander): purposfully trap the program
}

#define assert(expression) (void)((expression) || (__assert(#expression, __FILE__, __LINE__), 0))
#else
#define assert(expression)
#endif


// VGA constants
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

// Helpers
#define fixed_array_count(array) (sizeof(array) / sizeof((array)[0]))

