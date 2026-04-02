#include "bluetooth_utils.h"
#include "globals.h"
#include "config.h"
#include "audio_utils.h"
#include "system_utils.h"
#include "nvs_utils.h"
#include "display_utils.h"
#include <math.h>
#include <vector> // Ensure vector is included

// Helper function to convert BD address to String
String bdAddrToString(const esp_bd_addr_t address) {
    char buf[18];
    sprintf(buf, "%02X:%02X:%02X:%02X:%02X:%02X",
            address[0], address[1], address[2], address[3], address[4], address[5]);
    return String(buf);
}


int32_t get_data_frames(Frame *frames, int32_t frame_count) {
    static float m_time = 0.0f; 

    float m_amplitude;
    float current_freq;
    float m_deltaTime = 1.0f / 44100.0f;
    float m_phase = 0.0f;
    float pi_2 = PI * 2.0f;

#ifdef DEBUG_A2DP_AUDIO_PATH
    current_freq = 440.0f;    
    m_amplitude = 8000.0f;    
#else
    unsigned long now = millis();

    if (new_bt_beep_request && !current_bt_beep_is_active && now >= btBeepScheduledStartTime) {
        m_time = 0.0f; 
        current_bt_beep_is_active = true;
        new_bt_beep_request = false; 
        current_bt_beep_actual_end_time = now + btBeepDurationVolatile; 
    }

    if (current_bt_beep_is_active) {
        if (now < current_bt_beep_actual_end_time && btBeepFrequency > 0) {
            current_freq = (float)btBeepFrequency;
            m_amplitude = 10000.0f; 
        } else {
            current_bt_beep_is_active = false;
            btBeepFrequency = 0; 
        }
    }

    if (!current_bt_beep_is_active) {
        if (a2dp_source.is_connected()) {
            current_freq = 1.0f;    
            m_amplitude = 1.0f;     
        } else {
            current_freq = 1.0f;    
            m_amplitude = 0.0f;     
        }
    }
#endif

    for (int sample = 0; sample < frame_count; ++sample) {
        float angle = pi_2 * current_freq * m_time + m_phase;
        int16_t audio_sample = (int16_t)(m_amplitude * sin(angle));
        frames[sample].channel1 = audio_sample;
        frames[sample].channel2 = audio_sample;
        m_time += m_deltaTime;
        if (m_time >= 1.0f) { 
            m_time -= 1.0f;
        }
    }

    delay(1); 
    return frame_count;
}

void a2dp_connection_state_changed_callback(esp_a2d_connection_state_t state, void *object_instance) {
    if (state == ESP_A2D_CONNECTION_STATE_CONNECTED) {
        bluetoothJustConnected = true; 
    } else if (state == ESP_A2D_CONNECTION_STATE_DISCONNECTED) {
        bluetoothJustDisconnected = true; 
    }
}

// SSID Callback: Used for auto-connect AND discovery/scanning
bool a2dp_ssid_callback(const char *ssid, esp_bd_addr_t address, int rrsi) {
    if (currentState == BLUETOOTH_SCANNING && scanInProgress) { // Only add devices while scan is actively running
        // --- Discovery Mode ---
        bool found = false;
        String addrStr = bdAddrToString(address); 
        for (const auto& device : discoveredBtDevices) {
            if (device.address == addrStr) { 
                found = true;
                break;
            }
        }
        if (!found && discoveredBtDevices.size() < MAX_BT_DEVICES_DISPLAY) { 
            BTDevice newDevice;
            newDevice.name = String(ssid ? ssid : ""); // Handle potential NULL SSID
            newDevice.address = addrStr; 
            discoveredBtDevices.push_back(newDevice);
            redrawMenu = true; // Signal that the display needs updating
        }
        return false; // IMPORTANT: Always return false during scanning to prevent auto-connection
    } else if (currentState != BLUETOOTH_SCANNING) { // Check for auto-connect only if NOT scanning
        // --- Auto-Connect Mode ---
        if (!currentBluetoothDeviceName.isEmpty() && ssid && strcmp(ssid, currentBluetoothDeviceName.c_str()) == 0) {
            return true; // Found target, allow connection
        }
        return false; // Not the target device for auto-connect
    }
    return false; 
}


