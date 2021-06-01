l#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

// FAT12 filesystem
// https://wiki.osdev.org/FAT12
// NOTE(alexander): this is not meant to be a feature complete FAT system,
//                  right now the goal is to find the kernel binary.

typedef uint8_t bool;
#define true 1
#define false 0

typedef uint8_t u8;
typedef int8_t s8;
typedef uint16_t u16;
typedef int16_t s16;
typedef uint32_t u32;
typedef int32_t s32;

#pragma pack(push, 1)
typedef struct {
    // BIOS Parameter Block
    u8  jump_instruction[3]; // this is for jumping over the parameter block
    u8  oem[8];
    u16 bytes_per_sector;
    u8  sectors_per_cluster;
    u16 reserved_sectors;
    u8  fat_count;
    u16 dir_entry_count;
    u16 total_sectors;
    u8  media_discriptor_type;
    u16 sectors_per_fat;
    u16 sectors_per_track;
    u16 heads;
    u32 hidden_sectors;
    u32 large_sector_count;
    
    // Extended Boot Record
    u8  drive_number;
    u8  reserved;
    u8  signature;
    u32 volume_id;
    u8  volume_label[11];
    u8  system_id[8];
    
    // The code is located here (we don't care about this)
    // Ends with 0xAA55.
} Fat_Boot_Sector;
#pragma pack(pop)

#define FatAttribute_Read_Only = 0x01,
#define FatAttribute_Hidden = 0x02,
#define FatAttribute_System = 0x04,
#define FatAttribute_Volume_Id = 0x08,
#define FatAttribute_Directory = 0x10,
#define FatAttribute_Archive = 0x20,
#define FatAttribute_Long_Filename = \
FatAttribute_Read_Only | \
FatAttribute_Hidden | \
FatAttribute_System | \
FatAttribute_Volume_Id;

#pragma pack(push, 1)
typedef struct {
    union {
        struct {
            u8 name[8];
            u8 extension[3];
        };
        u8 filename[11];
    };
    u8 attributes;
    u8 reserved;
    u8 creation_time_tenths; // tenths of a second
    u16 creation_time; // hour - 5 bits, minutes - 6 bits and seconds - 5 bits
    u16 creation_date; // year - 7 bits, month - 4 bits and seconds - 5 bits
    u16 last_access_date;
    u16 first_cluster_high; // always zero in FAT 12 and FAT 16
    u16 last_modification_time;
    u16 last_modification_date;
    u16 first_cluster_low; // use this number to find the first cluster for this entry
    u32 size; // in bytes
} Fat_Directory_Entry;
#pragma pack(pop)

// NOTE(alexander): these are global for new
Fat_Boot_Sector boot_sector;
Fat_Directory_Entry* root_directory;
u32 root_directory_end_lba;
u8* fat;

bool read_sectors(FILE* disk, u32 lba, u32 count, void* result) {
    bool ok = true;
    ok = ok && (fseek(disk, boot_sector.bytes_per_sector * lba, SEEK_SET) == 0);
    ok = ok && (fread(result, boot_sector.bytes_per_sector, count, disk) == count);
    return ok;
}

Fat_Directory_Entry* fat_find_file(Fat_Directory_Entry* directory, char* filename) {
    for (u32 i = 0; i < boot_sector.dir_entry_count; i++) {
        if (memcmp(filename, directory[i].filename, 11) == 0) {
            return &directory[i];
        }
    }
    
    return NULL;
}

bool fat_read_file(FILE* disk, Fat_Directory_Entry* file_entry, u8* buffer) {
    bool ok = true;
    u16 current_cluster = file_entry->first_cluster_low;
    
    do {
        u32 lba = root_directory_end_lba + (current_cluster - 2) * boot_sector.sectors_per_cluster;
        ok = ok && read_sectors(disk, lba, boot_sector.sectors_per_cluster, buffer);
        buffer += boot_sector.bytes_per_sector * boot_sector.sectors_per_cluster;
        
        u16 fat_offset = current_cluster + (current_cluster / 2);
        u32 fat_value = (*(u16*)(fat + fat_offset));
        if (current_cluster & 0x0001) {
            current_cluster = fat_value >> 4;
        } else {
            current_cluster = fat_value & 0x0fff;
        }
    } while (ok && current_cluster < 0x0ff8);
    
    return ok;
}

int main(int argc, char* argv[]) {
    char* disk_path;
    if (argc > 1) {
        disk_path = argv[1];
    } else {
        disk_path = "main_floppy.img";
    }
    
    char* filename;
    if (argc > 2) {
        filename = argv[2];
    } else {
        filename = "KERNEL  BIN";
    }
    
    FILE* disk = fopen(disk_path, "rb");
    if (!disk) {
        printf("\nFailed to read disk image `%s`\n", disk_path);
        return 1;
    }
    
    if (fread(&boot_sector, sizeof(Fat_Boot_Sector), 1, disk) == 0) {
        printf("\nFailed to read the boot sector, not valid FAT12 image\n");
        return 1;
    }
    
    // Read the file allocation table
    fat = (u8*) malloc(boot_sector.bytes_per_sector * boot_sector.sectors_per_fat);
    if (!read_sectors(disk, boot_sector.reserved_sectors, boot_sector.sectors_per_fat, fat)) {
        free(fat);
        printf("\nFailed to read the file allocation table, not valid FAT12 image\n");
        return 1;
    }
    
#if 0 // DEBUG: print out the fat table bytes
    for (int i = 0; i < 20; i++) {
        printf("%x ", (u32) fat[i]);
    }
#endif
    
    // Read the root directory
    u32 lba = boot_sector.reserved_sectors + boot_sector.sectors_per_fat * boot_sector.fat_count;
    u32 size = sizeof(Fat_Directory_Entry) * boot_sector.dir_entry_count;
    u32 num_sectors = (size / boot_sector.bytes_per_sector);
    if (size % boot_sector.bytes_per_sector > 0) {
        num_sectors++; // make sure to include partially used sectors
    }
    
    root_directory_end_lba = lba + num_sectors;
    root_directory = malloc(num_sectors * boot_sector.bytes_per_sector);
    if (!read_sectors(disk, lba, num_sectors, root_directory)) {
        printf("\nFailed to read the root directory, not valid FAT12 image\n");
        return 1;
    }
    
    Fat_Directory_Entry* file_entry = fat_find_file(root_directory, filename);
    if (!file_entry) {
        printf("\nCould not find the file `%s` in the root directory!\n", filename);
        return 1;
    }
    
    u8* file_data = malloc(file_entry->size + boot_sector.bytes_per_sector);
    if (!fat_read_file(disk, file_entry, file_data)) {
        printf("\nFailed to read the contents of file `%s`\n", filename);
    }
    
    for (int i = 0; i < 100; i++) {
        printf("%c", file_data[i]);
    }
    
    fclose(disk);
    
    return 0;
}