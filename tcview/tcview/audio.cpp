#include "stdafx.h"
#include "audio.h"
#include "tcview.h"

std::vector<std::string> Extensions{"MP3", "OGG", "WAV"};

// helper functions

void __ParseExtensions(const char *exts)
{
  std::vector<std::string> lexts = string_split(exts, ';');
  for(auto &i: lexts)
    Extensions.push_back(i.substr(2));
}

Sound::Sound(std::string filename)
{
  this->load(filename);
}

void Sound::load(std::string filename)
{
  this->sound = BASS_StreamCreateFile(false, filename.c_str(), 0, 0, BASS_STREAM_AUTOFREE);
  if(this->sound == 0)
    throw std::invalid_argument("invalid filename");
}

void Sound::play()
{
  BASS_ChannelPlay(this->sound, true);
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

std::string Sound::get_filename()
{
  BOOL success;
  BASS_CHANNELINFO info;
  success = BASS_ChannelGetInfo(this->sound, &info);
  if(!success)
    throw std::runtime_error("unable to retrieve filename");
  return std::string{info.filename};
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

void AudioInitialize()
{
  BASS_Init(-1, 48000, BASS_DEVICE_STEREO, NULL, NULL);
}

void AudioShutdown()
{
  BASS_Free();
}

bool AudioCanLoad()
{
  return HIWORD(BASS_GetVersion()) == BASSVERSION;
}

std::vector<std::string> AudioGetExtensions()
{
  return Extensions;
}

void AudioUnloadPlugins()
{
  BASS_PluginFree(0);
}

void AudioLoadPlugins()
{
  BASS_PLUGININFO *hPluginInfo;
  HPLUGIN hPlugin;
  HANDLE hFind = INVALID_HANDLE_VALUE;
  unsigned int i;
  std::string currentdir = GetModuleDirectory();
  std::string fullpath{0};
  std::string searchpattern{0};
  WIN32_FIND_DATAA ffd;
  // append the wildcards
  searchpattern = currentdir + "\\plugins\\*.dll";

  // searching all plugin files
  hFind = FindFirstFileA(searchpattern.c_str(), &ffd);
  if(hFind == INVALID_HANDLE_VALUE)
    return;

  do
  {
    if(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
      continue;
    else
    {
      fullpath = currentdir + "\\plugins\\" + ffd.cFileName;
      hPlugin = BASS_PluginLoad(fullpath.c_str(), 0);
      if(!hPlugin)
        continue;
      hPluginInfo = (BASS_PLUGININFO*)BASS_PluginGetInfo(hPlugin);
      for(i=0;i<hPluginInfo->formatc;i++)
        __ParseExtensions(hPluginInfo->formats[i].exts);
    }
  }
  while(FindNextFileA(hFind, &ffd) != 0);
}
