#include "input_handler.h"
#include "globals.h"
#include "config.h"
#include "display_utils.h"
#include "nvs_utils.h"
#include "system_utils.h"
#include "audio_utils.h"     // For reset_bt_beep_state
#include "bluetooth_utils.h" 
#include <LittleFS.h>


void handleModeSelectionInput() {
    const char* modeItems[] = {"Live Fire", "Dry Fire Par", "Noisy Range"};
    int modeCount = sizeof(modeItems) / sizeof(modeItems[0]);
    int rotation = StickCP2.Lcd.getRotation();
    int itemsPerScreen = (rotation % 2 == 0) ? MENU_ITEMS_PER_SCREEN_PORTRAIT : MENU_ITEMS_PER_SCREEN_LANDSCAPE;

    if (currentMenuSelection < menuScrollOffset) {
        menuScrollOffset = currentMenuSelection; redrawMenu = true;
    } else if (currentMenuSelection >= menuScrollOffset + itemsPerScreen) {
        menuScrollOffset = currentMenuSelection - itemsPerScreen + 1; redrawMenu = true;
    }

    if (redrawMenu) {
        displayMenu("Select Mode", modeItems, modeCount, currentMenuSelection, menuScrollOffset);
        redrawMenu = false;
    }

    bool upPressed = (rotation == 3) ? M5.BtnPWR.wasClicked() : StickCP2.BtnB.wasClicked();
    bool downPressed = (rotation == 3) ? StickCP2.BtnB.wasClicked() : M5.BtnPWR.wasClicked();

    if (upPressed) {
        resetActivityTimer();
        currentMenuSelection = (currentMenuSelection - 1 + modeCount) % modeCount; redrawMenu = true;
    }
    if (downPressed) {
        resetActivityTimer();
        currentMenuSelection = (currentMenuSelection + 1) % modeCount; redrawMenu = true;
    }

    if (StickCP2.BtnA.wasClicked()) {
        resetActivityTimer();
        currentMode = (OperatingMode)currentMenuSelection;
        switch (currentMode) {
            case MODE_LIVE_FIRE:   setState(LIVE_FIRE_READY); break;
            case MODE_DRY_FIRE:    setState(DRY_FIRE_READY); break;
            case MODE_NOISY_RANGE: setState(NOISY_RANGE_READY); break;
        }
        StickCP2.Lcd.fillScreen(BLACK);
        menuScrollOffset = 0;
    }
}

