Sprinkles
=========


Folders
-------

All `.c`, `.h` and `.ino` files are for Arduino, which doesn't
play well with being in a sub-folder.

- contract: The ERC-721 Contract for tracking Official Sprinkles
- mock-sprinkle: Tests for simulating Sprinkle connections

Todo
----

- Wifi Configuration; currently SSID and password are baked into the code
- Require server responses be signed (since we don't use HTTPS)
- BLE Verification
- Cache Images in NVS (requires custom partitioning)
- Move to C ESP-IDF apis instead of Arduino C++ APIs

Streatch Goals
--------------

- Ken Burns Effect
- Reduce power consumption?
