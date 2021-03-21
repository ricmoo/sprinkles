
// NOTE:
// The Secure Enclave requires an I2C Buffer of 256. This is not standard
// in many Arduino environments. You will need to modify the Wire.h files
// to increase it. Also the Arduino IDE likes to aggresively cache things,
// so you should try switching boards (to one you do not have hooked up)
// and recompile, which will flush the build cache. Then quit the app and
// relaunch it. I also recommend adding a new method (I added testThis())
// to the Wire.h and Wire.cpp and call it to make sure your new files
// is being picked up.
//
// Thanks to rz259 and lewispg228, without whom I would never have figured
// this out; Always check closed issues. :)
// See: https://github.com/sparkfun/SparkFun_ATECCX08a_Arduino_Library/issues/7

/**
 * TODO:
 * 
 * WiFi Configuration
 * If no Wifi connection is found, provide a QR code for AP setup
 * 
 * BLE Validation
 * Let any device connect over BLE and pass a challenge to sign proviing this
 * is an official Sprinkle.
 * 
 * - Use a signature from the server to authenticate (we avoid using SSL), optionally
 *   encrypting the request using ECDH.
 * 
 * - Cache Images to NVS; this requires a custom partition table, so ESP-IDF will be
 *   needed in a more intimate way. Not hard, just time.
 * 
 * - Move to the C libraries; C++ fragments memory weird and makes things feel disjoint. :)
 * 
 */

#include <SparkFun_ATECCX08a_Arduino_Library.h>
#include <SPI.h>
#include <WiFi.h>
#include <Wire.h>
#include <stdio.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "display.h"

// Display Pins
#define PIN_DISPLAY_DC         (21)
#define PIN_DISPLAY_RESET      (22)
#define PIN_DISPLAY_BACKLIGHT  (27)

// Secure Enclave Pins
#define PIN_SECURE_SDA          (33)
#define PIN_SECURE_SCL          (32)
#define ADDRESS_SECURE          (0x60)

#define MIN_DISPLAY_FPS    (60)
static DisplayContext displayContext = NULL;


// Wifi Configuration (@TODO: dynamic configuration)
char ssid[] = "RicMooGuest";
char password[] = "hellokitty";


static ATECCX08A atecc;

void setup() {
  
  // Set up the serial monitor
  Serial.begin(115200);

  displayContext = display_init(DisplaySpiBusVspi, PIN_DISPLAY_DC, PIN_DISPLAY_RESET, PIN_DISPLAY_BACKLIGHT, DisplayRotationPinsTop);

  // Use the second core for polling the server (core 0)
  //xTaskCreatePinnedToCore(&displayTask, "fetcher", 1024 * 2, NULL, 0, NULL, 0);

  // This cannot be done in the fetcherTask for some reason; once we move to esp-idf it should be better
  connectWifi();

  setupSecureEnclave();
  testSign();

  // Run the Fethcer on the default core (core 1)
  fetcherTask(NULL);
}



void loop() {
  Serial.println("Never reaches here.");
  delay(100000);
}

void setupSecureEnclave() {
  printf("Going to start...\n");
  Wire.begin(PIN_SECURE_SDA, PIN_SECURE_SCL);

  printf("Here we go: %d\n", Wire.testThis());

  if (atecc.begin() == true) {
    Serial.println("Successful wakeUp(). I2C connections are good.");
  } else {
    Serial.println("Device not found. Check wiring.");
    while (1);
  }

  printSecureEnclaveInfo();

  // check for configuration
  if (!(atecc.configLockStatus && atecc.dataOTPLockStatus && atecc.slot0LockStatus)) {
    Serial.print("Device not configured. Please use the configuration sketch.");
    while (1);
  }
}

void connectWifi() {
  int status = WL_IDLE_STATUS;
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);

    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, password);

    // wait 2.5 seconds before trying again
    delay(2500);
  }
}