void handleSettingsInput() {
    resetActivityTimer();
    const char* title = "Settings";
    const char** items = nullptr;
    int itemCount = 0;
    int rotation = StickCP2.Lcd.getRotation();
    int itemsPerScreen = (rotation % 2 == 0) ? MENU_ITEMS_PER_SCREEN_PORTRAIT : MENU_ITEMS_PER_SCREEN_LANDSCAPE;

    static const char* mainItems[] = {"General", "Bluetooth", "Dry Fire", "Noisy Range", "Device Status", "List Files", "Power Off Now", "Save & Exit"};
    static const char* generalItems[] = {"Max Shots", "Beep Settings", "Shot Threshold", "Min 1st Shot", "Screen Rotation", "Boot Animation", "Auto Sleep", "Show Total Time", "Calibrate Thresh.", "Back"};
    static const char* beepItems[] = {"Beep Duration", "Beep Tone", "Back"};
    static const char* noisyItems[] = {"Recoil Threshold", "Calibrate Recoil", "Back"};

    const int maxDryFireItems = 1 + MAX_PAR_BEEPS + 1;
    static const char* dryFireItemsBuffer[maxDryFireItems];
    static String dryFireItemStrings[MAX_PAR_BEEPS];

    static const char* bluetoothItems[7]; 


    switch (settingsMenuLevel) {
        case 0:
            items = mainItems;
            itemCount = sizeof(mainItems) / sizeof(mainItems[0]);
            title = "Settings";
            break;
        case 1:
            items = generalItems;
            itemCount = sizeof(generalItems) / sizeof(generalItems[0]);
            title = "General Settings";
            break;
        case 2:
            title = "Dry Fire Settings";
            itemCount = 0;
            dryFireItemsBuffer[itemCount++] = "Par Beep Count";
            for (int i = 0; i < dryFireParBeepCount && i < MAX_PAR_BEEPS; ++i) {
                dryFireItemStrings[i] = "Par Time " + String(i + 1) + ": " + String(dryFireParTimesSec[i], 1) + "s";
                dryFireItemsBuffer[itemCount++] = dryFireItemStrings[i].c_str();
            }
            dryFireItemsBuffer[itemCount++] = "Back";
            items = dryFireItemsBuffer;
            break;
        case 3:
            items = noisyItems;
            itemCount = sizeof(noisyItems) / sizeof(noisyItems[0]);
            title = "Noisy Range Settings";
            break;
        case 4:
             items = beepItems;
             itemCount = sizeof(beepItems) / sizeof(beepItems[0]);
             title = "Beep Settings";
             break;
        case 5: 
            title = "Bluetooth Settings";
            bluetoothItems[0] = "Connect"; 
            bluetoothItems[1] = "Disconnect";
            bluetoothItems[2] = "Volume"; 
            bluetoothItems[3] = "BT Audio Offset"; 
            bluetoothItems[4] = "Auto Reconnect"; 
            bluetoothItems[5] = "Scan for Devices"; 
            bluetoothItems[6] = "Back";
            items = bluetoothItems; 
            itemCount = sizeof(bluetoothItems) / sizeof(bluetoothItems[0]); 
            break;
    }

    if (currentMenuSelection >= itemCount) {
        currentMenuSelection = max(0, itemCount - 1);
        redrawMenu = true;
    }
    if (currentMenuSelection < menuScrollOffset) {
        menuScrollOffset = currentMenuSelection; redrawMenu = true;
    } else if (currentMenuSelection >= menuScrollOffset + itemsPerScreen) {
        menuScrollOffset = currentMenuSelection - itemsPerScreen + 1; redrawMenu = true;
    }

    if (redrawMenu) {
        if (items) { displayMenu(title, items, itemCount, currentMenuSelection, menuScrollOffset); }
        redrawMenu = false;
    }

    bool upPressed = (rotation == 3) ? M5.BtnPWR.wasClicked() : StickCP2.BtnB.wasClicked();
    bool downPressed = (rotation == 3) ? StickCP2.BtnB.wasClicked() : M5.BtnPWR.wasClicked();

    if (upPressed) {
        currentMenuSelection = (currentMenuSelection - 1 + itemCount) % itemCount; redrawMenu = true;
    }
    if (downPressed) {
        currentMenuSelection = (currentMenuSelection + 1) % itemCount; redrawMenu = true;
    }

    if (StickCP2.BtnA.pressedFor(LONG_PRESS_DURATION_MS)) {
         if (settingsMenuLevel == 0) {
             setState(MODE_SELECTION); currentMenuSelection = (int)currentMode; menuScrollOffset = 0;
             StickCP2.Lcd.fillScreen(BLACK);
         } else if (settingsMenuLevel == 1 || settingsMenuLevel == 2 || settingsMenuLevel == 3 || settingsMenuLevel == 5) {
             int oldMenuLevel = settingsMenuLevel;
             settingsMenuLevel = 0;
             currentMenuSelection = (oldMenuLevel == 5 ? 1 : (oldMenuLevel == 2 ? 2 : (oldMenuLevel == 3 ? 3 : 0) ) ); 
             menuScrollOffset = 0;
             redrawMenu = true;
         } else if (settingsMenuLevel == 4) {
             settingsMenuLevel = 1; currentMenuSelection = 1; 
             menuScrollOffset = 0; redrawMenu = true;
         }
         return;
    }

    if (StickCP2.BtnA.wasClicked()) {
        bool needsActionRedraw = true;

        if (settingsMenuLevel == 0) {
            if (strcmp(items[currentMenuSelection], "General") == 0) settingsMenuLevel = 1;
            else if (strcmp(items[currentMenuSelection], "Bluetooth") == 0) settingsMenuLevel = 5;
            else if (strcmp(items[currentMenuSelection], "Dry Fire") == 0) settingsMenuLevel = 2;
            else if (strcmp(items[currentMenuSelection], "Noisy Range") == 0) settingsMenuLevel = 3;
            else if (strcmp(items[currentMenuSelection], "Device Status") == 0) {
                setState(DEVICE_STATUS); needsActionRedraw = false; StickCP2.Lcd.fillScreen(BLACK);
            }
            else if (strcmp(items[currentMenuSelection], "List Files") == 0) {
                setState(LIST_FILES); fileListScrollOffset = 0; needsActionRedraw = false; StickCP2.Lcd.fillScreen(BLACK);
            }
            else if (strcmp(items[currentMenuSelection], "Power Off Now") == 0) {
                StickCP2.Lcd.fillScreen(BLACK);
                StickCP2.Lcd.setTextDatum(MC_DATUM);
                StickCP2.Lcd.drawString("Powering Off...", StickCP2.Lcd.width()/2, StickCP2.Lcd.height()/2);
                delay(1500);
                StickCP2.Power.powerOff();
            }
            else if (strcmp(items[currentMenuSelection], "Save & Exit") == 0) {
                saveSettings(); playSuccessBeeps(); setState(MODE_SELECTION);
                currentMenuSelection = (int)currentMode; menuScrollOffset = 0; needsActionRedraw = false;
                StickCP2.Lcd.fillScreen(BLACK);
            }
            currentMenuSelection = 0; menuScrollOffset = 0;
        }
        else if (settingsMenuLevel == 1) { 
            editingSettingName = items[currentMenuSelection];
            stateBeforeEdit = SETTINGS_MENU_GENERAL;
            if (strcmp(editingSettingName, "Max Shots") == 0) {
                settingBeingEdited = EDIT_MAX_SHOTS; editingIntValue = currentMaxShots; setState(EDIT_SETTING); needsActionRedraw = false; StickCP2.Lcd.fillScreen(BLACK);
            } else if (strcmp(editingSettingName, "Beep Settings") == 0) {
                settingsMenuLevel = 4; currentMenuSelection = 0; menuScrollOffset = 0; 
            } else if (strcmp(editingSettingName, "Shot Threshold") == 0) {
                settingBeingEdited = EDIT_SHOT_THRESHOLD; editingIntValue = shotThresholdRms; setState(EDIT_SETTING); needsActionRedraw = false; StickCP2.Lcd.fillScreen(BLACK);
            } else if (strcmp(editingSettingName, "Min 1st Shot") == 0) {
                settingBeingEdited = EDIT_MIN_FIRST_SHOT; editingIntValue = minFirstShotTimeMs; setState(EDIT_SETTING); needsActionRedraw = false; StickCP2.Lcd.fillScreen(BLACK);
            } else if (strcmp(editingSettingName, "Screen Rotation") == 0) {
                settingBeingEdited = EDIT_ROTATION; editingIntValue = screenRotationSetting; setState(EDIT_SETTING); needsActionRedraw = false; StickCP2.Lcd.fillScreen(BLACK);
            } else if (strcmp(editingSettingName, "Boot Animation") == 0) {
                settingBeingEdited = EDIT_BOOT_ANIM; editingBoolValue = playBootAnimation; setState(EDIT_SETTING); needsActionRedraw = false; StickCP2.Lcd.fillScreen(BLACK);
            } else if (strcmp(editingSettingName, "Auto Sleep") == 0) {
                settingBeingEdited = EDIT_AUTO_SLEEP; editingBoolValue = enableAutoSleep; setState(EDIT_SETTING); needsActionRedraw = false; StickCP2.Lcd.fillScreen(BLACK);
            } else if (strcmp(editingSettingName, "Show Total Time") == 0) {
                settingBeingEdited = EDIT_SHOW_TOTAL_TIME; editingBoolValue = showTotalTime; setState(EDIT_SETTING); needsActionRedraw = false; StickCP2.Lcd.fillScreen(BLACK);
            } else if (strcmp(editingSettingName, "Calibrate Thresh.") == 0) {
                setState(CALIBRATE_THRESHOLD); peakRMSOverall = 0; micPeakRMS.resetPeak(); needsActionRedraw = false; StickCP2.Lcd.fillScreen(BLACK);
            } else if (strcmp(editingSettingName, "Back") == 0) {
                settingsMenuLevel = 0; currentMenuSelection = 0; menuScrollOffset = 0; 
            }
        }
        else if (settingsMenuLevel == 2) { 
            stateBeforeEdit = SETTINGS_MENU_DRYFIRE;
            if (strcmp(items[currentMenuSelection], "Par Beep Count") == 0) {
                editingSettingName = items[currentMenuSelection];
                settingBeingEdited = EDIT_PAR_BEEP_COUNT; editingIntValue = dryFireParBeepCount; setState(EDIT_SETTING); needsActionRedraw = false; StickCP2.Lcd.fillScreen(BLACK);
            } else if (strncmp(items[currentMenuSelection], "Par Time", 8) == 0) {
                int parTimeIndex = currentMenuSelection - 1; 
                if (parTimeIndex >= 0 && parTimeIndex < dryFireParBeepCount) {
                    String tempTitle = "Par Time " + String(parTimeIndex + 1);
                    static char parTimeEditTitle[20]; 
                    strncpy(parTimeEditTitle, tempTitle.c_str(), sizeof(parTimeEditTitle) - 1);
                    parTimeEditTitle[sizeof(parTimeEditTitle) - 1] = '\0';
                    editingSettingName = parTimeEditTitle;
                    settingBeingEdited = EDIT_PAR_TIME_ARRAY;
                    editingIntValue = parTimeIndex; 
                    editingFloatValue = dryFireParTimesSec[parTimeIndex];
                    setState(EDIT_SETTING);
                    needsActionRedraw = false;
                    StickCP2.Lcd.fillScreen(BLACK);
                }
            } else if (strcmp(items[currentMenuSelection], "Back") == 0) {
                settingsMenuLevel = 0; currentMenuSelection = 2; menuScrollOffset = 0; 
            }
        }
        else if (settingsMenuLevel == 3) { 
            editingSettingName = items[currentMenuSelection];
            stateBeforeEdit = SETTINGS_MENU_NOISY;
            if (strcmp(editingSettingName, "Recoil Threshold") == 0) {
                settingBeingEdited = EDIT_RECOIL_THRESHOLD; editingFloatValue = recoilThreshold; setState(EDIT_SETTING); needsActionRedraw = false; StickCP2.Lcd.fillScreen(BLACK);
            } else if (strcmp(editingSettingName, "Calibrate Recoil") == 0) {
                setState(CALIBRATE_RECOIL); needsActionRedraw = false; StickCP2.Lcd.fillScreen(BLACK); peakRecoilValue = 0;
            } else if (strcmp(editingSettingName, "Back") == 0) {
                settingsMenuLevel = 0; currentMenuSelection = 3; menuScrollOffset = 0; 
            }
        }
        else if (settingsMenuLevel == 4) { 
            editingSettingName = items[currentMenuSelection];
            stateBeforeEdit = SETTINGS_MENU_BEEP;
             if (strcmp(editingSettingName, "Beep Duration") == 0) {
                settingBeingEdited = EDIT_BEEP_DURATION; editingULongValue = currentBeepDuration; setState(EDIT_SETTING); needsActionRedraw = false; StickCP2.Lcd.fillScreen(BLACK);
            } else if (strcmp(editingSettingName, "Beep Tone") == 0) {
                settingBeingEdited = EDIT_BEEP_TONE; editingIntValue = currentBeepToneHz; setState(EDIT_SETTING); needsActionRedraw = false; StickCP2.Lcd.fillScreen(BLACK);
            } else if (strcmp(editingSettingName, "Back") == 0) {
                settingsMenuLevel = 1; currentMenuSelection = 1; 
                menuScrollOffset = 0;
            }
        }
        else if (settingsMenuLevel == 5) { // Bluetooth Settings
            stateBeforeEdit = SETTINGS_MENU_BLUETOOTH;
            editingSettingName = items[currentMenuSelection]; 
            if (strcmp(items[currentMenuSelection], "Connect") == 0) {
                if (!a2dp_source.is_connected()) {
                    if (!currentBluetoothDeviceName.isEmpty()) {
                        a2dp_source.end(); 
                        delay(100); 
                        a2dp_source.set_data_callback_in_frames(get_data_frames);
                        a2dp_source.set_on_connection_state_changed(a2dp_connection_state_changed_callback);
                        a2dp_source.set_ssid_callback(a2dp_ssid_callback);
                        a2dp_source.set_volume(currentBluetoothVolume); 

                        a2dp_source.start((char*)currentBluetoothDeviceName.c_str()); 
                    } else {
                        // No device name stored, maybe prompt to scan?
                        playUnsuccessBeeps(); 
                        redrawMenu = true; 
                    }
                } else {
                    playSuccessBeeps(); // Already connected
                }
                needsActionRedraw = true; 
            } else if (strcmp(items[currentMenuSelection], "Disconnect") == 0) {
                if (a2dp_source.is_connected()){
                    a2dp_source.disconnect();
                }
                reset_bt_beep_state(); 
                needsActionRedraw = true; 
            } else if (strcmp(items[currentMenuSelection], "Volume") == 0) {
                settingBeingEdited = EDIT_BT_VOLUME;
                editingIntValue = currentBluetoothVolume;
                setState(EDIT_SETTING);
                needsActionRedraw = false; StickCP2.Lcd.fillScreen(BLACK);
            } else if (strcmp(items[currentMenuSelection], "BT Audio Offset") == 0) { 
                settingBeingEdited = EDIT_BT_AUDIO_OFFSET;
                editingIntValue = currentBluetoothAudioOffsetMs;
                setState(EDIT_SETTING);
                needsActionRedraw = false; StickCP2.Lcd.fillScreen(BLACK);
            } else if (strcmp(items[currentMenuSelection], "Auto Reconnect") == 0) {
                settingBeingEdited = EDIT_BT_AUTO_RECONNECT;
                editingBoolValue = currentBluetoothAutoReconnect;
                setState(EDIT_SETTING);
                needsActionRedraw = false; StickCP2.Lcd.fillScreen(BLACK);
            } else if (strcmp(items[currentMenuSelection], "Scan for Devices") == 0) {
                // --- Setup for A2DP Discovery Scan ---
                if (a2dp_source.is_connected()){
                    a2dp_source.disconnect(); 
                }
                a2dp_source.end(); // Fully stop A2DP 
                delay(200);        
                
                // currentBluetoothDeviceName = ""; // Keep the last connected name unless a new one is selected
                reset_bt_beep_state(); 

                // Re-initialize essential callbacks 
                a2dp_source.set_data_callback_in_frames(get_data_frames);
                a2dp_source.set_on_connection_state_changed(a2dp_connection_state_changed_callback);
                a2dp_source.set_ssid_callback(a2dp_ssid_callback); 
                a2dp_source.set_volume(currentBluetoothVolume); 

                stateBeforeScan = SETTINGS_MENU_BLUETOOTH;
                setState(BLUETOOTH_SCANNING); 
                scanInProgress = true; // Flag that scan should be active
                scanStartTime = 0;     // Timer will start in handleBluetoothScanning
                scanMenuSelection = 0;
                scanMenuScrollOffset = 0;
                discoveredBtDevices.clear(); 
                
                a2dp_source.start(); // Start A2DP in discovery mode 
                
                needsActionRedraw = false; 
                StickCP2.Lcd.fillScreen(BLACK); 
                // --- End Scan Setup ---
            }
            else if (strcmp(items[currentMenuSelection], "Back") == 0) {
                settingsMenuLevel = 0; currentMenuSelection = 1; menuScrollOffset = 0; 
            }
        }
        if (needsActionRedraw) redrawMenu = true;
    }
}

