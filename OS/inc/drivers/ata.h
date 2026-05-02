#pragma once
#include <stdint.h>
#include <stddef.h>

#define ATA_SECTOR_SIZE 512

/**
 * ata_pio_read28 - read one 512-byte sector using ATA PIO LBA28
 * @lba: 28-bit logical block address to read
 * @out512: destination buffer, must hold ATA_SECTOR_SIZE bytes
 *
 * Return: 0 on success, nonzero error code on failure
 */
int ata_pio_read28(uint32_t lba, uint8_t* out512);

/**
 * ata_pio_write28 - write one 512-byte sector using ATA PIO LBA28
 * @lba: 28-bit logival block address to write
 * @in512: source buffer, must contain ATA_SECTOR_SIZE bytes
 *
 * Return: 0 on success, nonzero error code on failure
 */
int ata_pio_write28(uint32_t lba, const uint8_t* in512);