// Manages the A2DP discovery process and device selection
void handleBluetoothScanning() {
    resetActivityTimer(); 
    int rotation = StickCP2.Lcd.getRotation();
    int itemsPerScreen = MENU_ITEMS_PER_SCREEN_LANDSCAPE;

    if (scanInProgress) {
        // --- Scan is Active ---
        if (scanStartTime == 0) { // First time entering this state
            scanStartTime = millis(); 
            // Display "Scanning..." message immediately
            // The list will be drawn over this by displayBluetoothScanResults if redrawMenu is true
        }
        
        // Check if scan duration has elapsed
        if (millis() - scanStartTime >= (BT_SCAN_DURATION_S * 1000)) {
            a2dp_source.end(); // Stop A2DP discovery
            scanInProgress = false; // Mark scan as complete
            scanStartTime = 0;      // Reset timer
            redrawMenu = true;      // Flag to draw final results without "Scanning..." title
            
            // Provide feedback based on results
            if (discoveredBtDevices.empty()) {
                playUnsuccessBeeps(); 
            } else {
                playSuccessBeeps(); 
            }
        } 
        
        // Update display if needed (either for new device or scan completion)
        if (redrawMenu) {
            displayBluetoothScanResults(); // Show current list (title changes based on scanInProgress)
            redrawMenu = false;
        }

        // Handle early cancel via long press while scanning
        if (StickCP2.BtnA.pressedFor(LONG_PRESS_DURATION_MS)) {
             a2dp_source.end(); // Stop discovery
             scanInProgress = false;
             discoveredBtDevices.clear(); 
             setState(stateBeforeScan);   
             currentMenuSelection = 5;    
             int btMenuItemsPerScreen = MENU_ITEMS_PER_SCREEN_LANDSCAPE;
             menuScrollOffset = max(0, currentMenuSelection - btMenuItemsPerScreen + 1);
             StickCP2.Lcd.fillScreen(BLACK);
             return;
        }

        // If still scanning, return to allow loop to continue
        if (scanInProgress) return; 
    }

    // --- Scan Complete - Display Results & Handle Input ---
    // This section is reached only AFTER scanInProgress becomes false
    
    if (redrawMenu) { // Redraw if needed (e.g., after scrolling)
        displayBluetoothScanResults(); 
        redrawMenu = false;
    }

    // Input Handling for Scan Results navigation
    bool upPressed = (rotation == 3) ? M5.BtnPWR.wasClicked() : StickCP2.BtnB.wasClicked();
    bool downPressed = (rotation == 3) ? StickCP2.BtnB.wasClicked() : M5.BtnPWR.wasClicked();

    if (upPressed) {
        if (!discoveredBtDevices.empty()) {
            scanMenuSelection = (scanMenuSelection - 1 + discoveredBtDevices.size()) % discoveredBtDevices.size();
            redrawMenu = true;
        }
    }
    if (downPressed) {
        if (!discoveredBtDevices.empty()) {
            scanMenuSelection = (scanMenuSelection + 1) % discoveredBtDevices.size();
            redrawMenu = true;
        }
    }

    // Adjust scroll offset for scan results display
    if (scanMenuSelection < scanMenuScrollOffset) {
        scanMenuScrollOffset = scanMenuSelection; redrawMenu = true;
    } else if (scanMenuSelection >= scanMenuScrollOffset + itemsPerScreen) {
        scanMenuScrollOffset = scanMenuSelection - itemsPerScreen + 1; redrawMenu = true;
    }

    // Exit scan screen (Hold Front Button) - Return to BT Settings without connecting
    if (StickCP2.BtnA.pressedFor(LONG_PRESS_DURATION_MS)) {
        // Discovery is already stopped as scanInProgress is false here
        discoveredBtDevices.clear(); 
        setState(stateBeforeScan);   
        currentMenuSelection = 5;    
        int btMenuItemsPerScreen = (StickCP2.Lcd.getRotation() % 2 == 0) ? MENU_ITEMS_PER_SCREEN_PORTRAIT : MENU_ITEMS_PER_SCREEN_LANDSCAPE;
        menuScrollOffset = max(0, currentMenuSelection - btMenuItemsPerScreen + 1);
        StickCP2.Lcd.fillScreen(BLACK);
        return;
    }

    // Select a device from the scan results (Short Press Front Button) - Connect Immediately
    if (StickCP2.BtnA.wasClicked()) {
        if (!discoveredBtDevices.empty() && scanMenuSelection < discoveredBtDevices.size()) {
            BTDevice selectedDevice = discoveredBtDevices[scanMenuSelection];
            String deviceToConnect = "";

            // Prioritize name, fallback to address if name is empty
            if (!selectedDevice.name.isEmpty()) {
                deviceToConnect = selectedDevice.name;
            } else if (!selectedDevice.address.isEmpty()){
                deviceToConnect = selectedDevice.address; 
            } 
            
            if (!deviceToConnect.isEmpty()) {
                currentBluetoothDeviceName = deviceToConnect; // Update global variable
                saveSettings(); // Save the newly selected device name to NVS
                playSuccessBeeps(); // Indicate selection success

                // --- Attempt Connection Immediately ---
                a2dp_source.end(); // Ensure A2DP is stopped 
                delay(1000); // Wait 1 second as requested
                
                // Re-initialize essential callbacks 
                a2dp_source.set_data_callback_in_frames(get_data_frames);
                a2dp_source.set_on_connection_state_changed(a2dp_connection_state_changed_callback);
                a2dp_source.set_ssid_callback(a2dp_ssid_callback);
                a2dp_source.set_volume(currentBluetoothVolume); 
                
                a2dp_source.start((char*)currentBluetoothDeviceName.c_str()); // Start connection attempt
                // --- End Connection Attempt ---

                discoveredBtDevices.clear(); // Clear scan results list
                setState(stateBeforeScan);   // Return to Bluetooth Settings menu
                currentMenuSelection = 0;    // Highlight "Connect" item
                int btMenuItemsPerScreen = MENU_ITEMS_PER_SCREEN_LANDSCAPE;
                menuScrollOffset = max(0, currentMenuSelection - btMenuItemsPerScreen + 1);
                StickCP2.Lcd.fillScreen(BLACK); // Prepare for BT settings menu display

            } else {
                playUnsuccessBeeps();
            }
        } else {
            playUnsuccessBeeps();
        }
    }
}