void handleEditSettingInput() {
    resetActivityTimer();
    bool valueChanged = false;
    int rotation = StickCP2.Lcd.getRotation();
    bool upPressed = (rotation == 3) ? M5.BtnPWR.wasClicked() : StickCP2.BtnB.wasClicked();
    bool downPressed = (rotation == 3) ? StickCP2.BtnB.wasClicked() : M5.BtnPWR.wasClicked();

    if (upPressed || downPressed) {
        valueChanged = true;
        int increment = upPressed ? 1 : -1;

        switch(settingBeingEdited) {
            case EDIT_MAX_SHOTS: editingIntValue = min(max(editingIntValue + increment, 1), MAX_SHOTS_LIMIT); break;
            case EDIT_BEEP_DURATION: editingULongValue = min(max(editingULongValue + (unsigned long)(increment * 50), 50UL), 2000UL); break;
            case EDIT_BEEP_TONE: editingIntValue = min(max(editingIntValue + (increment * 100), 500), 8000); break;
            case EDIT_SHOT_THRESHOLD: editingIntValue = min(max(editingIntValue + (increment * 500), 100), 32000); break;
            case EDIT_MIN_FIRST_SHOT: editingIntValue = min(max(editingIntValue + (increment * 10), 0), 500); break;
            case EDIT_PAR_BEEP_COUNT: editingIntValue = min(max(editingIntValue + increment, 1), MAX_PAR_BEEPS); break;
            case EDIT_PAR_TIME_ARRAY: editingFloatValue = min(max(editingFloatValue + (increment * 0.1f), 0.1f), 10.0f); break;
            case EDIT_RECOIL_THRESHOLD: editingFloatValue = min(max(editingFloatValue + (increment * 0.1f), 0.5f), 5.0f); break;
            case EDIT_ROTATION: editingIntValue = (editingIntValue + increment + 4) % 4; break;
            case EDIT_BOOT_ANIM: editingBoolValue = !editingBoolValue; break;
            case EDIT_AUTO_SLEEP: editingBoolValue = !editingBoolValue; break;
            case EDIT_SHOW_TOTAL_TIME: editingBoolValue = !editingBoolValue; break;
            case EDIT_BT_AUTO_RECONNECT: editingBoolValue = !editingBoolValue; break;
            case EDIT_BT_VOLUME:
                editingIntValue = min(max(editingIntValue + (increment * 5), 0), 127);
                break;
            case EDIT_BT_AUDIO_OFFSET: 
                editingIntValue = min(max(editingIntValue + (increment * BT_AUDIO_OFFSET_STEP_MS), -1000), 500); 
                if (a2dp_source.is_connected()) { 
                    playSyncCalibrationTone(currentBeepToneHz, BEEP_NOTE_DURATION_MS, editingIntValue);
                } else {
                    playFeedbackTone(2500, 20); 
                }
                break;
            default: valueChanged = false; break;
        }
        if (settingBeingEdited == EDIT_ROTATION) {
            StickCP2.Lcd.setRotation(editingIntValue);
            redrawMenu = true;
        }
        if (valueChanged && 
            settingBeingEdited != EDIT_BOOT_ANIM && 
            settingBeingEdited != EDIT_AUTO_SLEEP && 
            settingBeingEdited != EDIT_SHOW_TOTAL_TIME &&
            settingBeingEdited != EDIT_BT_AUTO_RECONNECT &&
            settingBeingEdited != EDIT_BT_AUDIO_OFFSET) { 
            playFeedbackTone(2500, 20); 
        }
    }

    if (StickCP2.BtnA.pressedFor(LONG_PRESS_DURATION_MS)) {
        if (settingBeingEdited == EDIT_ROTATION) {
             StickCP2.Lcd.setRotation(screenRotationSetting); 
        }
        setState(stateBeforeEdit);
        StickCP2.Lcd.fillScreen(BLACK);
        settingBeingEdited = EDIT_NONE;
        playUnsuccessBeeps();
        return;
    }

    if (StickCP2.BtnA.wasClicked()) {
        switch(settingBeingEdited) {
            case EDIT_MAX_SHOTS: currentMaxShots = editingIntValue; break;
            case EDIT_BEEP_DURATION: currentBeepDuration = editingULongValue; break;
            case EDIT_BEEP_TONE: currentBeepToneHz = editingIntValue; break;
            case EDIT_SHOT_THRESHOLD: shotThresholdRms = editingIntValue; break;
            case EDIT_MIN_FIRST_SHOT: minFirstShotTimeMs = editingIntValue; break;
            case EDIT_PAR_BEEP_COUNT: dryFireParBeepCount = editingIntValue; break;
            case EDIT_PAR_TIME_ARRAY:
                if (editingIntValue >= 0 && editingIntValue < MAX_PAR_BEEPS) { 
                    dryFireParTimesSec[editingIntValue] = editingFloatValue;
                }
                break;
            case EDIT_RECOIL_THRESHOLD: recoilThreshold = editingFloatValue; break;
            case EDIT_ROTATION: screenRotationSetting = editingIntValue; break;
            case EDIT_BOOT_ANIM: playBootAnimation = editingBoolValue; break;
            case EDIT_AUTO_SLEEP: enableAutoSleep = editingBoolValue; break;
            case EDIT_SHOW_TOTAL_TIME: showTotalTime = editingBoolValue; break;
            case EDIT_BT_AUTO_RECONNECT:
                currentBluetoothAutoReconnect = editingBoolValue;
                break;
            case EDIT_BT_VOLUME:
                currentBluetoothVolume = editingIntValue;
                a2dp_source.set_volume(currentBluetoothVolume);
                break;
            case EDIT_BT_AUDIO_OFFSET: 
                currentBluetoothAudioOffsetMs = editingIntValue;
                break;
            default: break;
        }
        setState(stateBeforeEdit);
        StickCP2.Lcd.fillScreen(BLACK);
        settingBeingEdited = EDIT_NONE;
        playSuccessBeeps();
        return;
    }

    if (redrawMenu || valueChanged) {
        displayEditScreen();
        redrawMenu = false;
    }
}

