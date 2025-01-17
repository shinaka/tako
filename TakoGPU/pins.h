#pragma once

//=====================================
// PIO Assignments
//=====================================
// PIO0: CPU Communication & PSRAM
// SM0: PSRAM
// SM1: CPU Receive
// SM2: CPU Transmit
// SM3: Reserved

// PIO1: Display
// SM0: Display SPI
// SM1-3: Reserved

// PIO2: Sprite Engine
// SM0: Sprite Lookup
// SM1: Pattern Fetch
// SM2: Sprite Compose
// SM3: Reserved

// Pin Definitions for GPU Board

//=====================================
// PSRAM Interface (APS6404L)
//=====================================
#define PIN_PSRAM_SCK    18  // Clock
#define PIN_PSRAM_D0     19  // Data 0 (SI/SIO0 in QSPI mode)
#define PIN_PSRAM_D1     20  // Data 1 (SO/SIO1 in QSPI mode)
#define PIN_PSRAM_D2     21  // Data 2 (SIO2 in QSPI mode)
#define PIN_PSRAM_D3     22  // Data 3 (SIO3 in QSPI mode)
#define PIN_PSRAM_CS     23  // Chip Select (active low)

//=====================================
// Display Interface (ST7789)
//=====================================
// SPI Mode
#define PIN_DISP_MOSI    12  // SPI MOSI
#define PIN_DISP_SCK     13  // SPI Clock
#define PIN_DISP_CS      14  // Chip Select (active low)
#define PIN_DISP_DC      15  // Data/Command control
#define PIN_DISP_RST     16  // Reset (active low)
#define PIN_DISP_BL      17  // Backlight control

//=====================================
// CPU Communication Interface
//=====================================
// 8-bit Parallel Interface
#define PIN_D0           0   // Data Bus [0]
#define PIN_D1           1   // Data Bus [1]
#define PIN_D2           2   // Data Bus [2]
#define PIN_D3           3   // Data Bus [3]
#define PIN_D4           4   // Data Bus [4]
#define PIN_D5           5   // Data Bus [5]
#define PIN_D6           6   // Data Bus [6]
#define PIN_D7           7   // Data Bus [7]
#define PIN_CS           8   // Chip Select (active low)
#define PIN_RW           9   // Read/Write (1=read, 0=write)
#define PIN_WAIT         10  // Wait/Ready (0=busy, 1=ready)

//=====================================
// Status
//=====================================
#define PIN_LED          25  // Built-in LED for status

//=====================================
// Pin Groups
//=====================================
// data bus pins array
static const uint8_t PIN_DATA_BUS[] = 
{
    PIN_D0, PIN_D1, PIN_D2, PIN_D3,
    PIN_D4, PIN_D5, PIN_D6, PIN_D7
};
#define DATA_BUS_PIN_COUNT 8

// PSRAM pins array
static const uint8_t PIN_PSRAM_ALL[] = 
{
    PIN_PSRAM_SCK, PIN_PSRAM_D0, PIN_PSRAM_D1,
    PIN_PSRAM_D2, PIN_PSRAM_D3, PIN_PSRAM_CS
};
#define PSRAM_PIN_COUNT 6

// display pins array
static const uint8_t PIN_DISPLAY_ALL[] = 
{
    PIN_DISP_MOSI, PIN_DISP_SCK, PIN_DISP_CS,
    PIN_DISP_DC, PIN_DISP_RST, PIN_DISP_BL
};
#define DISPLAY_PIN_COUNT 6
