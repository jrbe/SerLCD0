// SerLCD0_Test.ino - Test program for non-blocking LCD library
// Tests cold start timing, color changes, and queue monitoring
// Version F0.0.4

#include <Wire.h>                         // Required for I2C communication
#include "SerLCD0.h"                      // Non-blocking LCD library

// Create LCD object using Wire1 (Arduino R4 Qwiic connector)
SerLCD0 lcd(Wire1);

// Program state enumeration for state machine control
enum TestState {
    INIT_WAIT,      // Wait for cold start timing
    INIT,           // Initialize display
    RECONNECT_WAIT, // Wait before reconnect attempt
    SET_BACKLIGHT,  // Set initial backlight
    WRITE_LINE1,    // Write display lines sequentially
    WRITE_LINE2,
    WRITE_LINE3,
    WRITE_LINE4,
    RUNNING         // Normal operation state
};

// Warning level enumeration for backlight color control
enum WarningLevel {
    NORMAL,     // White backlight - normal operation
    CAUTION,    // Orange backlight - minor warning
    WARNING     // Red backlight - serious warning
};

// State tracking variables
TestState currentState = INIT_WAIT;       // Current program state
WarningLevel warningState = NORMAL;       // Current warning level
unsigned long lastStatusUpdate = 0;        // Last status display update time
unsigned long lastStateChange = 0;         // Last state transition time
unsigned long lastWarningToggle = 0;       // Last warning level change time
unsigned long startTime = 0;               // Program start time

// Timing configurations (milliseconds)
const unsigned long STATUS_INTERVAL = 1000; // Status update interval
const unsigned long WARNING_TOGGLE = 5000;  // Time between warning states
const unsigned long STATE_DELAY = 100;      // Delay between state changes
const unsigned long COLD_START_TIME = 350;  // Cold start delay for display

// Set warning state and corresponding backlight color
void setWarningState(WarningLevel level) {
    warningState = level;                   // Update warning state
    
    switch(level) {
        case NORMAL:
            lcd.setBacklight(255, 255, 255); // White for normal
            break;
        case CAUTION:
            lcd.setBacklight(255, 140, 0);   // Orange for caution
            break;
        case WARNING:
            lcd.setBacklight(255, 0, 0);     // Red for warning
            break;
    }
}

void setup() {
    // Initialize serial communication for debugging
    Serial.begin(115200);
    Serial.println("\nStarting SerLCD0 Test...");
    
    // Record program start time for cold start timing
    startTime = millis();
    
    // Initialize I2C communication
    Wire1.begin();
    Wire1.setClock(100000);                // Standard 100kHz I2C
    
    // Configure LCD debug settings
    SerLCD0::setSerLCD0_Debug(true);       // Enable debug output
    SerLCD0::setSerLCD0_ErrorThreshold(1); // Show first error
    
    // Set LCD timing parameters
    lcd.setInitTime(1000);                 // 1 second init
    lcd.setCmdTime(5);                     // 5ms between commands
    lcd.setClearTime(50);                  // 50ms for clear
    
    // Start in cold start wait state
    currentState = INIT_WAIT;
    
    Serial.println("Setup complete - entering main loop");
}

