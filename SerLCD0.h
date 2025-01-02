// SerLCD0.h - Non-blocking LCD library for OpenLCD displays
// Version F0.0.3
// Designed for Arduino R4 Wire/Wire1/Wire2 compatibility
// Uses non-blocking operations, command queue, and state machine for timing

#ifndef SERLCD0_H
#define SERLCD0_H

#include <Arduino.h>
#include <Wire.h>

// Command structure for LCD operations queue
struct LCDCommand {
    // Command types for different LCD operations
    enum Type {
        NONE,           // No command (empty/invalid)
        WRITE_CHAR,     // Write a single character to display
        SPECIAL_CMD,    // Special display commands (254 prefix)
        SETTING_CMD,    // Settings commands (0x7C prefix)
        RGB_CMD         // RGB backlight control command
    };
    
    Type type;                 // Type of command to execute
    uint8_t data[3];          // Command data (up to 3 bytes for RGB)
    uint8_t dataLen;          // Number of valid data bytes
};

// Main LCD control class, inherits from Print for text output
class SerLCD0 : public Print {
public:
    // Constructor - allows selection of Wire interface and I2C address
    SerLCD0(TwoWire &wirePort = Wire, uint8_t i2c_addr = 0x72);
    
    // Core initialization and control
    void begin(TwoWire &wirePort);          // Initialize with Wire interface
    void reinitialize();                    // Reset display to initial state
    bool update();                          // Process command queue (call in loop)
    
    // Basic display operations
    void clear();                           // Clear display content
    void home();                            // Return cursor to home position
    void setCursor(uint8_t col, uint8_t row);  // Set cursor position
    void setBacklight(uint8_t r, uint8_t g, uint8_t b);  // Set RGB backlight
    void noBacklight();                     // Turn off backlight
    void display();                         // Turn on display
    void noDisplay();                       // Turn off display
    
    // Timing configuration methods
    void setInitTime(unsigned long ms) { _initTime = ms; }         // Set initialization delay
    void setCmdTime(unsigned long ms) { _cmdTime = ms; }           // Set command processing time
    void setClearTime(unsigned long ms) { _clearTime = ms; }       // Set clear screen time
    void setErrorResetTime(unsigned long ms) { _errorResetTime = ms; }  // Set error recovery time
    
    // Debug control - unique names to avoid conflicts
    static void setSerLCD0_Debug(bool enable) { _SerLCD0_Debug = enable; }
    static void setSerLCD0_ErrorThreshold(uint8_t threshold) { _SerLCD0_ErrorThreshold = threshold; }
    
    // Queue and status monitoring
    uint8_t getQueueSize() const { return QUEUE_SIZE; }           // Get maximum queue capacity
    uint8_t getQueueCount() const;                                // Get current items in queue
    float getQueuePercentFull() const;                           // Get queue fill percentage
    uint8_t getErrorCount() const { return _errorCount; }         // Get cumulative error count
    
    // Display status checks
    bool isReady() const { return _state == State::READY; }       // Check if ready for command
    bool isBusy() const { return _state != State::READY; }        // Check if processing
    bool hasError() const { return _state == State::ERROR; }      // Check for error state
    bool needsRefresh() const { return _needsFullRefresh; }       // Check if refresh needed
    void clearRefreshFlag() { _needsFullRefresh = false; }        // Clear refresh flag
    const char* getStateString() const;                           // Get state as string
    static bool getDebug() { return _SerLCD0_Debug; }             // Get debug status
    
    // Print interface implementation for text output
    virtual size_t write(uint8_t);                                // Write single character
    using Print::write;                                           // Use Print's write methods

private:
    // Display state management
    enum class State {
        READY,              // Ready for next command
        PROCESSING,         // Processing current command
        AWAITING_RESPONSE, // Waiting for display response
        ERROR              // Error recovery state
    };
    
    // Command queue configuration
    static const uint8_t QUEUE_SIZE = 32;    // Maximum queued commands
    LCDCommand _cmdQueue[QUEUE_SIZE];        // Command queue storage
    uint8_t _queueHead;                      // Queue read position
    uint8_t _queueTail;                      // Queue write position
    
    // Timing parameters (milliseconds)
    unsigned long _initTime = 1000;          // Display initialization time
    unsigned long _cmdTime = 5;              // Command processing time
    unsigned long _clearTime = 50;           // Clear screen time
    unsigned long _errorResetTime = 100;     // Error recovery time
    
    // OpenLCD firmware command constants
    static const uint8_t SPECIAL_COMMAND = 254;  // Special command prefix
    static const uint8_t SETTING_COMMAND = 0x7C; // Settings command prefix
    static const uint8_t CLEAR_COMMAND = 0x01;   // Clear display command
    static const uint8_t HOME_COMMAND = 0x02;    // Home cursor command
    static const uint8_t RGB_COMMAND = 0x2B;     // RGB backlight command ('+'')
    static const uint8_t COLD_START_TIME = 350;  // Cold start delay
    
    // Debug and error control
    static bool _SerLCD0_Debug;              // Debug output enable
    static uint8_t _SerLCD0_ErrorThreshold;  // Error threshold for reset
    
    // Hardware interface
    TwoWire* _wirePort;                      // I2C interface pointer
    uint8_t _i2cAddr;                        // I2C device address
    
    // State tracking
    State _state;                            // Current state
    unsigned long _lastActionTime;           // Last action timestamp
    uint8_t _errorCount;                     // Error counter
    bool _needsFullRefresh;                  // Display refresh flag
    
    // Internal command processing
    bool queueCommand(const LCDCommand& cmd);   // Add command to queue
    bool processNextCommand();                  // Process next queued command
    bool sendCommand(const LCDCommand& cmd);    // Send command to display
    void handleError();                        // Handle error condition
    void resetQueue();                         // Clear command queue
};

#endif
