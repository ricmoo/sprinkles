
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

  // Run the Fethcer on the default core (core 1)
  fetcherTask(NULL);


  // Show current cached cat 

  // Move to fetch
//  setupSecureEnclave();
}



void loop() {
  Serial.println("Moo");



  /*
    while (client.available()) {
    char c = client.read();
    Serial.write(c);
  }

  // if the server's disconnected, stop the client:

  if (!client.connected()) {

    Serial.println();

    Serial.println("disconnecting from server.");

    client.stop();

    // do nothing forevermore:

    while (true);

  }
  */
  
  // put your main code here, to run repeatedly:
  
  delay(100000);
}

void setupSecureEnclave() {
  Wire.begin(PIN_SECURE_SDA, PIN_SECURE_SCL);

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

//#define DATA_BUFFER_SIZE      (1024 * 200)


void printSecureEnclaveInfo() {
  // Read all 128 bytes of Configuration Zone
  // These will be stored in an array within the instance named: atecc.configZone[128]
  atecc.readConfigZone(false); // Debug argument false (OFF)

  // Print useful information from configuration zone data
  Serial.println();

  // Serial Number
  Serial.print("Serial Number: \t");
  for (int i = 0; i < 9; i++) {
    if ((atecc.serialNumber[i] >> 4) == 0) { Serial.print("0"); }
    Serial.print(atecc.serialNumber[i], HEX);
  }
  Serial.println();

  // Revision Number
  Serial.print("Rev Number: \t");
  for (int i = 0 ; i < 4 ; i++) {
    if ((atecc.revisionNumber[i] >> 4) == 0) { Serial.print("0"); }
    Serial.print(atecc.revisionNumber[i], HEX);
  }
  Serial.println();

  // Configuration Zone
  Serial.print("Config Zone: \t");
  if (atecc.configLockStatus) {
    Serial.println("Locked");
  } else { 
    Serial.println("NOT Locked");
  }

  // Data/OTP Zone:
  Serial.print("Data/OTP Zone: \t");
  if (atecc.dataOTPLockStatus) {
    Serial.println("Locked");
  } else {
    Serial.println("NOT Locked");
  }

  // Data Slot 0
  Serial.print("Data Slot 0: \t");
  if (atecc.slot0LockStatus) {
    Serial.println("Locked");
  } else {
    Serial.println("NOT Locked");
  }

  Serial.println();

  // if everything is locked up, then configuration is complete, so let's print the public key
  if (atecc.configLockStatus && atecc.dataOTPLockStatus && atecc.slot0LockStatus)  {
    if(atecc.generatePublicKey() == false) {
      Serial.println("Failure to generate this device's Public Key");
      Serial.println();
    }
  }
}

#define DATA_BUFFER_SIZE        (1024 * 60)

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

uint8_t fromNibble(uint8_t v) {
  if (v >= 0x30 && v <= 0x39) { return v - 0x30; }
  return (v | 0x20) - 0x61 + 10;
}

void fetcherTask(void *context) {
  printf("Fetcher: coreId=%d\n", xPortGetCoreID());

  WiFiClient webClient;

//  char server[] = "www.google.com";

  // Connect Wifi
  uint8_t *dataBuffer0 = (uint8_t*)malloc(DATA_BUFFER_SIZE + 1);
  uint8_t *dataBuffer1 = (uint8_t*)malloc(DATA_BUFFER_SIZE + 1);
  printf("DD: %p %p\n", dataBuffer0, dataBuffer1);
//  uint8_t *dataBuffer = (uint8_t*)heap_caps_malloc(DATA_BUFFER_SIZE + 1, MALLOC_CAP_SPIRAM);

  while (true) {
    Serial.println("Fetcher");

    if (webClient.connect("powerful-stream-18222.herokuapp.com", 80)) {
      Serial.println("connected to server");

      // Make a HTTP request:
      
//      webClient.println("GET /image?contract_address=0x31385d3520bced94f77aae104b406994d8f2168c&token_id=7197 HTTP/1.1");
      webClient.println("GET /image?contract_address=0x06012c8cf97bead5deae237070f9587f8e7a266d&token_id=821201 HTTP/1.1");
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
    //Serial.println("Display");
    uint32_t screenDone = 0;
    while (!screenDone) {
      screenDone = display_render(displayContext);
    }
    //Serial.println(uxTaskGetStackHighWaterMark(NULL));
    delay(1000);
  }
}
