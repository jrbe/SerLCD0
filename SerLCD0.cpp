//SerLCD0.cpp - Non-blocking LCD library implementation for OpenLCD displays
// Version F0.0.4
// Implements command queueing and state machine for timing-critical display operations

#include "SerLCD0.h"

// Static member initialization with explanatory comments
// Controls debug message output to Serial monitor - disabled by default for production use
bool SerLCD0::_SerLCD0_Debug = false;               

// Sets threshold for errors before triggering reset - defaults to 1 for quick recovery
uint8_t SerLCD0::_SerLCD0_ErrorThreshold = 1;       

// Constructor for SerLCD0 class - initializes display interface and state
SerLCD0::SerLCD0(TwoWire &wirePort, uint8_t i2c_addr) {
    _wirePort = &wirePort;         // Store reference to I2C interface object
    _i2cAddr = i2c_addr;           // Store display's I2C address (default 0x72)
    _state = State::READY;         // Initialize state machine to ready state
    _queueHead = 0;                // Initialize queue read position to start
    _queueTail = 0;                // Initialize queue write position to start
    _errorCount = 0;               // Initialize error counter to zero
    _needsFullRefresh = true;      // Set flag to perform full display refresh on first update
}

// Initialize display with specified Wire interface
void SerLCD0::begin(TwoWire &wirePort) {
    _wirePort = &wirePort;         // Update Wire interface pointer
    reinitialize();                // Perform full display reinitialization
}

// Reset display to known good state
void SerLCD0::reinitialize() {
    resetQueue();                  // Clear any pending commands from queue
    _state = State::PROCESSING;    // Set state to processing during init
    _lastActionTime = millis();    // Record initialization start time
    _errorCount = 0;               // Reset the error counter
    _needsFullRefresh = true;      // Mark display for full refresh
    
    // Queue basic initialization sequence
    clear();                       // Queue display clear command
    setBacklight(255, 255, 255);   // Queue white backlight command
}

// Main update function - handles state machine and command processing
bool SerLCD0::update() {
    unsigned long currentTime = millis();    // Get current time for timing checks
    
    // State machine implementation
    switch(_state) {
        case State::PROCESSING:
            // Check if minimum command processing time has elapsed
            if(currentTime - _lastActionTime >= _cmdTime) {
                _state = State::READY;       // Return to ready state if time elapsed
            }
            break;
            
        case State::ERROR:
            // Check if error recovery time has elapsed
            if(currentTime - _lastActionTime >= _errorResetTime) {
                _state = State::READY;       // Return to ready state
                _errorCount = 0;             // Reset error counter
                reinitialize();              // Attempt display reinitialization
            }
            return false;                    // Indicate not ready while in error state
            
        case State::READY:
            // Process next queued command if available
            if(_queueHead != _queueTail) {   // Check if queue contains commands
                return processNextCommand();  // Process next command in queue
            }
            break;
    }
    
    return (_state == State::READY);         // Return true if in ready state
}

// Calculate current number of commands in queue
uint8_t SerLCD0::getQueueCount() const {
    // Handle queue wrap-around when calculating size
    return (_queueTail >= _queueHead) ? 
        _queueTail - _queueHead :            // Simple case: tail after head
        QUEUE_SIZE - (_queueHead - _queueTail); // Wrapped case: tail before head
}

// Calculate queue fullness as percentage
float SerLCD0::getQueuePercentFull() const {
    // Convert current queue count to percentage of total capacity
    return (getQueueCount() * 100.0) / QUEUE_SIZE;
}

// Add new command to queue
bool SerLCD0::queueCommand(const LCDCommand& cmd) {
    // Calculate next queue position with wrap-around
    uint8_t nextTail = (_queueTail + 1) % QUEUE_SIZE;
    
    // Check for queue full condition
    if(nextTail == _queueHead) {
        handleError();             // Trigger error handling for full queue
        return false;              // Indicate command not queued
    }
    
    // Add command to queue and update tail position
    _cmdQueue[_queueTail] = cmd;   // Store command in queue
    _queueTail = nextTail;         // Update queue write position
    
    return true;                   // Indicate successful queue
}

// Process the next command in queue
bool SerLCD0::processNextCommand() {
    // Verify state and queue not empty
    if(_state != State::READY || _queueHead == _queueTail) {
        return false;              // Return false if not ready or queue empty
    }
    
    // Get command at current queue position
    LCDCommand& cmd = _cmdQueue[_queueHead];
    
    // Attempt to send command to display
    if(sendCommand(cmd)) {
        _queueHead = (_queueHead + 1) % QUEUE_SIZE;  // Update queue read position
        _state = State::PROCESSING;                   // Enter processing state
        _lastActionTime = millis();                   // Record command start time
        return true;                                  // Indicate successful processing
    }
    
    handleError();                 // Handle command transmission failure
    return false;                  // Indicate processing failure
}

