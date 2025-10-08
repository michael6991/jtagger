#include "Arduino.h"

#include "../include/art.h"

void print_welcome()
{
    Serial.println();
    Serial.write(art0, sizeof(art0)); Serial.flush();
    Serial.write(art1, sizeof(art1)); Serial.flush();
    Serial.write(art2, sizeof(art2)); Serial.flush();
    Serial.write(art3, sizeof(art3)); Serial.flush();
    Serial.write(art4, sizeof(art4)); Serial.flush();
    Serial.write(art5, sizeof(art5)); Serial.flush();
    Serial.write(art6, sizeof(art6)); Serial.flush();
    Serial.write(art7, sizeof(art7)); Serial.flush();
    Serial.write(art8, sizeof(art8)); Serial.flush();
}
