#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <stdbool.h>

// FAT12 filesystem
// https://wiki.osdev.org/FAT12
// NOTE(alexander): this is not meant to be a feature complete FAT system,
//                  right now the goal is to find the kernel binary.

// NOTE(alexander): define more convinient types
typedef unsigned int uint;
typedef int8_t       s8;
typedef uint8_t      u8;
typedef int16_t      s16;
typedef uint16_t     u16;
typedef int32_t      s32;
typedef uint32_t     u32;
typedef int64_t      s64;
typedef uint64_t     u64;
typedef uintptr_t    umm;
typedef intptr_t     smm;
typedef float        f32;
typedef double       f64;
typedef int32_t      b32;
typedef const char*  cstr;

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


typedef u8 Fat_Attribute;
enum {
    FatAttribute_Free = 0x00,
    FatAttribute_Read_Only = 0x01,
    FatAttribute_Hidden = 0x02,
    FatAttribute_System = 0x04,
    FatAttribute_Volume_Id = 0x08,
    FatAttribute_Directory = 0x10,
    FatAttribute_Archive = 0x20,
    FatAttribute_Long_Filename = (FatAttribute_Read_Only |
                                  FatAttribute_Hidden |
                                  FatAttribute_System |
                                  FatAttribute_Volume_Id),
};