void printSecureEnclaveInfo() {
  // Read all 128 bytes of Configuration Zone
  // These will be stored in an array within the instance named: atecc.configZone[128]
  atecc.readConfigZone(false); // Debug argument false (OFF)

  // Print useful information from configuration zone data
  printf("\n");

  // Serial Number
  dumpBuffer("Serial Number: \t", atecc.serialNumber, 9);

  // Revision Number
  dumpBuffer("Rev Number: \t", atecc.revisionNumber, 4);

  // Configuration Zone
  printf("Config Zone: \t%s\n", atecc.configLockStatus ? "Locked": "NOT Locked");

  // Data/OTP Zone:
  printf("Data/OTP Zone: \t%s\n", atecc.dataOTPLockStatus ? "Locked": "NOT Locked");

  // Data Slot 0
  printf("Data Slot 0: \t%s\n", atecc.slot0LockStatus ? "Locked": "NOT Locked");

  printf("\n");

  // if everything is locked up, then configuration is complete, so let's print the public key
  if (atecc.configLockStatus && atecc.dataOTPLockStatus && atecc.slot0LockStatus)  {
    if(atecc.generatePublicKey() == false) {
      printf("Failure to generate this device's Public Key\n");
      printf("\n");
    }
  }
}

// Read line up to length characters and if it begins with (case-insensitive) "Content-Length:"
// return the decimal value converted to a number
int32_t getContentLength(uint8_t *line, uint32_t length) {
  uint32_t offset = 0;

  // "content-length:"
  uint8_t contentLength[] = { 0x63, 0x6f, 0x6e, 0x74, 0x65, 0x6e, 0x74, 0x2d, 0x6c, 0x65, 0x6e, 0x67, 0x74, 0x68, 0x3a };

  while (offset < sizeof(contentLength)) {
    if (offset >= length) { return -1; }
    uint8_t b = line[offset];
    // .     "-"          ":"
    if (b != 0x2d && b != 0x3a) {
      // Lowercase
      if (b >= 0x41 && b <= 0x5a) { b |= 0x20; }

      // Make sure it is a letter
      if (b < 0x61 && b > 0x7a) { return -1; }

      // Make sure it matches the header name
      if (b != contentLength[offset]) { return -1; }
    }
    offset++;
  }

  int32_t result = 0;
  while (offset < length) {
    if (result > 1000000) {
      printf("too large!");
      return -1;
    }
    uint8_t b = line[offset];
    if (b == 0x0a || b == 0x0d) { break; }
    if (b < 0x30 || b > 0x39) { return -1; }
    result = (result * 10) + (b - 0x30);
    offset++;
  }

  return result;
}

// Convert a human-readable nibble to its value
uint8_t fromNibble(uint8_t v) {
  if (v >= 0x30 && v <= 0x39) { return v - 0x30; }
  return (v | 0x20) - 0x61 + 10;
}

// Dump a buffer to the console
void dumpBuffer(const char* const header, uint8_t *data, uint32_t length) {
  printf(header);
  for (uint8_t i = 0; i < length; i++) {
    printf("%02x", data[i]);
  }
  printf("\n");
}

void testSign() {
  // @TODO: in the futre, this will be a challenge/timestamp challenge form the server
//  atecc.wakeUp();
  uint8_t sprinkles[] = {
    79, 243,  36, 196, 222,  23, 195, 182,
    234, 173, 149, 202, 173,  50, 128, 111,
    116, 213, 246, 244, 176, 113,  64,  98, 
    56,  94, 187, 160, 170, 206, 220, 226
  };
  atecc.createSignature(sprinkles);
//  bool success = atecc.loadTempKey(sprinkles);
//  success = atecc.signTempKey();
  dumpBuffer("Signature: ", atecc.signature, 64);
}

