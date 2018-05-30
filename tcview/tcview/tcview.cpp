#include "stdafx.h"
#include "bass.h"
#include "listplug.h"
#include "shlwapi.h"
#include "audio.h"
#include "tcview.h"

#include <algorithm>
#include <iterator>
#include <sstream>

#define IDT_TIMER1 100

HINSTANCE hinst;
HWND Parent;
LONG_PTR DefWinProc;
unsigned int WindowCount = 0;

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

LRESULT CALLBACK WinProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  if(msg == WM_KEYDOWN)
  {
    switch(wParam)
    {
      case 'L':
        //SwitchLooping();
        return 0;
    }
  }
  return CallWindowProc((WNDPROC)DefWinProc, hwnd, msg, wParam, lParam);
}

void CALLBACK ReplaceWindowProcTimer(HWND hwnd, UINT msg, UINT_PTR idEvent, DWORD dwTime)
{
  LONG_PTR winproc = GetWindowLongPtr(hwnd, GWLP_WNDPROC);
  if(winproc != DefWinProc)
  {
    DefWinProc = winproc;
    SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)WinProc);
    KillTimer(hwnd, idEvent);
  }
}

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
  Sound *sound;
  sound = (Sound*)GetWindowLongPtr(ListWin, GWLP_USERDATA);
  if(sound != NULL)
  {
    sound->stop();
    delete sound;
  }
  WindowCount--;
  if(WindowCount == 0)
    AudioShutdown();
  DestroyWindow(ListWin);
}

HWND __stdcall ListLoad(HWND ParentWin,char* FileToLoad,int ShowFlags)
{
  HWND hwnd;
  RECT r;
  Sound *sound;

  GetClientRect(ParentWin,&r);

  hwnd=CreateWindow(L"EDIT",0,WS_CHILD | WS_VISIBLE | ES_READONLY,
    r.left,r.top,r.right-r.left,
    r.bottom-r.top,ParentWin,NULL,hinst,NULL);
  if(!hwnd)
    return NULL;

  WindowCount++;

  if(WindowCount == 1)
    AudioInitialize();

  try
  {
    sound = new Sound{std::string{FileToLoad}};
    sound->play();
  }
  catch (const std::invalid_argument& e)
  {
    DestroyWindow(hwnd);
    WindowCount--;
    if(WindowCount == 0)
      AudioShutdown();
    return NULL;
  }
  SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)sound);
  Parent = ParentWin;
  ShowWindow(hwnd, SW_SHOW);
  SetFocus(hwnd);
  UpdateWindow(hwnd);
  DefWinProc = GetWindowLongPtr(hwnd, GWLP_WNDPROC);
  SetTimer(hwnd, IDT_TIMER1, 50, (TIMERPROC)ReplaceWindowProcTimer);
  return hwnd;
}

int __stdcall ListLoadNext(HWND ParentWin, HWND ListWin, char *FileToLoad, int ShowFlags)
{
  Sound *sound;
  sound = (Sound*)GetWindowLongPtr(ListWin, GWLP_USERDATA);
  if(sound != NULL)
  {
    sound->fade_out();
    delete sound;
  }
  SetWindowLongPtr(ListWin, GWLP_USERDATA, (LONG_PTR)NULL);
  try
  {
    sound = new Sound{std::string{FileToLoad}};
    sound->play();
  }
  catch (const std::invalid_argument& e)
  {
    return LISTPLUGIN_ERROR;
  }
  SetWindowLongPtr(ListWin, GWLP_USERDATA, (LONG_PTR)sound);
  return LISTPLUGIN_OK;
}
