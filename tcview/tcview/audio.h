#pragma once

#include "bass.h"
#include <filesystem>
#include <string>
#include <vector>

namespace Audio
{
  class Sound
  {
    private:
      HSTREAM sound;
    public:
      Sound(std::filesystem::path filename);
      void fade_out();
      std::filesystem::path get_filename();
      void load(std::filesystem::path filename);
      void pause();
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

  bool CanLoad();
  std::vector<std::string> GetExtensions();
  void Initialize();
  void LoadPlugins();
  void Shutdown();
  void UnloadPlugins();
};
