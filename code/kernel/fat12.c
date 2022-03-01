
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



