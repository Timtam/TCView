#include "stdafx.h"
#include "audio.h"

Sound::Sound()
{
}

void Sound::load(std::string filename)
{
  this->sound = BASS_StreamCreateFile(false, filename.c_str(), 0, 0, BASS_STREAM_AUTOFREE);
  if(this->sound != 0)
    this->play();
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
