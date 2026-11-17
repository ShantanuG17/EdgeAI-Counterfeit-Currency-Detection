/* Fake Currency Detection — Vec15
 * Regions: R1=Ink, R2=Thread, R3=Blank
 * Features (15 total):
 *   R1 (3): UV_C_460nm, IR_V_900nm, IR_K_680nm
 *   R2 (4): UV_C_460nm, UV_A_410nm, UV_R_730nm, White_C_460nm
 *   R3 (8): UV_C_460nm, UV_E_510nm, UV_B_435nm, UV_F_535nm,
 *            White_B_435nm, White_E_510nm, White_C_460nm, IR_A_410nm
 *
 * Label mapping: 0 = Fake, 1 = Genuine
 *
 * Fixes applied vs original:
 *   1. setIlluminationCurrent()  → setBulbCurrent() with correct enum constant
 *   2. sensor.getGain()          → tracked via static currentGain variable
 *      (AS7265X library has no getGain() getter)
 */

#include <Project_inferencing.h>
#include <Wire.h>
#include "SparkFun_AS7265X.h"

AS7265X sensor;

// ── Track gain globally — library has no getGain() getter ───────────────────
static uint8_t currentGain = AS7265X_GAIN_64X;

// ── Feature array (15 values, order must match EI training columns) ──────────
// [0]  R1_UV_C_460nm
// [1]  R1_IR_V_900nm
// [2]  R1_IR_K_680nm
// [3]  R2_UV_C_460nm
// [4]  R2_UV_A_410nm
// [5]  R2_UV_R_730nm
// [6]  R2_White_C_460nm
// [7]  R3_UV_C_460nm
// [8]  R3_UV_E_510nm
// [9]  R3_UV_B_435nm
// [10] R3_UV_F_535nm
// [11] R3_White_B_435nm
// [12] R3_White_E_510nm
// [13] R3_White_C_460nm
// [14] R3_IR_A_410nm
static float features[EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE];

// ── State machine ────────────────────────────────────────────────────────────
typedef enum { WAIT_R1, WAIT_R2, WAIT_R3, INFER } State;
State state = WAIT_R1;

// ── EI callback ──────────────────────────────────────────────────────────────
int raw_feature_get_data(size_t offset, size_t length, float *out_ptr) {
    memcpy(out_ptr, features + offset, length * sizeof(float));
    return 0;
}

void print_inference_result(ei_impulse_result_t result);

// ── Sensor helpers ───────────────────────────────────────────────────────────

void scanUV(float &uv_A, float &uv_B, float &uv_C, float &uv_E, float &uv_F, float &uv_R) {
    sensor.setMeasurementMode(AS7265X_MEASUREMENT_MODE_6CHAN_CONTINUOUS); // FIX #1
    sensor.disableIndicator();
    sensor.setBulbCurrent(AS7265X_LED_CURRENT_LIMIT_12_5MA, AS7265x_LED_UV);
    sensor.enableBulb(AS7265x_LED_UV);
    sensor.takeMeasurements();
    sensor.disableBulb(AS7265x_LED_UV);

    uv_A = sensor.getCalibratedA();   // 410 nm
    uv_B = sensor.getCalibratedB();   // 435 nm
    uv_C = sensor.getCalibratedC();   // 460 nm
    uv_E = sensor.getCalibratedE();   // 510 nm
    uv_F = sensor.getCalibratedF();   // 535 nm
    uv_R = sensor.getCalibratedR();   // 730 nm
}

void scanIR(float &ir_A, float &ir_K, float &ir_V) {
    sensor.setMeasurementMode(AS7265X_MEASUREMENT_MODE_6CHAN_CONTINUOUS); // FIX #1
    sensor.disableIndicator();
    sensor.enableBulb(AS7265x_LED_IR);
    sensor.takeMeasurements();
    sensor.disableBulb(AS7265x_LED_IR);

    ir_A = sensor.getCalibratedA();   // 410 nm  (NIR chip)
    ir_K = sensor.getCalibratedK();   // 680 nm
    ir_V = sensor.getCalibratedV();   // 900 nm
}