#pragma pack(push, 1)
typedef struct {
    union {
        struct {
            u8 name[8];
            u8 extension[3];
        };
        u8 filename[11];
    };
    Fat_Attribute attributes;
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

typedef struct {
    void* content;
    umm size;
} Read_File_Result;

Read_File_Result
read_entire_file(const char* filepath) {
    Read_File_Result result;
    result.content = 0;
    result.size = 0;
    
    FILE* file = fopen(filepath, "rb");
    if (!file) {
        printf("Failed to read file %s\n", filepath);
        return result;
    }
    
    fseek(file, 0, SEEK_END);
    umm file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    result.content = malloc(file_size);
    result.size = file_size;
    fread(result.content, result.size, 1, file);
    fclose(file);
    
    return result;
}

bool
read_sectors(FILE* disk, u32 lba, u32 count, void* result) {
    bool ok = true;
    ok = ok && (fseek(disk, boot_sector.bytes_per_sector * lba, SEEK_SET) == 0);
    ok = ok && (fread(result, boot_sector.bytes_per_sector, count, disk) == count);
    return ok;
}

u16 // NOTE(alexander): read entry in FAT12, requires some bit tricks to get the 12-bit values
fat_read_entry(u16 cluster) {
    u16 fat_offset = cluster + (cluster / 2);
    u32 fat_value = (*(u16*)(fat + fat_offset));
    if (cluster % 2 == 0) {
        cluster = fat_value & 0x0fff;
    } else {
        cluster = fat_value >> 4;
    }
    return cluster;
}

void
fat_write_entry(u16 cluster, u16 value) {
    u16 fat_offset = cluster + (cluster / 2);
    u32 fat_value = (*(u16*)(fat + fat_offset));
    if (cluster % 2 == 0) {
        *(u16*)(fat + fat_offset) = (0x0fff & value) | (0xf000 & fat_value);
    } else {
        *(u16*)(fat + fat_offset) = (value << 4) | (0x000f & fat_value);
    }
}

inline u32
fat_cluster_to_lba(u16 cluster) {
    return root_directory_end_lba + (cluster - 2) * boot_sector.sectors_per_cluster;
}

Fat_Directory_Entry* 
fat_find_file(Fat_Directory_Entry* directory, char* filename) {
    for (u32 i = 0; i < boot_sector.dir_entry_count; i++) {
        if (memcmp(filename, directory[i].filename, 11) == 0) {
            return &directory[i];
        }
    }
    
    return NULL;
}

void
fat_write_directory_entry(Fat_Directory_Entry* entry, const char* filename, u8 attributes) {
    // Get the current local time
    time_t rawtime;
    struct tm* timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    
    // Write directory entry
    memcpy(entry->filename, filename, 11);
    entry->attributes = attributes;
    entry->creation_time_tenths = 0; // TODO(alexander): do we care about this?
    entry->creation_time = 0x1f & timeinfo->tm_hour;
    entry->creation_time |= (0x3f << 5) & timeinfo->tm_min;
    entry->creation_time |= (0x3f << 11) & timeinfo->tm_sec;
    entry->creation_date = 0x7F & timeinfo->tm_year;
    entry->creation_date |= (0xf << 5) & timeinfo->tm_mon;
    entry->creation_date |= (0x1f << 11) & timeinfo->tm_mday;
    entry->last_access_date = entry->creation_date;
    entry->first_cluster_high = 0;
    entry->last_modification_time = entry->creation_time;
    entry->last_modification_date = entry->creation_date;
    entry->first_cluster_low = 0;
    entry->size = 0;
}

bool
fat_write_file(const char* input_filepath,
               const char* filename,
               u8 attributes,
               void* disk_memory,
               Fat_Directory_Entry* directory) {
    
    for (u32 i = 0; i < boot_sector.dir_entry_count; i++) {
        Fat_Directory_Entry* entry = &directory[i];
        if (entry->attributes == FatAttribute_Free) {
            
            // Find empty slot in file allocation table
            u16 first_cluster = 2;
            while ((first_cluster = fat_read_entry(first_cluster)) != 0) {
                if (first_cluster >= 0x0ff8) {
                    return false; // NOTE(alexander): failed to write, not empty clusters available
                }
                first_cluster++;
            }
            
            // Write the contents
            Read_File_Result file = read_entire_file(input_filepath);
            if (file.size == 0) {
                return false; // NOTE(alexander): failed to read the contents of the file
            }
            
            u32 cluster_size = (boot_sector.sectors_per_cluster * boot_sector.bytes_per_sector);
            u32 num_clusters = file.size / cluster_size;
            if ((file.size % cluster_size) > 0) {
                num_clusters++; // make sure to include partially used sectors
            }
            
            // Copy the file contents to the each available cluster and update the file allocation table
            u16 cluster = first_cluster;
            u8* file_next_cluster = file.content;
            for (int index = 0; index < num_clusters; index++) {
                while (fat_read_entry(cluster) != 0) {
                    cluster++;
                }
                u32 lba = fat_cluster_to_lba(cluster);
                void* cluster_dst = (u8*) disk_memory + (lba * boot_sector.bytes_per_sector);
                memcpy(cluster_dst, file_next_cluster, boot_sector.bytes_per_sector);
                file_next_cluster += boot_sector.bytes_per_sector;
                fat_write_entry(cluster, (index == num_clusters - 1) ? 0x0FFF : cluster + 1);
                cluster++;
            }
            
            fat_write_directory_entry(entry, filename, attributes);
            entry->first_cluster_low = first_cluster + 2;
            entry->size = file.size;
            break;
        }
    }
    
    return true;
}

bool
fat_read_file(FILE* disk, Fat_Directory_Entry* file_entry, u8* buffer) {
    bool ok = true;
    u16 current_cluster = file_entry->first_cluster_low;
    
    do {
        u32 lba = fat_cluster_to_lba(current_cluster);
        ok = ok && read_sectors(disk, lba, boot_sector.sectors_per_cluster, buffer);
        buffer += boot_sector.sectors_per_cluster * boot_sector.bytes_per_sector;
        current_cluster = fat_read_entry(current_cluster);
    } while (ok && current_cluster < 0x0ff8);
    
    return ok;
}

int
main(int argc, char* argv[]) {
    char* disk_path;
    const char* msg_usage_string = "\nusage: %s (format | search) <floppy_iamge>\n";
    
    if (argc <= 1) {
        printf(msg_usage_string, argv[0]);
        return 1;
    }
    
    if (argc > 2) {
        disk_path = argv[2];
    } else {
        disk_path = "main_floppy.img";
    }
    
    
    char* command = argv[1];
    
    if (strcmp(command, "format") == 0) {
        
        // Load the bootsector and store it in disk memory
        Read_File_Result bootloader = read_entire_file("boot.bin");
        if (bootloader.size == 0) return 1;
        boot_sector = *((Fat_Boot_Sector*) bootloader.content);
        
        // Zero allocate all the sectors and store the bootloader
        umm disk_size = boot_sector.total_sectors * boot_sector.bytes_per_sector;
        void* disk_memory = calloc(1, disk_size);
        memcpy(disk_memory, bootloader.content, bootloader.size);
        free(bootloader.content);
        
        // Setup the file allocation table
        fat = ((u8*) disk_memory) + boot_sector.bytes_per_sector;
        
        // Setup the root directory
        u32 lba = boot_sector.reserved_sectors + boot_sector.sectors_per_fat * boot_sector.fat_count;
        root_directory = (Fat_Directory_Entry*) ((u8*) disk_memory + lba * boot_sector.bytes_per_sector);
        u32 size = sizeof(Fat_Directory_Entry) * boot_sector.dir_entry_count;
        u32 num_sectors = (size / boot_sector.bytes_per_sector);
        if (size % boot_sector.bytes_per_sector > 0) {
            num_sectors++; // make sure to include partially used sectors
        }
        
        root_directory_end_lba = lba + num_sectors;
        
        // Store the volume id
        fat_write_directory_entry(root_directory, "BINBOWSOS  ", FatAttribute_Volume_Id);
        fat_write_entry(0, 0xFF0); // reserved cluster
        fat_write_entry(1, 0xFFF); // last cluster
        
        // Save the kernel as a file in the FAT12 filesystem
        fat_write_file("kernel.bin", "KERNEL  BIN", FatAttribute_Archive, disk_memory, root_directory);
        
        // Copy file allocation table for data redundancy
        void* fat2 = ((u8*) disk_memory) + (boot_sector.bytes_per_sector * (boot_sector.sectors_per_fat + 1));
        memcpy(fat2, fat, boot_sector.bytes_per_sector * boot_sector.sectors_per_fat);
        
        // Write disk memory out to file
        FILE* disk_file = fopen(disk_path, "wb");
        fwrite(disk_memory, disk_size, 1, disk_file);
        fclose(disk_file);
        
    } else if (strcmp(command, "search") == 0) {
        char* filename;
        if (argc > 3) {
            filename = argv[3];
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
        
    } else {
        printf("\nUnknown command\n");
        printf(msg_usage_string, argv[0]);
        
    }
    
    return 0;
}