void fetcherTask(void *context) {
  printf("Fetcher: coreId=%d\n", xPortGetCoreID());

  WiFiClient webClient;

  // Connect Wifi

  uint8_t sprinkles[] = {
    79, 243,  36, 196, 222,  23, 195, 182,
    234, 173, 149, 202, 173,  50, 128, 111,
    116, 213, 246, 244, 176, 113,  64,  98, 
    56,  94, 187, 160, 170, 206, 220, 226
  };

  const uint8_t hexChars[] = { 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46 };

  while (true) {
    Serial.println("Fetcher");

    if (webClient.connect("powerful-stream-18222.herokuapp.com", 80)) {
      Serial.println("connected to server");

      
      bool success = atecc.createSignature(sprinkles);
      printf("Signed (%d): ");
      dumpBuffer("", atecc.signature, 64);
      
      uint8_t prefix[] = "GET http://powerful-stream-18222.herokuapp.com/image?signature=0x";
      uint8_t suffix[] = " HTTP/1.1";
      uint8_t url[256];

      uint32_t offset = 0;
      while (true) {
        uint8_t v = prefix[offset];
        if (v == 0) { break; }
        url[offset++] = v;
      }

      for (uint32_t i = 0; i < 64; i++) {
          url[offset++] = hexChars[atecc.signature[i] >> 4];
          url[offset++] = hexChars[atecc.signature[i] & 0x0f];
      }

      uint32_t i = 0;
      while (true) {
        uint8_t v = suffix[i++];
        if (v == 0) { break; }
        url[offset++] = v;
      }

      url[offset++] = 0;

      printf("URL: \"%s\"\n", url);

      // Make a HTTP request:
      
//      webClient.println("GET /image?contract_address=0x31385d3520bced94f77aae104b406994d8f2168c&token_id=7197 HTTP/1.1");
//      webClient.println("GET /image?contract_address=0x06012c8cf97bead5deae237070f9587f8e7a266d&token_id=1337 HTTP/1.1");
//      webClient.println("GET /image?contract_address=0x06012c8cf97bead5deae237070f9587f8e7a266d&token_id=19709 HTTP/1.1");
//      webClient.println("GET /image?contract_address=0x91e03CA709C1950e621060e64ddEbdc3B7C6deDE&token_id=89 HTTP/1.1");
      webClient.println((const char* const)url);
      
      webClient.println("Host: powerful-stream-18222.herokuapp.com");
      webClient.println("Connection: close");
      webClient.println();

      uint32_t statusCode = 0;
      int32_t contentLength = -1;

      {
        uint32_t lineIndex = 0;
        uint32_t lastFour = 0;
        uint8_t line[128];
        uint32_t lineOffset = 0;
  
        // Read the headers
        while (true) {
          // Done
          if (!webClient.connected()) { break; }
  
          // Stall for more bytes to arrive
          if (!webClient.available()) {
            delay(10);
            continue;
          }
  
          // Line was too long; just keep consuming it
          if (lineOffset >= sizeof(line)) { lineOffset = 0; }
  
          uint8_t b = webClient.read();
          lastFour = (lastFour << 8) | b;
  
          if (b == 0x0a) { // The LF of CRLF
            line[lineOffset] = 0;
            if (lineIndex == 0) {
              printf("HTTP Status: %s\n", line);
            } else {
              printf("Header: %s\n", line);
              if (contentLength == -1) {
                contentLength = getContentLength(line, lineOffset);
              }
            }
            lineOffset = 0;
            lineIndex++;
          } else {
            line[lineOffset] = b;
            if (b != 0x20) { lineOffset++; }
          }
  
          if (lastFour == 0x0d0a0d0a) {
            printf("Done headers!\n");
            break;
          }
        }
      }

      printf("Content-Length: %d\n", contentLength);

      // Read body
      uint32_t skip = 2;
      uint32_t count = 0;
      uint8_t color[6];
      uint32_t index = 0;
      while (true) {
        // Done
        if (!webClient.connected()) { break; }

        // Stall for more bytes to arrive
        if (!webClient.available()) {
          delay(10);
          continue;
        }

        uint8_t b = webClient.read();
        if (skip) {
          skip--;
          continue;
        }

        color[count++] = fromNibble(b);
        if (count == 6) {
          display_setPixel(index % 240, index / 240, (color[0] << 4) | color[1], (color[2] << 4) | color[3], (color[4] << 4) | color[5]);
          index++;
          count = 0;
        }
      }
    }

    printf("Redraw!\n");
    redraw();

    printf("Fetcher Hgih-Water: %d\n", uxTaskGetStackHighWaterMark(NULL));

    delay(10000);
  }
}

// Draw the entire screen (every fragment)
void redraw() {
    uint32_t screenDone = 0;
    while (!screenDone) {
      screenDone = display_render(displayContext);
    }
}

void displayTask(void *context) {
  printf("Display: coreId=%d\n", xPortGetCoreID());

  // Initialize the Display
  displayContext = display_init(DisplaySpiBusVspi, PIN_DISPLAY_DC, PIN_DISPLAY_RESET, PIN_DISPLAY_BACKLIGHT, DisplayRotationPinsTop);
  while (true) {
    uint32_t screenDone = 0;
    while (!screenDone) {
      screenDone = display_render(displayContext);
    }
  }
}