void scanWhite(float &white_B, float &white_C, float &white_E) {
    sensor.setMeasurementMode(AS7265X_MEASUREMENT_MODE_6CHAN_CONTINUOUS);

    // Drop gain to 1x BEFORE enabling White LED to avoid saturation.
    // Use currentGain to restore afterwards — library has no getGain(). FIX #2
    sensor.setGain(AS7265X_GAIN_16X);// FIX #1
    sensor.disableIndicator();
    sensor.enableBulb(AS7265x_LED_WHITE);
    sensor.takeMeasurements();
    sensor.disableBulb(AS7265x_LED_WHITE);

    // Read ALL values BEFORE restoring gain
    white_B = sensor.getCalibratedB();   // 435 nm
    white_C = sensor.getCalibratedC();   // 460 nm
    white_E = sensor.getCalibratedE();   // 510 nm

    sensor.setGain(currentGain);  // restore to whatever was set in setup()
}

// ── Setup ────────────────────────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);
    while (!Serial);

    Serial.println("=== Fake Currency Detector — Vec15 ===");

    Wire.begin();
    if (!sensor.begin()) {
        Serial.println("ERR: AS7265X not found. Check wiring.");
        while (1);
    }

    // Keep currentGain in sync with whatever gain you set here
    currentGain = AS7265X_GAIN_64X;
    sensor.setGain(currentGain);
    sensor.setIntegrationCycles(50);  // ~280 ms integration
    sensor.disableIndicator();

    Serial.println("Sensor ready.");
    Serial.println();
    Serial.println("Send any key to scan R1 (Ink region)...");
}

