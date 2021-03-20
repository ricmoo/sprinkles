/**
 * 
 * Resources:
 * - https://github.com/espressif/esp-idf/blob/master/examples/peripherals/spi_master/main/spi_master_example_main.c
 */


#include "display.h"

#include <stdio.h>
#include <string.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/spi_master.h>
#include <driver/gpio.h>

#include "soc/gpio_struct.h"

//#include "demo.h"

// If using a display with the CS pin pulled low
#define NO_CS_PIN

#define DISPLAY_HEIGHT    240
#define DISPLAY_WIDTH     240

#define FRAGMENT_HEIGHT   24

// This is a HARD requirement; otherwise expect infinite loops and nothing to work
#if (240 % FRAGMENT_HEIGHT) != 0
#error "Fragment Height is not a factor of 240"
#endif

// The height of each fragment; this **MUST** be a factor of 240 (or there will be infinite loops)
const uint8_t DisplayFragmentHeight = FRAGMENT_HEIGHT;

const uint8_t DisplayFragmentCount = DISPLAY_HEIGHT / FRAGMENT_HEIGHT;


// Common ST7789 Commands and Parameter Components
typedef enum Command {
    CommandSLPOUT                 = 0x11,      // Sleep Out

    CommandINVOFF                 = 0x20,      // Display Inversion Off
    CommandINVON                  = 0x21,      // Display Inversion On
    CommandDISPON                 = 0x29,      // Display On

    CommandCASET                  = 0x2a,      // Column Address Set (4 parameters)
    CommandRASET                  = 0x2b,      // Row Address Set (4 parameters)
    CommandRAMWR                  = 0x2c,      // Memory Write (N parameters)

    CommandMADCTL                 = 0x36,      // Memory Data Access Control (1 parameter)
    CommandMADCTL_1_page          = (1 << 7),  // - Bottom to Top (vs. Top to Bottom)
    CommandMADCTL_1_column        = (1 << 6),  // - Right to Left (vs. Left to Right)
    CommandMADCTL_1_page_column   = (1 << 5),  // - Reverse Mode (vs. Normal Mode)
    CommandMADCTL_1_line_address  = (1 << 4),  // - LCD Refresh Bottom to Top (vs. LCD Refresh Top to Bottom)
    CommandMADCTL_1_rgb           = (1 << 3),  // - BGR (vs. RGB)
    CommandMADCTL_1_data_latch    = (1 << 2),  // - LCD Refresh Right to Left (vs. LCD Refresh Left to Right)

    CommandCOLMOD                 = 0x3a,      // Interface Pixel Format (1 parameter)
    CommandCOLMOD_1_format_65k    = 0x50,      // 65k colors
    CommandCOLMOD_1_format_262k   = 0x30,      // 262k colors
    CommandCOLMOD_1_width_12bit   = 0x03,      // 12-bit pixels
    CommandCOLMOD_1_width_16bit   = 0x05,      // 16-bit pixels
    CommandCOLMOD_1_width_18bit   = 0x06,      // 18-bit pixels

    CommandPORCTRL                = 0xb2,      // Porch Setting (5 parameters
    CommandGCTRL                  = 0xb7,      // Gate Control (1 parameter)
    CommandVCOMS                  = 0xbb,      // VCOM Setting (1 parameter)

    CommandLCMCTRL                = 0xc0,      // LCM Control (@TODO: Look more into this!) (1 parameter)
    CommandLCMCTRL_1_XMY          = (1 << 6),  // - XOR MY setting in command 36h
    CommandLCMCTRL_1_XBGR         = (1 << 5),  // - XOR RGB setting in command 36h
    CommandLCMCTRL_1_XREV         = (1 << 4),  // - XOR inverse setting in command 21h
    CommandLCMCTRL_1_XMX          = (1 << 3),  // - XOR MX setting in command 36h
    CommandLCMCTRL_1_XMH          = (1 << 2),  // - this bit can reverse source output order and only support for RGB interface without RAM mode
    CommandLCMCTRL_1_XMV          = (1 << 1),  // - XOR MV setting in command 36h
    CommandLCMCTRL_1_XGS          = (1 << 0),  // - XOR GS setting in command E4h

    CommandVDVVRHEN               = 0xc2,      // VDV and VRH Command Enable (2 parameters)
    CommandVRHS                   = 0xc3,      // VRH Set (1 parameter)
    CommandVDVS                   = 0xc4,      // VDV Set (1 parameter)

    CommandFRCTRL2                = 0xc6,      // Frame Rate Control in Normal Mode (1 parameter)
    CommandFRCTRL2_1_60hz         = 0x0f,      // - 60hz

    CommandPWCTRL1                = 0xd0,      // Power Control 1 (2 parameters)
    CommandPWCTRL1_1              = 0xa4,      // - First parameter
    CommandPWCTRL1_2_AVDD_6_4     = 0x00,      // - AVD 6.4v
    CommandPWCTRL1_2_AVDD_6_6     = 0x40,      // - AVD 6.6v
    CommandPWCTRL1_2_AVDD_6_8     = 0x80,      // - AVD 6.8v
    CommandPWCTRL1_2_AVCL_4_4     = 0x00,      // - AVCL -4.4v
    CommandPWCTRL1_2_AVCL_4_6     = 0x10,      // - AVCL -4.6v
    CommandPWCTRL1_2_AVCL_4_8     = 0x20,      // - AVCL -4.8v
    CommandPWCTRL1_2_AVCL_5_0     = 0x30,      // - AVCL -5.0v
    CommandPWCTRL1_2_VDS_2_19     = 0x00,      // - AVD 2.19v
    CommandPWCTRL1_2_VDS_2_3      = 0x01,      // - AVD 2.3v
    CommandPWCTRL1_2_VDS_2_4      = 0x02,      // - AVD 2.4v
    CommandPWCTRL1_2_VDS_2_51     = 0x03,      // - AVD 2.51v

    CommandPVGAMCTRL              = 0xe0,      // Positive Voltage Gamma Control (14 parameters)
    CommandNVGAMCTRL              = 0xe1,      // Negative Voltage Gamma Control (14 parameters)

    CommandDone                   = 0xfd,      // Pseudo-Command; internal use only; done commands
    CommandWait                   = 0xff,      // Pseudo-Command; internal use only; wait 10ms
} Command;


