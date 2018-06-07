#include "stdafx.h"
#include "bass.h"
#include "listplug.h"
#include "shlwapi.h"
#include "audio.h"
#include "tcview.h"
#include "window.h"
#include "config.h"

#ifndef DELAYIMP_INSECURE_WRITABLE_HOOKS
  #define DELAYIMP_INSECURE_WRITABLE_HOOKS
#endif

#include <algorithm>
#include <delayimp.h>
#include <iterator>
#include <sstream>
#include <stdlib.h>
#include <strsafe.h>

// first of all, declare the delay linker entry point
FARPROC WINAPI DelayLoadHook(unsigned dliNotify, PDelayLoadInfo pdli)
{
  std::string path;
  switch (dliNotify)
  {
    case dliStartProcessing:

      // If you want to return control to the delay-load helper, return 0. 
      // Otherwise, return a pointer to a FARPROC helper function that will 
      // be used instead, thereby bypassing the rest of the helper.
      break;

    case dliNotePreLoadLibrary:

      // If you want to return control to the delay-load helper, return 0.
      // Otherwise, return your own HMODULE to be used by the helper 
      // instead of having it call LoadLibrary itself.
    {
      // You can build the DLL path by yourself, and call LoadLibrary 
      // to load the DLL from the path. For simplicity, the sample uses 
      // the dll name to load the DLL, which is the default behavior of 
      // the helper function.
      #ifdef _WIN64
        std::string drive;
        std::string dir;
        std::string file;
        std::string ext;
        drive.resize(MAX_PATH);
        dir.resize(MAX_PATH);
        file.resize(MAX_PATH);
        ext.resize(MAX_PATH);
        _splitpath_s(pdli->szDll, &drive.front(), MAX_PATH, &dir.front(), MAX_PATH, &file.front(), MAX_PATH, &ext.front(), MAX_PATH);
        drive.resize( strlen(drive.data() ));
        dir.resize( strlen(dir.data() ));
        file.resize( strlen( file.data()));
        ext.resize( strlen(ext.data() ));
        path = drive + dir + file + "_x64" + ext;
      #else
        path.assign(pdli->szDll);
      #endif
      HMODULE hLib = LoadLibraryA(path.c_str());
      return reinterpret_cast<FARPROC>(hLib);
    }

    case dliNotePreGetProcAddress:

      // If you want to return control to the delay-load helper, return 0. 
      // If you choose you may supply your own FARPROC function address and 
      // bypass the helper's call to GetProcAddress.
      break;

    case dliFailLoadLib : 

      // LoadLibrary failed.
      // If you don't want to handle this failure yourself, return 0. In 
      // this case the helper will raise an exception (ERROR_MOD_NOT_FOUND) 
      // and exit. If you want to handle the failure by loading an 
      // alternate DLL (for example), then return the HMODULE for the 
      // alternate DLL. The helper will continue execution with this 
      // alternate DLL and attempt to find the requested entrypoint via 
      // GetProcAddress.

      break;

    case dliFailGetProc :

      // GetProcAddress failed.
      // If you don't want to handle this failure yourself, return 0. In 
      // this case the helper will raise an exception (ERROR_PROC_NOT_FOUND) 
      // and exit. If you choose you may handle the failure by returning an 
      // alternate FARPROC function address.

      break;

    case dliNoteEndProcessing : 

      // This notification is called after all processing is done. There is 
      // no opportunity for modifying the helper's behavior at this point 
      // except by longjmp()/throw()/RaiseException. No return value is 
      // processed.

      break;
  }

  return NULL;
}

// At the global level, set the delay-load hooks.
PfnDliHook __pfnDliNotifyHook2 = DelayLoadHook,
           __pfnDliFailureHook2 = DelayLoadHook;

HINSTANCE hinst;

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

std::wstring GetModuleDirectory()
{
  wchar_t currentdir[MAX_PATH];
  GetModuleFileNameW(hinst, currentdir, _countof(currentdir));
  PathRemoveFileSpecW(currentdir);
  return std::wstring{currentdir};
}

std::wstring mbs_to_wcs(std::string mbs)
{
  size_t r;
  std::wstring ws(mbs.size(), L' '); // Overestimate number of code points.
  mbstowcs_s(&r, &ws.front(), mbs.size()+1, mbs.c_str(), mbs.size());
  ws.resize(r);
  return ws;
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
  size_t size;
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
  size = maxlen;
  if(detect.length() < size)
    size = detect.length() + 1;
  StringCchCopyA(detectstring, size, &detect.front() );
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

HWND __stdcall ListLoadW(HWND ParentWin,wchar_t* FileToLoad,int ShowFlags)
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
    sound = new Sound{std::wstring{FileToLoad}};
    sound->set_looping(Configuration::instance()->looping);
    sound->set_volume(Configuration::instance()->volume);
    sound->play();
    WindowSetSound(res.first, sound);
  }
  catch (const std::invalid_argument& e)
  {
    (void)e;
    cnt = WindowDestroy(res.first);
    if(cnt == 0)
      AudioShutdown();
    return NULL;
  }

  WindowShow(res.first);

  return res.first;
}

int __stdcall ListLoadNextW(HWND ParentWin, HWND ListWin, wchar_t *FileToLoad, int ShowFlags)
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
    sound = new Sound{std::wstring{FileToLoad}};
    sound->set_looping(Configuration::instance()->looping);
    sound->set_volume(Configuration::instance()->volume);
    sound->play();
  }
  catch (const std::invalid_argument& e)
  {
    (void)e;
    return LISTPLUGIN_ERROR;
  }
  WindowSetSound(ListWin, sound);
  WindowShow(ListWin);
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