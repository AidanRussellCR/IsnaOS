ATA
===

The ATA driver provides basic PIO-based disk access using 28-bit LBA
addressing. It currently reads and writes one 512-byte sector at a time.

Constants
---------

``ATA_SECTOR_SIZE``
    Size of one ATA sector, currently 512 bytes.

Functions
---------

``ata_pio_read28(uint32_t lba, uint8_t* out512)``
    Read one sector from disk into ``out512``.

``ata_pio_write28(uint32_t lba, const uint8_t* in512)``
    Write one sector from ``in512`` to disk.

Notes
-----

The driver waits for the device to become ready, selects the LBA28 address,
issues the ATA read/write command, then transfers 256 16-bit words through the
ATA data port.

Limitations
-----------

Only one-sector PIO transfers are supported. DMA, interrupts, and LBA48 are not
currently implemented.