// ST7789 Initialization Sequence
// Place data into DRAM. Constant data gets placed into DROM by default, which is not accessible by DMA.
DRAM_ATTR static const uint8_t st7789_init_sequence[] = {
    CommandMADCTL,     1,   0,
    CommandCOLMOD,     1,   (CommandCOLMOD_1_format_65k | CommandCOLMOD_1_width_16bit),
    CommandPORCTRL,    5,   0x0c, 0x0c, 0x00, 0x33, 0x33,
    CommandGCTRL,      1,   0x45, // @TODO: Fill in ; Vgh=13.65V, Vgl=-10.43V
    CommandVCOMS,      1,   0x2b, // @TODO: Fill in ; VCOM=1.175V
    CommandLCMCTRL,    1,   (CommandLCMCTRL_1_XBGR | CommandLCMCTRL_1_XMX | CommandLCMCTRL_1_XMH),
    CommandVDVVRHEN,   2,   0x01, 0xff,
    CommandVRHS,       1,   0x11, // @TODO: Fill in ; Vap=4.4+
    CommandVDVS,       1,   0x20, // @TODO: Fill in ; VDV=0
    CommandFRCTRL2,    1,   (CommandFRCTRL2_1_60hz),
    CommandPWCTRL1,    2,   CommandPWCTRL1_1,
                            (CommandPWCTRL1_2_AVDD_6_8 | CommandPWCTRL1_2_AVCL_4_8 | CommandPWCTRL1_2_VDS_2_3),
    CommandPVGAMCTRL, 14,   0xd0, 0x00, 0x05, 0x0e, 0x15, 0x0d, 0x37, 0x43, 0x47, 0x09,
                            0x15, 0x12, 0x16, 0x19,
    CommandNVGAMCTRL, 14,   0xd0, 0x00, 0x05, 0x0d, 0x0c, 0x06, 0x2d, 0x44, 0x40, 0x0e,
                            0x1c, 0x18, 0x16, 0x19,
    CommandSLPOUT,     0,   CommandWait, // only need to wait for 5ms, but 10 givens us a bit of a buffer
    CommandDISPON,     0,
    CommandINVON,      0,
    CommandCASET,      4,   (0 >> 8), (0 & 0xff), ((DISPLAY_WIDTH - 1) >> 8), ((DISPLAY_WIDTH - 1) & 0xff),
    CommandDone
};

