#include "audio_utils.h"
#include "globals.h"
#include "config.h" 
#include <freertos/FreeRTOS.h> 
#include <freertos/task.h>     
#include <freertos/queue.h>

// --- LEDC Configuration for Buzzers ---
#define BUZZER_LEDC_CHANNEL 2
#define BUZZER_LEDC_RESOLUTION 8

// --- Buzzer Task (Runs on Core 0) ---
void buzzerTask(void *pvParameters) {
    BuzzerRequest receivedRequest;

    // Attach both buzzer pins to the same LEDC channel/timer
    // so they fire in perfect hardware sync
    ledcSetup(BUZZER_LEDC_CHANNEL, 2000, BUZZER_LEDC_RESOLUTION);
    ledcAttachPin(BUZZER_PIN, BUZZER_LEDC_CHANNEL);
    ledcAttachPin(BUZZER_PIN_2, BUZZER_LEDC_CHANNEL);

    for (;;) {
        if (xQueueReceive(buzzerQueue, &receivedRequest, portMAX_DELAY) == pdPASS) {
            if (receivedRequest.frequency > 0 && receivedRequest.duration > 0) {
                ledcWriteTone(BUZZER_LEDC_CHANNEL, receivedRequest.frequency);
                vTaskDelay(pdMS_TO_TICKS(receivedRequest.duration));
                ledcWriteTone(BUZZER_LEDC_CHANNEL, 0);
            } else if (receivedRequest.duration > 0) {
                 vTaskDelay(pdMS_TO_TICKS(receivedRequest.duration));
            }
        }
         vTaskDelay(pdMS_TO_TICKS(1)); 
    }
}


// --- Audio Control Functions (Called from Core 1) ---

// Resets the Bluetooth beep state variables.
void reset_bt_beep_state() {
    new_bt_beep_request = false;
    current_bt_beep_is_active = false;
    btBeepFrequency = 0; 
}

// Plays a tone, APPLYING the global Bluetooth audio offset.
// Used for critical timing sounds like timer start beeps.
// Plays ONLY on BT if connected, otherwise ONLY on buzzer.
void playTone(int freq, int duration) {
    unsigned long now = millis();
    if (a2dp_source.is_connected()) {
        // --- Bluetooth Path Only ---
        btBeepFrequency = freq;
        btBeepDurationVolatile = duration; 
        btBeepScheduledStartTime = now + currentBluetoothAudioOffsetMs; 
        new_bt_beep_request = true; 
        current_bt_beep_is_active = false; 
        
        // --- DO NOT send to buzzer queue when BT is connected ---

    } else {
        // --- Buzzer Only Path ---
        BuzzerRequest request = {freq, duration};
        xQueueSend(buzzerQueue, &request, (TickType_t)0); 
    }
}

// Plays a tone for immediate UI feedback, IGNORING the global Bluetooth audio offset.
// Plays ONLY on BT if connected, otherwise ONLY on buzzer.
void playFeedbackTone(int freq, int duration) {
    unsigned long now = millis();
    if (a2dp_source.is_connected()) {
        // --- Bluetooth Path Only ---
        btBeepFrequency = freq;
        btBeepDurationVolatile = duration;
        btBeepScheduledStartTime = now; // No offset
        new_bt_beep_request = true;
        current_bt_beep_is_active = false;

        // --- DO NOT send to buzzer queue when BT is connected ---

    } else {
        // --- Buzzer Only Path ---
        BuzzerRequest request = {freq, duration};
        xQueueSend(buzzerQueue, &request, (TickType_t)0); 
    }
}


// Plays a synchronized tone on buzzer and Bluetooth (with specified offset) for calibration.
// This function INTENTIONALLY plays on both buzzer and BT simultaneously.
// 'offsetMs' is the value being tested by the user.
void playSyncCalibrationTone(int freq, int duration, int offsetMs) {
    unsigned long now = millis();
    reset_bt_beep_state(); 

    // 1. Send request to buzzer task (plays immediately on Core 0)
    BuzzerRequest buzzerReq = {freq, duration};
    xQueueSend(buzzerQueue, &buzzerReq, (TickType_t)0); 

    // 2. Set up Bluetooth tone. Its scheduled start is offset from 'now'.
    if (a2dp_source.is_connected()) {
        btBeepFrequency = freq;
        btBeepDurationVolatile = duration;
        btBeepScheduledStartTime = now + offsetMs; 
        new_bt_beep_request = true;
        current_bt_beep_is_active = false; 
    }
}


void playSuccessBeeps() {
    int octave = 6; 
    int freqs[] = {1047, 1175, 1319, 1397, 1568}; 

    for (int f_val : freqs) {
        playFeedbackTone(f_val, BEEP_NOTE_DURATION_MS); // Use feedback tone (queued or BT)
        vTaskDelay(pdMS_TO_TICKS(BEEP_NOTE_DURATION_MS + BEEP_NOTE_DELAY_MS)); 
    }
}

void playUnsuccessBeeps() {
    int freq_val = 262; 
    int repeatTone = 2;
    unsigned long toneDuration = (unsigned long)(BEEP_NOTE_DURATION_MS * 1.5f);
    for (int i = 0; i < repeatTone; ++i) {
        playFeedbackTone(freq_val, toneDuration); // Use feedback tone (queued or BT)
        vTaskDelay(pdMS_TO_TICKS(toneDuration + BEEP_NOTE_DELAY_MS * 2));
    }
}