void handleDeviceStatusInput() {
    resetActivityTimer();
    if (redrawMenu) {
        displayDeviceStatusScreen();
        redrawMenu = false;
    }
    if (StickCP2.BtnA.pressedFor(LONG_PRESS_DURATION_MS)) {
        setState(SETTINGS_MENU_MAIN);
        currentMenuSelection = 4; 
        int rotation = StickCP2.Lcd.getRotation();
        int itemsPerScreen = (rotation % 2 == 0) ? MENU_ITEMS_PER_SCREEN_PORTRAIT : MENU_ITEMS_PER_SCREEN_LANDSCAPE;
        menuScrollOffset = max(0, currentMenuSelection - itemsPerScreen + 1);
        StickCP2.Lcd.fillScreen(BLACK);
    }
}

void handleListFilesInput() {
    resetActivityTimer();
    int rotation = StickCP2.Lcd.getRotation();
    int itemsPerScreen = (rotation % 2 == 0) ? MENU_ITEMS_PER_SCREEN_PORTRAIT + 2 : MENU_ITEMS_PER_SCREEN_LANDSCAPE + 1;

    if (redrawMenu) {
        fileListCount = 0;
        File root = LittleFS.open("/");
        if (root && root.isDirectory()) {
            File file = root.openNextFile();
            while(file && fileListCount < MAX_FILES_LIST){
                if(!file.isDirectory()){
                    fileListNames[fileListCount] = String(file.name());
                    fileListSizes[fileListCount] = file.size();
                    fileListCount++;
                }
                file = root.openNextFile();
            }
            root.close();
        }
    }

    bool upPressed = (rotation == 3) ? M5.BtnPWR.wasClicked() : StickCP2.BtnB.wasClicked();
    bool downPressed = (rotation == 3) ? StickCP2.BtnB.wasClicked() : M5.BtnPWR.wasClicked();

    if (upPressed) {
        if (fileListScrollOffset > 0) {
            fileListScrollOffset--;
            redrawMenu = true;
        }
    }
    if (downPressed) {
        if (fileListScrollOffset + itemsPerScreen < fileListCount) {
            fileListScrollOffset++;
            redrawMenu = true;
        }
    }

    if (redrawMenu) {
        displayListFilesScreen();
        redrawMenu = false;
    }

     if (StickCP2.BtnA.pressedFor(LONG_PRESS_DURATION_MS)) {
         setState(SETTINGS_MENU_MAIN);
         currentMenuSelection = 5; 
         menuScrollOffset = max(0, currentMenuSelection - itemsPerScreen + 1);
         StickCP2.Lcd.fillScreen(BLACK);
     }
}