typedef enum MessageType {
    MessageTypeCommand      = 0,
    MessageTypeData         = 1
} MessageType;


typedef struct _DisplayContext {
    // The SPI device (low-speed during initialization, then upgraded to high-speed)
    spi_device_handle_t spi;

    // Two fragments, one for inflight data to the SPI hardware and one for a backbuffer
    uint8_t *fragments[2];

    // The currently inflight fragment (-1 for none; first round)
    int8_t inflightFragment;

    // The pins for D/C (Data/Contral), Reset and the Backlight
    uint8_t pinDC;
    uint8_t pinReset;
    uint8_t pinBacklight;

    // The co-routine state
    uint8_t currentY;
    uint32_t frame;

#ifdef DEBUG_SHOW_FPS
    // Gather statistics on frame rate
    uint16_t frameCount;
    uint32_t t0;
#endif

    // The prepared SPI transactions for sending fragments
    spi_transaction_t transactions[4];
} _DisplayContext;


static void* st7789_wrapTransaction(_DisplayContext *context, MessageType dc) {
    return (void*)((dc << 7) | context->pinDC);
}

// This is only used during initialization
static void st7789_send(_DisplayContext *context, MessageType dc, const uint8_t *data, int length) {
    if (length == 0) { return; }

    // Create the SPI transaction
    spi_transaction_t transaction;
    memset(&transaction, 0, sizeof(spi_transaction_t));
    transaction.tx_buffer = data;
    transaction.length = 8 * length;
    transaction.user = st7789_wrapTransaction(context, dc);

    // Send and wait for completion
    esp_err_t result = spi_device_polling_transmit(context->spi, &transaction);

    assert(result == ESP_OK);
}

// The ST7789 requires a GPIO pin to be set high for data and low for commands. Before
// each transaction this is called, which determines the transaxction type from the
// .user data, which is set using the st7789_wrapTransaction function.
static void IRAM_ATTR st7789_spi_pre_transfer_callback(spi_transaction_t *txn) {
    int user = (int)(txn->user);

    // We manage GPIO directly to keep it in IRAM, so we can call it from an ISR
    uint32_t level = (user >> 7);
    gpio_num_t gpio_num = (user & 0x7f);
    if (level) {
        GPIO.out_w1ts = (1 << gpio_num);
    } else {
        GPIO.out_w1tc = (1 << gpio_num);
    }
}

