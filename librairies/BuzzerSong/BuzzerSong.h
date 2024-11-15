#ifndef BUZZER_SONG_H
#define BUZZER_SONG_H

#include "Arduino.h"

class BuzzerSong
{
public:
    BuzzerSong(int buzzerPin);
    void playWeWishYou();

private:
    int _buzzerPin;
    int tempo;
    int melody[100];
    int notes;
    int wholenote;
    int divider, noteDuration;
};

#endif