void handleCalibrationInput(TimerState calibrationType) {
    resetActivityTimer();
    float currentValue = 0.0f;
    float accX, accY, accZ;
    const char* title = "Calibrating...";
    const char* unit = "";
    bool valueChanged = false;
    int rotation = StickCP2.Lcd.getRotation();
    int itemsPerScreen = (rotation % 2 == 0) ? MENU_ITEMS_PER_SCREEN_PORTRAIT : MENU_ITEMS_PER_SCREEN_LANDSCAPE;

    if (calibrationType == CALIBRATE_THRESHOLD) {
        title = "Calibrate Threshold";
        unit = "RMS";
        currentValue = micPeakRMS.getPeakRMS();
        if (currentValue > peakRMSOverall) {
            peakRMSOverall = currentValue;
            valueChanged = true;
        }
        micPeakRMS.resetPeak();
    } else if (calibrationType == CALIBRATE_RECOIL) {
        title = "Calibrate Recoil";
        unit = "G";
        StickCP2.Imu.getAccelData(&accX, &accY, &accZ);
        currentValue = abs(accZ);
        if (currentValue > peakRecoilValue) {
            peakRecoilValue = currentValue;
            valueChanged = true;
        }
    }

    if (redrawMenu || valueChanged) {
        if (!redrawMenu && valueChanged) {
             StickCP2.Lcd.fillRect(0, StickCP2.Lcd.height() / 2 - 25, StickCP2.Lcd.width(), 50, BLACK);
        } else {
            StickCP2.Lcd.fillScreen(BLACK);
            displayCalibrationScreen(title, (calibrationType == CALIBRATE_RECOIL ? peakRecoilValue : peakRMSOverall), unit);
        }
        StickCP2.Lcd.setTextDatum(MC_DATUM);
        StickCP2.Lcd.setTextFont(1);
        StickCP2.Lcd.setTextSize(3);
        String peakStr = "PEAK: " + String((calibrationType == CALIBRATE_RECOIL ? peakRecoilValue : peakRMSOverall), (calibrationType == CALIBRATE_RECOIL ? 2 : 0));
        StickCP2.Lcd.drawString(peakStr, StickCP2.Lcd.width() / 2, (StickCP2.Lcd.height()/2));
        drawLowBatteryIndicator(); 
        redrawMenu = false;
    }

    if (StickCP2.BtnA.pressedFor(LONG_PRESS_DURATION_MS)) {
        stateBeforeEdit = (calibrationType == CALIBRATE_THRESHOLD) ? SETTINGS_MENU_GENERAL : SETTINGS_MENU_NOISY;
        setState(stateBeforeEdit);
        currentMenuSelection = (calibrationType == CALIBRATE_THRESHOLD) ? 6 : 1; 
        menuScrollOffset = max(0, currentMenuSelection - itemsPerScreen + 1);
        StickCP2.Lcd.fillScreen(BLACK);
        playUnsuccessBeeps();
    } else if (StickCP2.BtnA.wasClicked()) {
        if (calibrationType == CALIBRATE_THRESHOLD) {
            shotThresholdRms = (int)peakRMSOverall;
            stateBeforeEdit = SETTINGS_MENU_GENERAL;
            setState(stateBeforeEdit);
            currentMenuSelection = 6;
            menuScrollOffset = max(0, currentMenuSelection - itemsPerScreen + 1);
        } else if (calibrationType == CALIBRATE_RECOIL) {
            recoilThreshold = peakRecoilValue;
            stateBeforeEdit = SETTINGS_MENU_NOISY;
            setState(stateBeforeEdit);
            currentMenuSelection = 1;
            menuScrollOffset = max(0, currentMenuSelection - itemsPerScreen + 1);
        }
        StickCP2.Lcd.fillScreen(BLACK);
        playSuccessBeeps();
    }
}


bool checkTimerExitButtons() {
    return false; 
}