// Initialize all pins and send the initialization sequence to the display
static void st7789_init(_DisplayContext *context, DisplayRotation rotation) {
    // Initialize non-SPI GPIOs
    gpio_set_direction(context->pinDC, GPIO_MODE_OUTPUT);
    gpio_set_direction(context->pinReset, GPIO_MODE_OUTPUT);
    gpio_set_direction(context->pinBacklight, GPIO_MODE_OUTPUT);

    // Reset the display 
    gpio_set_level(context->pinReset, 0);

    // Send reset (T_RW; requires 10us)
    vTaskDelay(1 / portTICK_RATE_MS);

    gpio_set_level(context->pinReset, 1);

    // Wait for hardware reset (T_RT; 5ms for ) to complete (120ms needed if leaving sleep)
    vTaskDelay(6 / portTICK_RATE_MS); 

    uint32_t cmdIndex = 0;
    while (true) {
        uint8_t cmd = st7789_init_sequence[cmdIndex++];

        // Pseudo-commands...
        if (cmd == CommandWait) {
            vTaskDelay(6 / portTICK_RATE_MS);
            continue;
        } else if (cmd == CommandDone) {
            break;
        }

        st7789_send(context, MessageTypeCommand, &cmd, 1);

        // ST7789 command + parameters
        uint8_t paramCount = st7789_init_sequence[cmdIndex++];

        // Setting the Addressing requires injecting the screen rotation.
        // We only support 2 rotations for now, since they can be managed
        // exclusively with setup commands; other modes require adjusting
        // the offsets of memory writes, since the 320x240 memory only
        // uses 240x240, so the 80x240 pixels that are not shown differ
        // depending on the rotation.
        if (cmd == CommandMADCTL) {
            uint8_t operand = 0;
            switch (rotation) {
                case DisplayRotationPinsTop:
                    operand = 0;
                    break;
                case DisplayRotationPinsLeft:
                    operand = (CommandMADCTL_1_page_column | CommandMADCTL_1_column);
                    break;
            }
            st7789_send(context, MessageTypeData, &operand, 1);

        } else {
            st7789_send(context, MessageTypeData, &st7789_init_sequence[cmdIndex], paramCount);
        }

        cmdIndex += paramCount;
    }
  
    // Turn on backlight
    gpio_set_level(context->pinBacklight, 1);
}

// Asynchronously send a fragment (240 x FRAGMENT_HEIGHT) to the display using DMA.
// This will return immediately, and a call to the st7789_await_fragment function
// is required to the wait for these transactions to complete. Between the calls to
// st7789_asend_fragment and st7789_await_fragment the CPU is free to perform other
// tasks.
static void st7789_asend_fragment(_DisplayContext *context) {

    // The current y position 
    uint32_t y = context->currentY;
    context->transactions[1].tx_data[0] = y >> 8;                           // Start row (high)
    context->transactions[1].tx_data[1] = y & 0xff;                         // start row (low)
    context->transactions[1].tx_data[2] = (y + DisplayFragmentHeight - 1) >> 8;    // End row (high)
    context->transactions[1].tx_data[3] = (y + DisplayFragmentHeight - 1) & 0xff;  // End row (low)

    // Fragment data
    context->transactions[3].tx_buffer = context->fragments[context->inflightFragment];

    // Queue and send (asynchronously) all command and data transactions for this fragment
    for (int i = 0; i < 4; i++) {
        esp_err_t result = spi_device_queue_trans(context->spi, &(context->transactions[i]), portMAX_DELAY);
        assert(result == ESP_OK);
    }
}

// Wait for all the asynchronously sent transactions to complete.
// See: st7789_asend_fragment
static void st7789_await_fragment(_DisplayContext *context) {

    // Wait for all in-flight transactions are done
    spi_transaction_t *transaction;
    for (int i = 0; i < 4; i++) {
        esp_err_t result = spi_device_get_trans_result(context->spi, &transaction, portMAX_DELAY);
        assert(result == ESP_OK);
    }
}

static uint8_t *backBufferHi;
static uint8_t *backBufferLo;

// Terrible; but its a hackathon and memory pressure is being weird...
void display_setPixel(uint32_t x, uint32_t y, uint8_t r, uint8_t g, uint8_t b) {
    backBufferHi[y * DISPLAY_WIDTH + x] = (r & 0xf8) | (g >> 5);
    backBufferLo[y * DISPLAY_WIDTH + x] = ((g << 3) & 0xe0) | (b >> 3);
}

