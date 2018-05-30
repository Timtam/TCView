#pragma once

#include "bass.h"
#include <string>

class Sound
{
  private:
    HSTREAM sound;
  public:
    Sound();
    void fade_out();
    void load(std::string filename);
    void play();
    void stop();
};

void InitializeAudio();
