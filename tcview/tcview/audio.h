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
    bool is_playing();
    bool is_paused();
    bool is_stopped();
    void set_position(double pos);
    double get_position();
    double get_length();
    float get_volume();
    void set_volume(float vol);
};

bool AudioCanLoad();
std::vector<std::string> AudioGetExtensions();
void AudioInitialize();
void AudioLoadPlugins();
void AudioShutdown();
void AudioUnloadPlugins();