// Initialize the display driver for the ST7789 on a SPI bus. This requires using
// malloc because the memory acquired must be DMA compatible.
DisplayContext display_init(DisplaySpiBus spiBus, uint8_t pinDC, uint8_t pinReset, uint8_t pinBacklight, DisplayRotation rotation) {
    uint32_t t0 = millis();

    backBufferHi = malloc(240 * 240);
    backBufferLo = malloc(240 * 240);
    printf("B: %p %p\n", backBufferHi, backBufferLo);

    assert((DISPLAY_HEIGHT % DisplayFragmentHeight) == 0);

    _DisplayContext *context = malloc(sizeof(_DisplayContext));

    printf("B: %p %p %p\n", backBufferHi, backBufferLo, context);
    
    for (int i = 0; i < 2; i++) {
        uint8_t *data = heap_caps_malloc(DISPLAY_WIDTH * DisplayFragmentHeight * sizeof(uint16_t), MALLOC_CAP_DMA);
        assert(data != NULL && (((int)(data)) % 4) == 0);
        context->fragments[i] = data;
    }
    
    context->inflightFragment = -1;

    // GPIO pins
    context->pinDC = pinDC;
    context->pinReset = pinReset;
    context->pinBacklight = pinBacklight;

    // Current top Y coordinate to render
    context->currentY = 0;

    // Setup the Transaction parameters that are the same (ish) for all display updates
    for (uint i = 0; i < 4; i++) {
        memset(&(context->transactions[i]), 0, sizeof(spi_transaction_t));
        context->transactions[i].rx_buffer = NULL;
        context->transactions[i].flags = SPI_TRANS_USE_TXDATA;
    }

    // Page Address Set - Command
    context->transactions[0].length = 8;
    context->transactions[0].tx_data[0] = CommandRASET;
    context->transactions[0].user = st7789_wrapTransaction(context, MessageTypeCommand);

    // Page Address Set - Value
    context->transactions[1].length = 8 * 4;
    context->transactions[1].user = st7789_wrapTransaction(context, MessageTypeData);

    // Memory Write - Command
    context->transactions[2].length = 8;
    context->transactions[2].tx_data[0] = CommandRAMWR;
    context->transactions[2].user = st7789_wrapTransaction(context, MessageTypeCommand);

    // Memory Write - Value (remove the SPI_TRANS_USE_TXDATA flag)
    context->transactions[3].length = DISPLAY_WIDTH * 8 * sizeof(uint16_t) * DisplayFragmentHeight;
    context->transactions[3].user = st7789_wrapTransaction(context, MessageTypeData);
    context->transactions[3].flags = 0;

    // Get the selected device macro
    spi_host_device_t hostDevice = (spiBus == DisplaySpiBusHspi) ? HSPI_HOST: VSPI_HOST;

    // Bus Configuration
    {
        spi_bus_config_t busConfig = {
            .miso_io_num = -1,    // _DECODE_SPI_BUS_MISO(spiBus),
            .mosi_io_num = _DECODE_SPI_BUS_MOSI(spiBus),
            .sclk_io_num = _DECODE_SPI_BUS_SCLK(spiBus),
            .max_transfer_sz = DisplayFragmentHeight * DISPLAY_WIDTH * 2 + 8,
            .quadwp_io_num = -1,
            .quadhd_io_num = -1
        };

        esp_err_t result = spi_bus_initialize(hostDevice, &busConfig, 1);
        assert(result == ESP_OK);
    }

    // Device Interface Configuration
    spi_device_interface_config_t devConfig = {
        .clock_speed_hz = SPI_MASTER_FREQ_40M,           // Clock out at 40 MHz
#ifdef NO_CS_PIN
        // For ST7789 w/o a CS (i.e. pulled to ground)
        .mode = 3,                                       // SPI mode 0 (CPOL = 1, CPHA = 1)
        .spics_io_num = -1,                              // CS pin (not used)
#else
        // For ST7789 w/ a CS pin, which needs to be pulled low
        .mode = 0,                                       // SPI mode 0 (CPOL = 0, CPHA = 0)
        .spics_io_num = _DECODE_SPI_BUS_CS0(spiBus),    // CS pin (Chip Select)
#endif
        .queue_size = 7,                                 // Allow 7 in-flight transactions
        .pre_cb = st7789_spi_pre_transfer_callback,      // Handles the D/C gpio (Data/Command)
        .flags = SPI_DEVICE_NO_DUMMY
    };

    // Create a low-speed SPI device for initialization
    esp_err_t result = spi_bus_add_device(hostDevice, &devConfig, &(context->spi));
    assert (result == ESP_OK);

    // Initialize the display controller (with the low-speed SPI device)
    st7789_init(context, rotation);

    // Remove the low-speed SPI and replace it with a high-speed one
    result = spi_bus_remove_device(context->spi);
    assert (result == ESP_OK);        

    // Probably not necessary; ensure the old low-speed SPI device does not interfere with the new high-speed one
    memset(&(context->spi) , 0, sizeof(spi_device_handle_t));

    // Now configure a high-speed SPI interface for sending fragments
    devConfig.clock_speed_hz = SPI_MASTER_FREQ_80M;
    result = spi_bus_add_device(hostDevice, &devConfig, &(context->spi));
    assert (result == ESP_OK);        

    // Bookkeeping for statistics
    context->frame = 0;
    context->frameCount = 0;
    context->t0 = millis();

    printf("[DisplayDriver] Initialized: %d ms\n", context->t0 - t0);

    return context;
}

