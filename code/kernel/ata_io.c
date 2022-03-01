
// Advanced Technology Attachment


// TODO(Alexander): you can have multiple devices
// next one is base port 0x170, third is 0x1E8 and forth is 0x168.
#define ATA_BASE_PORT 0x01F0
#define ATA_DATA_PORT         ATA_BASE_PORT
#define ATA_ERROR_PORT        ATA_BASE_PORT + 1
#define ATA_SECTOR_COUNT_PORT ATA_BASE_PORT + 2
#define ATA_LBA_LOW_PORT      ATA_BASE_PORT + 3
#define ATA_LBA_MID_PORT      ATA_BASE_PORT + 4
#define ATA_LBA_HIGH_PORT     ATA_BASE_PORT + 5
#define ATA_DEVICE_PORT       ATA_BASE_PORT + 6
#define ATA_COMMAND_PORT      ATA_BASE_PORT + 7
#define ATA_CONTROL_PORT      ATA_BASE_PORT + 0x206


bool
ata_initialize(bool master) {
    u8 device_id = master ? 0xA0 : 0xB0;
    out(ATA_DEVICE_PORT, device_id);
    out(ATA_CONTROL_PORT, 0);
    
    out(ATA_DEVICE_PORT, device_id);
    u8 status = in(ATA_COMMAND_PORT);
    if (status == 0xFF) {
        return false;
    }
    
    out(ATA_DEVICE_PORT, device_id);
    
    return true;
}

void printf(cstring fmt, ...);

// NOTE(Alexander): LBA is only 28 bits long
bool
ata_read_sectors(u32 lba, u8 num_sectors, void* out_buffer) {
    debug_break();
    
    u8 device_and_lba_highest = 0xE0 | ((u8) (lba & 0x0F000000) >> 24);
    outb(ATA_DEVICE_PORT, device_and_lba_highest);
    outb(ATA_ERROR_PORT, 0);
    outb(ATA_SECTOR_COUNT_PORT, num_sectors);
    outb(ATA_LBA_LOW_PORT, (lba & 0x000000FF));
    outb(ATA_LBA_MID_PORT, (lba & 0x0000FF00) >> 8);
    outb(ATA_LBA_HIGH_PORT, (lba & 0x00FF0000) >> 16);
    outb(ATA_COMMAND_PORT, 0x20);
    
    u8 status = inb(ATA_COMMAND_PORT);
    while (((status & 0x80) == 0x80) &&
           ((status & 0x01) != 0x01)) {
        status = inb(ATA_COMMAND_PORT);
    }
    
    if (status & 0x01) {
        printf("Read error\n");
        //return false;
    }
    
    u32 count = 512/2 * num_sectors; // NOTE(Alexander): hard coded words per sector, (512 bytes per sector)
    rep_insw(ATA_DATA_PORT, (u16*) out_buffer, count);
    
    return true;
}