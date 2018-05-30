#pragma once

#include "bass.h"
#include <string>

class Sound
{
  private:
    HSTREAM sound;
  public:
    Sound(std::string filename);
    void fade_out();
    void load(std::string filename);
    void play();
    void stop();
};

bool CanLoadAudio();
void InitializeAudio();
void ShutdownAudio();