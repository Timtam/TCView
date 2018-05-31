#include "stdafx.h"
#include "bass.h"
#include "listplug.h"
#include "shlwapi.h"
#include "audio.h"
#include "tcview.h"
#include "window.h"
#include "config.h"
#include <stdlib.h>

#include <algorithm>
#include <iterator>
#include <sstream>

HINSTANCE hinst;

// configuration
BOOL Looping = FALSE;

template<typename Out>
void string_split(const char *s, char delim, Out result) {
  std::string os{s};
  std::stringstream ss(os);
  std::string item;
  while (std::getline(ss, item, delim)) {
    *(result++) = item;
  }
}

std::vector<std::string> string_split(const char *s, char delim) {
  std::vector<std::string> elems;
  string_split(s, delim, std::back_inserter(elems));
  return elems;
}

/*
void SwitchLooping()
{
  Looping = !Looping;
  if(Looping)
    BASS_ChannelFlags(CurrentSound, BASS_SAMPLE_LOOP, BASS_SAMPLE_LOOP);
  else
    BASS_ChannelFlags(CurrentSound, 0, BASS_SAMPLE_LOOP);
}
*/

std::string GetModuleDirectory()
{
  char currentdir[MAX_PATH];
  GetModuleFileNameA(hinst, currentdir, _countof(currentdir));
  PathRemoveFileSpecA(currentdir);
  return std::string{currentdir};
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
           )
{
  switch (ul_reason_for_call)
  {
  case DLL_PROCESS_ATTACH:
    if(AudioCanLoad() == false)
      return FALSE;
    hinst=(HINSTANCE)hModule;
    AudioLoadPlugins();
    break;
  case DLL_PROCESS_DETACH:
    AudioUnloadPlugins();
    break;
  case DLL_THREAD_ATTACH:
  case DLL_THREAD_DETACH:
    break;
  }
  return TRUE;
}

void __stdcall ListGetDetectString(char *detectstring, int maxlen)
{
  std::vector<std::string> exts = AudioGetExtensions();
  std::string detect{"MULTIMEDIA & ("};
  unsigned int size;
  unsigned int i;
  if(exts.size() == 0)
    return;
  for(i = 0; i < exts.size(); i++)
  {
    std::transform(exts[i].begin(), exts[i].end(),exts[i].begin(), ::toupper);
    detect += "EXT=\"" + exts[i] + "\" | ";
  }
  detect.erase(detect.length()-3, detect.length());
  detect += ")";
  if(detect.size() > maxlen)
    return;
  size = maxlen - 1;
  if(detect.length() < size)
    size = detect.length();
  detect.copy(detectstring, size);
  detectstring[size + 1] = '\0';
}

void __stdcall ListCloseWindow(HWND ListWin)
{
  int cnt;
  Sound *sound = WindowGetSound(ListWin);
  if(sound != NULL)
  {
    sound->stop();
    delete sound;
    WindowSetSound(ListWin, (Sound*)NULL);
  }
  cnt = WindowDestroy(ListWin);
  if(cnt == 0)
    AudioShutdown();
}

HWND __stdcall ListLoad(HWND ParentWin,char* FileToLoad,int ShowFlags)
{
  int cnt;
  std::pair<HWND, int> res;
  Sound *sound;

  res = WindowCreateChild(ParentWin, hinst);

  if(res.first == NULL)
  {
    return NULL;
  }

  if(res.second == 1)
    AudioInitialize();

  try
  {
    sound = new Sound{std::string{FileToLoad}};
    sound->set_looping(Configuration::instance()->looping);
    sound->play();
    WindowSetSound(res.first, sound);
  }
  catch (const std::invalid_argument& e)
  {
    cnt = WindowDestroy(res.first);
    if(cnt == 0)
      AudioShutdown();
    return NULL;
  }

  WindowShow(res.first);

  return res.first;
}

int __stdcall ListLoadNext(HWND ParentWin, HWND ListWin, char *FileToLoad, int ShowFlags)
{
  Sound *sound;
  sound = WindowGetSound(ListWin);
  if(sound != NULL)
  {
    sound->fade_out();
    delete sound;
  }
  WindowSetSound(ListWin, (Sound*)NULL);
  try
  {
    sound = new Sound{std::string{FileToLoad}};
    sound->set_looping(Configuration::instance()->looping);
    sound->play();
  }
  catch (const std::invalid_argument& e)
  {
    return LISTPLUGIN_ERROR;
  }
  WindowSetSound(ListWin, sound);
  return LISTPLUGIN_OK;
}

void __stdcall ListSetDefaultParams(ListDefaultParamStruct *dps)
{
  std::string ini{dps->DefaultIniName};
  PathRemoveFileSpecA(&ini.front() );
  ini.resize( strlen(ini.data() ));
  ini += "\\tcview.ini";
  ini.shrink_to_fit();
  Configuration::instance()->set_file(ini);
}