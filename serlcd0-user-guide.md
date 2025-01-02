# SerLCD0 Library User Guide
Version V0.0.0

## Overview
Non-blocking LCD library for OpenLCD displays, designed for Arduino R4 Wire/Wire1/Wire2? compatibility.

## Hardware
- Compatible with SparkFun SerLCD displays
- Default I2C address: 0x72
- Supports 20x4 character display
- RGB backlight control

## Basic Usage
```cpp
#include <Wire.h>
#include "SerLCD0.h"

SerLCD0 lcd(Wire1);  // Create LCD object using Wire1 (Qwiic)

void setup() {
    Wire1.begin();
    Wire1.setClock(100000);  // 100kHz I2C
    lcd.begin(Wire1);
}

void loop() {
    lcd.update();  // Required in loop for non-blocking operation
}
```

## Display Control Functions
```cpp
// Text Operations
lcd.print("Text");             // Write text at cursor position
lcd.setCursor(col, row);       // Position cursor (col: 0-19, row: 0-3)
lcd.clear();                   // Clear display content
lcd.home();                    // Return cursor to home position (0,0)

// Display Power
lcd.display();                 // Turn on display
lcd.noDisplay();              // Turn off display

// Backlight Control
lcd.setBacklight(r, g, b);     // Set RGB backlight (0-255 each)
lcd.noBacklight();             // Turn off backlight

// Common Colors
lcd.setBacklight(255, 255, 255);  // White
lcd.setBacklight(255, 0, 0);      // Red
lcd.setBacklight(0, 255, 0);      // Green
lcd.setBacklight(0, 0, 255);      // Blue
lcd.setBacklight(255, 140, 0);    // Orange
```

## Timing Configuration
```cpp
lcd.setInitTime(1000);          // Init delay (ms)
lcd.setCmdTime(5);              // Command process time (ms)
lcd.setClearTime(50);           // Clear screen time (ms)
lcd.setErrorResetTime(100);     // Error recovery time (ms)
```

## Status Monitoring
```cpp
// Queue Status
lcd.getQueueSize();            // Maximum queue capacity
lcd.getQueueCount();           // Current items in queue
lcd.getQueuePercentFull();     // Queue fill percentage (float)

// Display Status
lcd.isReady();                 // Ready for commands
lcd.isBusy();                  // Currently processing
lcd.hasError();                // In error state
lcd.needsRefresh();            // Needs full refresh
lcd.clearRefreshFlag();        // Clear refresh flag
lcd.getErrorCount();           // Get error count

// Debug Control
SerLCD0::setSerLCD0_Debug(true);           // Enable debug output
SerLCD0::setSerLCD0_ErrorThreshold(1);     // Set error threshold
```

## Advanced Usage
```cpp
// Custom Constructor
SerLCD0 lcd(Wire1, 0x72);      // Specify I2C interface and address

// Manual Initialization
lcd.reinitialize();            // Reset to initial state

// State Information
lcd.getStateString();          // Get current state as string
```

## Error Handling
- Library automatically handles communication errors
- Attempts recovery after error threshold exceeded
- Debug output provides error information when enabled
- Hot-plug recovery supported

## Best Practices
1. Call update() regularly in loop()
2. Check isReady() before sending commands
3. Monitor queue fill to prevent overflow
4. Enable debug output during development
5. Allow 350ms cold start time before initialization
6. Use non-blocking state machine patterns

## Example Code
See SerLCD0_Test.ino for comprehensive usage example including:
- Cold start handling
- Queue monitoring
- Error recovery
- Status display
- Color changing
- Non-blocking operation

## Common Issues
1. Display unresponsive
   - Check I2C address
   - Verify Wire interface
   - Allow cold start time

2. Garbled display
   - Clear display
   - Reinitialize
   - Check timing settings

3. Queue overflow
   - Reduce command frequency
   - Monitor queue percentage
   - Increase queue size if needed