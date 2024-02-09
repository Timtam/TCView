#include "stdafx.h"
#include "audio.h"
#include "tcview.h"

namespace Audio
{
  std::vector<std::string> Extensions{"MP3", "OGG", "WAV"};

  // helper functions

  void __ParseExtensions(const char *exts)
  {
    std::vector<std::string> lexts = string_split(exts, ';');
    for(auto &i: lexts)
      Extensions.push_back(i.substr(2));
  }

  Sound::Sound(std::filesystem::path filename)
  {
    this->load(filename);
  }

  void Sound::load(std::filesystem::path filename)
  {
    this->sound = BASS_StreamCreateFile(false, filename.c_str(), 0, 0, BASS_STREAM_AUTOFREE | BASS_ASYNCFILE | BASS_UNICODE);
    if(this->sound == 0)
      throw std::invalid_argument("invalid filename");
  }

  void Sound::pause()
  {
    BASS_ChannelPause(this->sound);
  }

  void Sound::play()
  {
    BASS_ChannelPlay(this->sound, false);
  }

  void Sound::fade_out()
  {
    BASS_ChannelSlideAttribute(this->sound, BASS_ATTRIB_VOL | BASS_SLIDE_LOG, -1, 200);
  }

  void Sound::stop()
  {
    BASS_ChannelStop(this->sound);
  }

  void Sound::set_looping(bool looping)
  {
    if(looping)
      BASS_ChannelFlags(this->sound, BASS_SAMPLE_LOOP, BASS_SAMPLE_LOOP);
    else
      BASS_ChannelFlags(this->sound, 0, BASS_SAMPLE_LOOP);
  }

  std::filesystem::path Sound::get_filename()
  {
    BOOL success;
    BASS_CHANNELINFO info;
    success = BASS_ChannelGetInfo(this->sound, &info);
    if(!success)
      throw std::runtime_error("unable to retrieve filename");
    return std::filesystem::path{(wchar_t*)info.filename};
  }

  bool Sound::is_playing()
  {
    return BASS_ChannelIsActive(this->sound) == BASS_ACTIVE_PLAYING;
  }

  bool Sound::is_paused()
  {
    return BASS_ChannelIsActive(this->sound) == BASS_ACTIVE_PAUSED;
  }

  bool Sound::is_stopped()
  {
    return BASS_ChannelIsActive(this->sound) == BASS_ACTIVE_STOPPED;
  }

  double Sound::get_position()
  {
    return BASS_ChannelBytes2Seconds(this->sound, BASS_ChannelGetPosition(this->sound, BASS_POS_BYTE));
  }

  double Sound::get_length()
  {
    return BASS_ChannelBytes2Seconds(this->sound, BASS_ChannelGetLength(this->sound, BASS_POS_BYTE));
  }

  void Sound::set_position(double pos)
  {
    BASS_ChannelSetPosition(this->sound, BASS_ChannelSeconds2Bytes(this->sound, pos), BASS_POS_BYTE);
  }

  float Sound::get_volume()
  {
    float f;
    BASS_ChannelGetAttribute(this->sound, BASS_ATTRIB_VOL, &f);
    return f;
  }

  void Sound::set_volume(float vol)
  {
    BASS_ChannelSetAttribute(this->sound, BASS_ATTRIB_VOL, vol);
  }

  void Initialize()
  {
    BASS_Init(-1, 48000, BASS_DEVICE_STEREO, NULL, NULL);
  }

  void Shutdown()
  {
    BASS_Free();
  }

  bool CanLoad()
  {
    return HIWORD(BASS_GetVersion()) == BASSVERSION;
  }

  std::vector<std::string> GetExtensions()
  {
    return Extensions;
  }

  void UnloadPlugins()
  {
    BASS_PluginFree(0);
  }

  void LoadPlugins()
  {
    BASS_PLUGININFO *hPluginInfo;
    HPLUGIN hPlugin;
    HANDLE hFind = INVALID_HANDLE_VALUE;
    unsigned int i;
    std::filesystem::path currentdir{GetModuleDirectory()};
    std::filesystem::path fullpath;
    std::filesystem::path searchpattern;
    WIN32_FIND_DATAW ffd;
    #ifdef _WIN64
      currentdir.append("x64");
    #else
      currentdir.append("x86");
    #endif
    // append the wildcards
    searchpattern.assign(currentdir);
    searchpattern.append("bass*.dll");

    // searching all plugin files
    hFind = FindFirstFileW(searchpattern.c_str(), &ffd);
    if(hFind == INVALID_HANDLE_VALUE)
      return;

    do
    {
      if(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        continue;
      else
      {
        fullpath = currentdir;
        fullpath.append(ffd.cFileName);
        hPlugin = BASS_PluginLoad(fullpath.c_str(), BASS_UNICODE);
        if(!hPlugin)
          continue;
        hPluginInfo = (BASS_PLUGININFO*)BASS_PluginGetInfo(hPlugin);
        for(i=0;i<hPluginInfo->formatc;i++)
          __ParseExtensions(hPluginInfo->formats[i].exts);
      }
    }
    while(FindNextFileW(hFind, &ffd) != 0);
  }
};