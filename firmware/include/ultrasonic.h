#ifndef ULTRASONIC_H
#define ULTRASONIC_H

#define ULTRASONIC_PAYLOAD_ID 1
#define ULTRASONIC_DISTANCE_LENGTH 20

// [0 ][1-3][4-7   ][8-(8+LEN)]
// [id][pad][millis][distances]
typedef struct {
    const char    payload_id = ULTRASONIC_PAYLOAD_ID;                            // payload id
    unsigned long m;                                     // time since esp32 was turned on
    float         distances[ULTRASONIC_DISTANCE_LENGTH]; // raw cm values of the distance sensor
} UltrasonicPayload;

#endif // ULTRASONIC_H 