// ── Main loop ────────────────────────────────────────────────────────────────
void loop() {

    if (!Serial.available()) return;
    Serial.read();  // consume incoming byte

    switch (state) {

        // ── R1: Ink ──────────────────────────────────────────────────────────
        case WAIT_R1: {
            Serial.println("\n[R1] Scanning Ink region...");

            float uv_A, uv_B, uv_C, uv_E, uv_F, uv_R;
            float ir_A, ir_K, ir_V;

            scanUV(uv_A, uv_B, uv_C, uv_E, uv_F, uv_R);
            scanIR(ir_A, ir_K, ir_V);

            // R1 features: UV_C_460nm, IR_V_900nm, IR_K_680nm
            features[0] = uv_C;
            features[1] = ir_V;
            features[2] = ir_K;

            Serial.printf("  R1_UV_C_460nm : %.4f\n", features[0]);
            Serial.printf("  R1_IR_V_900nm : %.4f\n", features[1]);
            Serial.printf("  R1_IR_K_680nm : %.4f\n", features[2]);

            state = WAIT_R2;
            Serial.println("\nSend any key to scan R2 (Security Thread region)...");
            break;
        }

        // ── R2: Security Thread ──────────────────────────────────────────────
        case WAIT_R2: {
            Serial.println("\n[R2] Scanning Security Thread region...");

            float uv_A, uv_B, uv_C, uv_E, uv_F, uv_R;
            float white_B, white_C, white_E;

            scanUV(uv_A, uv_B, uv_C, uv_E, uv_F, uv_R);
            scanWhite(white_B, white_C, white_E);

            // R2 features: UV_C_460nm, UV_A_410nm, UV_R_730nm, White_C_460nm
            features[3] = uv_C;
            features[4] = uv_A;
            features[5] = uv_R;
            features[6] = white_C;

            Serial.printf("  R2_UV_C_460nm    : %.4f\n", features[3]);
            Serial.printf("  R2_UV_A_410nm    : %.4f\n", features[4]);
            Serial.printf("  R2_UV_R_730nm    : %.4f\n", features[5]);
            Serial.printf("  R2_White_C_460nm : %.4f\n", features[6]);

            state = WAIT_R3;
            Serial.println("\nSend any key to scan R3 (Blank Paper region)...");
            break;
        }

        // ── R3: Blank Paper ──────────────────────────────────────────────────
        case WAIT_R3: {
            Serial.println("\n[R3] Scanning Blank Paper region...");

            float uv_A, uv_B, uv_C, uv_E, uv_F, uv_R;
            float ir_A, ir_K, ir_V;
            float white_B, white_C, white_E;

            scanUV(uv_A, uv_B, uv_C, uv_E, uv_F, uv_R);
            scanIR(ir_A, ir_K, ir_V);
            scanWhite(white_B, white_C, white_E);

            // R3 features: UV_C, UV_E, UV_B, UV_F, White_B, White_E, White_C, IR_A
            features[7]  = uv_C;
            features[8]  = uv_E;
            features[9]  = uv_B;
            features[10] = uv_F;
            features[11] = white_B;
            features[12] = white_E;
            features[13] = white_C;
            features[14] = ir_A;

            Serial.printf("  R3_UV_C_460nm    : %.4f\n", features[7]);
            Serial.printf("  R3_UV_E_510nm    : %.4f\n", features[8]);
            Serial.printf("  R3_UV_B_435nm    : %.4f\n", features[9]);
            Serial.printf("  R3_UV_F_535nm    : %.4f\n", features[10]);
            Serial.printf("  R3_White_B_435nm : %.4f\n", features[11]);
            Serial.printf("  R3_White_E_510nm : %.4f\n", features[12]);
            Serial.printf("  R3_White_C_460nm : %.4f\n", features[13]);
            Serial.printf("  R3_IR_A_410nm    : %.4f\n", features[14]);

            state = INFER;
            // Fall through immediately to inference
        }

        // ── Inference ─────────────────────────────────────────────────────────
        case INFER: {
            Serial.println("\n[INFER] Running classifier...");

            if (EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE != 15) {
                Serial.printf("ERR: Model expects %d features, code provides 15\n",
                              EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE);
                state = WAIT_R1;
                return;
            }

            ei_impulse_result_t result = { 0 };

            signal_t features_signal;
            features_signal.total_length = 15;
            features_signal.get_data = &raw_feature_get_data;

            EI_IMPULSE_ERROR res = run_classifier(&features_signal, &result, false);
            if (res != EI_IMPULSE_OK) {
                Serial.printf("ERR: Classifier failed (%d)\n", res);
                state = WAIT_R1;
                return;
            }

            print_inference_result(result);

            // Reset for next banknote
            state = WAIT_R1;
            Serial.println("\n─────────────────────────────────────────");
            Serial.println("Send any key to scan next note (R1 Ink)...");
            break;
        }
    }
}

// ── Print result ──────────────────────────────────────────────────────────────
void print_inference_result(ei_impulse_result_t result) {

    ei_printf("\nTiming: DSP %d ms, inference %d ms\r\n",
              result.timing.dsp,
              result.timing.classification);

    ei_printf("\nAll scores:\r\n");
    for (uint16_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
        ei_printf("  %s: %.5f\r\n",
                  ei_classifier_inferencing_categories[i],
                  result.classification[i].value);
    }

    // EI sorts labels alphabetically: index 0 = Fake, index 1 = Genuine
    float fake_score    = result.classification[0].value;
    float genuine_score = result.classification[1].value;

    ei_printf("\n╔══════════════════════════════════════╗\n");
    if (genuine_score > fake_score) {
        ei_printf("║  ✓ GENUINE NOTE  (%.1f%%)             ║\n", genuine_score * 100);
    } else {
        ei_printf("║  ✗ FAKE NOTE     (%.1f%%)             ║\n", fake_score * 100);
    }
    ei_printf("╚══════════════════════════════════════╝\n");

#if EI_CLASSIFIER_HAS_ANOMALY
    ei_printf("Anomaly score: %.3f\r\n", result.anomaly);
#endif
}
