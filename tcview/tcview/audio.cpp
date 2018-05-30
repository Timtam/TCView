#include "stdafx.h"
#include "audio.h"

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

void InitializeAudio()
{
  BASS_Init(-1, 48000, BASS_DEVICE_STEREO, NULL, NULL);
}

void ShutdownAudio()
{
  BASS_Free();
  BASS_PluginFree(0);
}

bool CanLoadAudio()
{
  return HIWORD(BASS_GetVersion()) == BASSVERSION;
}