// Send command to display via I2C
bool SerLCD0::sendCommand(const LCDCommand& cmd) {
    _wirePort->beginTransmission(_i2cAddr);  // Start I2C transmission
    
    // Process command based on type
    switch(cmd.type) {
        case LCDCommand::WRITE_CHAR:
            // Special handling for command characters
            if(cmd.data[0] == SPECIAL_COMMAND || cmd.data[0] == SETTING_COMMAND) {
                _wirePort->write(cmd.data[0]);  // Send character twice to escape
            }
            _wirePort->write(cmd.data[0]);      // Send character
            break;
            
        case LCDCommand::SPECIAL_CMD:
            _wirePort->write(SPECIAL_COMMAND);   // Send special command prefix
            _wirePort->write(cmd.data[0]);       // Send command byte
            break;
            
        case LCDCommand::SETTING_CMD:
            _wirePort->write(SETTING_COMMAND);   // Send settings command prefix
            _wirePort->write(cmd.data[0]);       // Send setting byte
            break;
            
        case LCDCommand::RGB_CMD:
            // Debug output for RGB values if enabled
            if (_SerLCD0_Debug) {
                Serial.println("Setting backlight RGB:");
                Serial.print(cmd.data[0]); Serial.print(",");
                Serial.print(cmd.data[1]); Serial.print(",");
                Serial.println(cmd.data[2]);
            }
            
            // Send RGB command sequence
            _wirePort->write(SETTING_COMMAND);   // Settings mode prefix
            _wirePort->write(RGB_COMMAND);       // RGB control command
            _wirePort->write(cmd.data[0]);       // Red value (0-255)
            _wirePort->write(cmd.data[1]);       // Green value (0-255)
            _wirePort->write(cmd.data[2]);       // Blue value (0-255)
            break;
            
        default:
            return false;                        // Invalid command type
    }
    
    // Complete I2C transmission and check result
    bool success = (_wirePort->endTransmission() == 0);
    if (_SerLCD0_Debug && !success) {
        Serial.println("I2C transmission failed");
    }
    return success;                             // Return transmission result
}

// Handle error conditions
void SerLCD0::handleError() {
    _errorCount++;                              // Increment error counter
    
    // Output debug information if enabled and at/above threshold
    if (_SerLCD0_Debug && _errorCount >= _SerLCD0_ErrorThreshold) {
        Serial.print("Error #");
        Serial.print(_errorCount);
        Serial.print(" in state: ");
        Serial.println(getStateString());
    }
    
    // Check if error threshold exceeded
    if(_errorCount > _SerLCD0_ErrorThreshold) {
        _state = State::ERROR;                  // Enter error state
        _needsFullRefresh = true;               // Mark for full refresh
        resetQueue();                           // Clear command queue
        
        if (_SerLCD0_Debug) {
            Serial.println("ERROR state - will reset");
        }
    }
    _lastActionTime = millis();                 // Record error time
}

// Reset queue to empty state
void SerLCD0::resetQueue() {
    _queueHead = 0;                            // Reset queue read position
    _queueTail = 0;                            // Reset queue write position
}

// Implement Print class write function
size_t SerLCD0::write(uint8_t b) {
    // Create command for character write
    LCDCommand cmd;
    cmd.type = LCDCommand::WRITE_CHAR;         // Set type to character write
    cmd.data[0] = b;                           // Store character
    cmd.dataLen = 1;                           // Set data length
    
    return queueCommand(cmd) ? 1 : 0;          // Return 1 if queued, 0 if failed
}

// Queue display clear command
void SerLCD0::clear() {
    LCDCommand cmd;
    cmd.type = LCDCommand::SPECIAL_CMD;        // Set type to special command
    cmd.data[0] = CLEAR_COMMAND;               // Set clear display command
    cmd.dataLen = 1;                           // Set data length
    queueCommand(cmd);                         // Queue the command
}

// Queue cursor home command
void SerLCD0::home() {
    LCDCommand cmd;
    cmd.type = LCDCommand::SPECIAL_CMD;        // Set type to special command
    cmd.data[0] = HOME_COMMAND;                // Set cursor home command
    cmd.dataLen = 1;                           // Set data length
    queueCommand(cmd);                         // Queue the command
}

// Set cursor position
void SerLCD0::setCursor(uint8_t col, uint8_t row) {
    row = min(row, (uint8_t)3);               // Limit row to 0-3 range
    
    // HD44780 memory offset for each row
    static const uint8_t row_offsets[] = { 0x00, 0x40, 0x14, 0x54 };
    
    // Create and queue cursor position command
    LCDCommand cmd;
    cmd.type = LCDCommand::SPECIAL_CMD;        // Set type to special command
    cmd.data[0] = 0x80 | (col + row_offsets[row]); // Calculate position command
    cmd.dataLen = 1;                           // Set data length
    queueCommand(cmd);                         // Queue the command
}

// Set RGB backlight color
void SerLCD0::setBacklight(uint8_t r, uint8_t g, uint8_t b) {
    // Debug output if enabled
    if (_SerLCD0_Debug) {
        Serial.print("Queueing backlight RGB(");
        Serial.print(r); Serial.print(",");
        Serial.print(g); Serial.print(",");
        Serial.print(b); Serial.println(")");
    }
    
    // Create and queue RGB command
    LCDCommand cmd;
    cmd.type = LCDCommand::RGB_CMD;            // Set type to RGB command
    cmd.data[0] = r;                           // Set red value
    cmd.data[1] = g;                           // Set green value
    cmd.data[2] = b;                           // Set blue value
    cmd.dataLen = 3;                           // Set data length
    
    // Queue command and debug output result
    bool success = queueCommand(cmd);
    if (_SerLCD0_Debug) {
        Serial.print("Backlight command ");
        Serial.println(success ? "queued" : "failed to queue");
    }
}

// Convert state enum to readable string
const char* SerLCD0::getStateString() const {
    switch(_state) {
        case State::READY: return "READY";              // Ready for commands
        case State::PROCESSING: return "PROCESSING";    // Processing command
        case State::AWAITING_RESPONSE: return "AWAITING_RESPONSE"; // Waiting for display
        case State::ERROR: return "ERROR";              // Error recovery
        default: return "UNKNOWN";                      // Invalid state
    }
}
