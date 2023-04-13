#include "error.h"

/**
 * @brief Inits the error handling functions
 *
 */
void error_init() {
    pinMode(LED_BUILTIN, OUTPUT);
    Serial.begin(115200);
}

/**
 * @brief Halts execution after a fatal error
 *
 * @param msg cause of fatal error
 */
void fatal_error(const char *msg) {
    Serial.println("### FATAL ERROR ###");
    Serial.println(msg);
    Serial.println("Please reset.");
    while (1) {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(1000);
        digitalWrite(LED_BUILTIN, LOW);
        delay(1000);
        Serial.print(".");
    }
}

/**
 * @brief Logging function to output to Serial
 *
 * @param msg logging message
 * @param msg_type severity (DEBUG, INFO, WARNING, ERROR)
 */
void print_msg(const char *msg, error_t msg_type) {
    if (msg_type < MSG_LEVEL) return;
    switch (msg_type) {
        case DEBUG:
            Serial.print("(debug) ");
            break;
        case INFO:
            Serial.print("(Info) ");
            break;
        case WARNING:
            Serial.print("(Warning) ");
            break;
        case ERROR:
            Serial.print("(ERROR) ");
            break;
        default:
            fatal_error("Undefined message type!");
    }
    Serial.println(msg);
}

/**
 * @brief dumps an array to serial
 *
 * @param array target array to dump
 * @param len number of elements to print
 * @param msg describes what the array is
 */
void dump_array_uint16(uint16_t *array, uint16_t len, const char *msg) {
    Serial.print(msg);
    Serial.print(" [ ");
    for (uint16_t i = 0; i < len; i++) {
        Serial.print(array[i]);
        Serial.print(", ");
    }
    Serial.println("]");
}

/**
 * @brief dumps an array to serial
 *
 * @param array target array to dump
 * @param len number of elements to print
 * @param msg describes what the array is
 */
void dump_array_fix15(fix15 *array, uint16_t len, const char *msg) {
    Serial.print(msg);
    Serial.print(" [ ");
    for (uint16_t i = 0; i < len; i++) {
        Serial.print(fix2float15(array[i]));
        Serial.print(", ");
    }
    Serial.println("]");
}

/**
 * @brief dumps an array to serial
 *
 * @param array target array to dump
 * @param len number of elements to print
 * @param msg describes what the array is
 */
void dump_array_double(double *array, uint16_t len, const char *msg) {
    Serial.print(msg);
    Serial.print(" [ ");
    for (uint16_t i = 0; i < len; i++) {
        Serial.print(array[i]);
        Serial.print(", ");
    }
    Serial.println("]");
}