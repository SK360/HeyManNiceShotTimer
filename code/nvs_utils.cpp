#include "nvs_utils.h"
#include "globals.h" // Access to global variables and preferences object
#include "config.h"  // Access to NVS_NAMESPACE and KEY_ constants

void loadSettings() {
    preferences.begin(NVS_NAMESPACE, false); // Open NVS

    currentMaxShots = preferences.getInt(KEY_MAX_SHOTS, 10);
    if (currentMaxShots > MAX_SHOTS_LIMIT) currentMaxShots = MAX_SHOTS_LIMIT;
    else if (currentMaxShots <= 0) currentMaxShots = 1;

    currentBeepDuration = preferences.getULong(KEY_BEEP_DUR, 150);
    currentBeepToneHz = preferences.getInt(KEY_BEEP_HZ, 2000);
    shotThresholdRms = preferences.getInt(KEY_SHOT_THRESH, 15311);
    dryFireParBeepCount = preferences.getInt(KEY_DF_BEEP_CNT, 3);
    if (dryFireParBeepCount < 1) dryFireParBeepCount = 1;
    if (dryFireParBeepCount > MAX_PAR_BEEPS) dryFireParBeepCount = MAX_PAR_BEEPS;

    for (int i = 0; i < MAX_PAR_BEEPS; ++i) {
        char key[12];
        sprintf(key, "dfParT_%d", i);
        dryFireParTimesSec[i] = preferences.getFloat(key, 1.0f);
    }

    recoilThreshold = preferences.getFloat(KEY_NR_RECOIL, 1.5f);
    screenRotationSetting = preferences.getInt(KEY_ROTATION, 3);
    if (screenRotationSetting < 0 || screenRotationSetting > 3) screenRotationSetting = 3;
    playBootAnimation = preferences.getBool(KEY_BOOT_ANIM, false);
    enableAutoSleep = preferences.getBool(KEY_AUTO_SLEEP, true);
    showTotalTime = preferences.getBool(KEY_SHOW_TOTAL_TIME, true);
    minFirstShotTimeMs = preferences.getInt(KEY_MIN_FIRST_SHOT, DEFAULT_MIN_FIRST_SHOT_TIME_MS);
    if (minFirstShotTimeMs < 0) minFirstShotTimeMs = 0;
    if (minFirstShotTimeMs > 500) minFirstShotTimeMs = 500;
    postBeepDelayMs = preferences.getInt(KEY_POST_BEEP_DELAY, DEFAULT_POST_BEEP_DELAY_MS);
    if (postBeepDelayMs < 50) postBeepDelayMs = 50;
    if (postBeepDelayMs > 1000) postBeepDelayMs = 1000;

    currentBluetoothDeviceName = preferences.getString(KEY_BT_DEVICE_NAME, "LEXON MINO L");
    currentBluetoothAutoReconnect = preferences.getBool(KEY_BT_AUTO_RECONNECT, false);
    currentBluetoothVolume = preferences.getInt(KEY_BT_VOLUME, 80);

    peakBatteryVoltage = preferences.getFloat(KEY_PEAK_BATT, 4.2f); // Also load peak battery here

    // preferences.end(); // Keep NVS open if frequently accessed, or close if done for now
}

void saveSettings() {
    // preferences.begin(NVS_NAMESPACE, false); // Ensure NVS is open if not already

    preferences.putInt(KEY_MAX_SHOTS, currentMaxShots);
    preferences.putULong(KEY_BEEP_DUR, currentBeepDuration);
    preferences.putInt(KEY_BEEP_HZ, currentBeepToneHz);
    preferences.putInt(KEY_SHOT_THRESH, shotThresholdRms);
    preferences.putInt(KEY_DF_BEEP_CNT, dryFireParBeepCount);
    for (int i = 0; i < MAX_PAR_BEEPS; ++i) {
         char key[12];
         sprintf(key, "dfParT_%d", i);
         preferences.putFloat(key, dryFireParTimesSec[i]);
    }
    preferences.putFloat(KEY_NR_RECOIL, recoilThreshold);
    preferences.putInt(KEY_ROTATION, screenRotationSetting);
    preferences.putBool(KEY_BOOT_ANIM, playBootAnimation);
    preferences.putBool(KEY_AUTO_SLEEP, enableAutoSleep);
    preferences.putBool(KEY_SHOW_TOTAL_TIME, showTotalTime);
    preferences.putInt(KEY_MIN_FIRST_SHOT, minFirstShotTimeMs);
    preferences.putInt(KEY_POST_BEEP_DELAY, postBeepDelayMs);

    preferences.putString(KEY_BT_DEVICE_NAME, currentBluetoothDeviceName);
    preferences.putBool(KEY_BT_AUTO_RECONNECT, currentBluetoothAutoReconnect);
    preferences.putInt(KEY_BT_VOLUME, currentBluetoothVolume);

    // preferences.end(); // Close NVS if done with batch of writes
}

void savePeakVoltage(float voltage) {
    // preferences.begin(NVS_NAMESPACE, false); // Ensure NVS is open
    preferences.putFloat(KEY_PEAK_BATT, voltage);
    // preferences.end();
}
