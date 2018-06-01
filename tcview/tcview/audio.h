#pragma once

#include "bass.h"
#include <string>
#include <vector>

class Sound
{
  private:
    HSTREAM sound;
  public:
    Sound(std::string filename);
    void fade_out();
    std::string get_filename();
    void load(std::string filename);
    void play();
    void stop();
    void set_looping(bool looping);
};

bool AudioCanLoad();
std::vector<std::string> AudioGetExtensions();
void AudioInitialize();
void AudioLoadPlugins();
void AudioShutdown();
void AudioUnloadPlugins();