void loop() {
    // Process LCD command queue
    lcd.update();
    
    // Get current time for state management
    unsigned long currentTime = millis();
    
    // State machine implementation
    switch(currentState) {
        case INIT_WAIT:
            // Wait for cold start time before initialization
            if(currentTime - startTime >= COLD_START_TIME) {
                Serial.println("Cold start complete - initializing display");
                lcd.begin(Wire1);
                currentState = INIT;
                lastStateChange = currentTime;
            }
            break;
            
        case INIT:
            // Initialize display when ready
            if(lcd.isReady()) {
                Serial.println("Display ready - clearing screen");
                lcd.clear();
                currentState = SET_BACKLIGHT;
                lastStateChange = currentTime;
            } else if(lcd.hasError()) {
                // Handle initialization errors
                currentState = RECONNECT_WAIT;
                lastStateChange = currentTime;
            }
            break;
            
        case RECONNECT_WAIT:
            // Wait before attempting reconnect
            if(currentTime - lastStateChange >= COLD_START_TIME) {
                Serial.println("Attempting reconnect...");
                lcd.reinitialize();
                currentState = INIT;
                lastStateChange = currentTime;
            }
            break;
            
        case SET_BACKLIGHT:
            // Set initial backlight state
            if(lcd.isReady() && (currentTime - lastStateChange > STATE_DELAY)) {
                Serial.println("Setting initial state");
                setWarningState(NORMAL);
                currentState = WRITE_LINE1;
                lastStateChange = currentTime;
            }
            break;
            
        case WRITE_LINE1:
            // Write first display line
            if(lcd.isReady() && (currentTime - lastStateChange > STATE_DELAY)) {
                Serial.println("Writing line 1");
                lcd.setCursor(0, 0);
                lcd.print("LCD Warning Test");
                currentState = WRITE_LINE2;
                lastStateChange = currentTime;
            }
            break;
            
        case WRITE_LINE2:
            // Write second display line
            if(lcd.isReady() && (currentTime - lastStateChange > STATE_DELAY)) {
                Serial.println("Writing line 2");
                lcd.setCursor(0, 1);
                lcd.print("Status: Normal");
                currentState = WRITE_LINE3;
                lastStateChange = currentTime;
            }
            break;
            
        case WRITE_LINE3:
            // Write third display line
            if(lcd.isReady() && (currentTime - lastStateChange > STATE_DELAY)) {
                Serial.println("Writing line 3");
                lcd.setCursor(0, 2);
                lcd.print("Queue Monitor Test");
                currentState = WRITE_LINE4;
                lastStateChange = currentTime;
            }
            break;
            
        case WRITE_LINE4:
            // Write fourth display line
            if(lcd.isReady() && (currentTime - lastStateChange > STATE_DELAY)) {
                Serial.println("Writing line 4");
                lcd.setCursor(0, 3);
                lcd.print("Time: ");
                currentState = RUNNING;
                lastStateChange = currentTime;
                lastStatusUpdate = currentTime;
                lastWarningToggle = currentTime;
            }
            break;
            
        case RUNNING:
              if(lcd.isReady() && (currentTime - lastStatusUpdate >= STATUS_INTERVAL)) {
                  // Time display
                  lcd.setCursor(5, 3);
                  lcd.print(currentTime / 1000);  
                  lcd.print("s ");  
          
                  // Move queue bar position
                  lcd.setCursor(12, 3);  // Adjust this number to move bar left/right
                  lcd.print("|");  
                  int barLength = (int)(lcd.getQueuePercentFull() * 0.07); // Adjust for available space
                  for(int i = 0; i < 7; i++) {
                      lcd.print(i < barLength ? '\xFF' : ' '); 
                  }
        
               if (SerLCD0::getDebug()) {
                   Serial.print("Queue: ");
                   Serial.print(lcd.getQueueCount());
                   Serial.print("/");
                   Serial.print(lcd.getQueueSize());
                   Serial.print(" (");
                   Serial.print(lcd.getQueuePercentFull(), 1);
                   Serial.print("%), Errors: ");
                   Serial.println(lcd.getErrorCount());
               }
               
               lastStatusUpdate = currentTime;
           }
            
            // Cycle warning states for demonstration
            if(currentTime - lastWarningToggle >= WARNING_TOGGLE) {
                switch(warningState) {
                    case NORMAL:
                        setWarningState(CAUTION);    // Normal -> Caution
                        lcd.setCursor(8, 1);
                        lcd.print("Caution ");
                        break;
                    case CAUTION:
                        setWarningState(WARNING);    // Caution -> Warning
                        lcd.setCursor(8, 1);
                        lcd.print("Warning!");
                        break;
                    case WARNING:
                        setWarningState(NORMAL);     // Warning -> Normal
                        lcd.setCursor(8, 1);
                        lcd.print("Normal  ");
                        break;
                }
                lastWarningToggle = currentTime;
            }
            
            // Check if display needs refresh
            if(lcd.needsRefresh()) {
                currentState = INIT;        // Restart initialization
                lcd.clearRefreshFlag();     // Clear refresh flag
            }
            break;
    }
}