// Release the resources for this display driver
void display_free(DisplayContext context) {
    heap_caps_free(((_DisplayContext*)context)->fragments[0]);
    heap_caps_free(((_DisplayContext*)context)->fragments[1]);
    free(context);
}

// Render a single fragment. Call this multiple times (until 1 is returned ) to
// render the full screen. You may do additional CPU operations between calls as
// drawing each fragment is handled by DMA, so the processor is otherwise free
uint32_t display_render(DisplayContext _context) {
    _DisplayContext *context = _context;

    context->frame++;

    // Advance the fragment starting Y
    uint32_t y0 = context->currentY;

    // Select the free fragment (keep in mind inflightFragment can be -1, 0, or 1)
    uint8_t backbufferFragment = (context->inflightFragment == 0) ? 1: 0;

    uint8_t *backbuffer = context->fragments[backbufferFragment];

    // R5 G6 B5
    uint32_t srcOffset = y0 * 240;
    uint32_t dstOffset = 0;
    for (uint32_t i = 0; i < 240 * DisplayFragmentHeight; i++) {
      backbuffer[dstOffset++] = backBufferHi[srcOffset];
      backbuffer[dstOffset++] = backBufferLo[srcOffset];
      srcOffset++;
    }

    uint32_t spareTime = millis();

    // Wait for the previous (if any; first time does not) transactions to complete
    if (context->inflightFragment != -1) {
        st7789_await_fragment(context);
        spareTime = millis() - spareTime;
    } else {
        spareTime = 0;
    }

    // Swap inflight with backbuffer fragments
    context->inflightFragment = backbufferFragment;

    // Send the new fragment we just generated in the backbuffer (asynchronously)
    st7789_asend_fragment(context);

    context->currentY += DisplayFragmentHeight;

    // The last fragment...
    if (context->currentY == DISPLAY_HEIGHT) {
        context->currentY = 0;

        // Update statistics and optionally dump them to the terminal
        context->frameCount++;
        if (context->frameCount == 1000) {
          printf("[DisplayDriver] Framerate: %f fps\n", ((float)(context->frameCount)) / ((millis() - (context->t0)) / 1000.0f));
          context->frameCount = 0;
          context->t0 = millis();
        }

        return 1;
    }

    return 0;
}
