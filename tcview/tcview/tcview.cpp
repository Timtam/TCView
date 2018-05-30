#include "stdafx.h"
#include "bass.h"
#include "listplug.h"
#include "shlwapi.h"
#include "audio.h"

#define DEBUG(text) MessageBox(NULL, text, L"Debug", MB_OK)
#define IDT_TIMER1 100

HINSTANCE hinst;
HWND Parent;
char Extensions[MAX_PATH] = {"EXT=\"MP3\" | EXT=\"WAV\" | EXT=\"OGG\""};
LONG_PTR DefWinProc;

// configuration
BOOL Looping = FALSE;

template<typename Out>
void split(const char *s, char delim, Out result) {
  std::string os;
  os.assign(s, s+strlen(s)+1);
  std::stringstream ss(os);
  std::string item;
  while (std::getline(ss, item, delim)) {
    *(result++) = item;
  }
}

std::vector<std::string> split(const char *s, char delim) {
  std::vector<std::string> elems;
  split(s, delim, std::back_inserter(elems));
  return elems;
}

void ParseExtensions(const char *exts)
{
  std::vector<std::string> lexts = split(exts, ';');
  unsigned int i;
  char tmp[MAX_PATH];
  for(i=0;i<lexts.size();i++)
  {
    std::transform(lexts[i].begin(), lexts[i].end(),lexts[i].begin(), ::toupper);
    sprintf(tmp, "%s | EXT=\"%s\"", Extensions, lexts[i].c_str()+2);
    sprintf(Extensions, "%s", tmp);
  }
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

void LoadPlugins()
{
  BASS_PLUGININFO *hPluginInfo;
  char pluginfile[MAX_PATH] = {0};
  HPLUGIN hPlugin;
  HANDLE hFind = INVALID_HANDLE_VALUE;
  unsigned int i;
  wchar_t currentdir[MAX_PATH] = {0};
  wchar_t fullpath[MAX_PATH] = {0};
  wchar_t searchpattern[MAX_PATH] = {0};
  WIN32_FIND_DATA ffd;
  // we'll get the dll directory first
  GetModuleFileNameW(hinst, currentdir, _countof(currentdir));
  PathRemoveFileSpec(currentdir);
  // append the wildcards
  swprintf(searchpattern, MAX_PATH, L"%s\\plugins\\*.dll", currentdir);

  // searching all plugin files
  hFind = FindFirstFile(searchpattern, &ffd);
  if(hFind == INVALID_HANDLE_VALUE)
    return;

  do
  {
    if(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
      continue;
    else
    {
      swprintf(fullpath, MAX_PATH, L"%s\\plugins\\%s", currentdir, ffd.cFileName);
      wcstombs(pluginfile, fullpath, MAX_PATH);
      hPlugin = BASS_PluginLoad(pluginfile, 0);
      if(!hPlugin)
        continue;
      hPluginInfo = (BASS_PLUGININFO*)BASS_PluginGetInfo(hPlugin);
      for(i=0;i<hPluginInfo->formatc;i++)
        ParseExtensions(hPluginInfo->formats[i].exts);
    }
  }
  while(FindNextFile(hFind, &ffd) != 0);
}

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

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
           )
{
  switch (ul_reason_for_call)
  {
  case DLL_PROCESS_ATTACH:
    if(HIWORD(BASS_GetVersion()) != BASSVERSION)
      return FALSE;
    hinst=(HINSTANCE)hModule;
    LoadPlugins();
    break;
  case DLL_PROCESS_DETACH:
    BASS_PluginFree(0);
    break;
  case DLL_THREAD_ATTACH:
  case DLL_THREAD_DETACH:
    break;
  }
  return TRUE;
}

void __stdcall ListGetDetectString(char *detectstring, int maxlen)
{
  sprintf(detectstring, "MULTIMEDIA & (%s)", Extensions);
}

void __stdcall ListCloseWindow(HWND ListWin)
{
  Sound *sound;
  sound = (Sound*)GetWindowLongPtr(ListWin, GWLP_USERDATA);
  if(sound != 0)
    sound->stop();
  BASS_Free();
  DestroyWindow(ListWin);
}

HWND __stdcall ListLoad(HWND ParentWin,char* FileToLoad,int ShowFlags)
{
  HWND hwnd;
  RECT r;
  Sound sound;

  GetClientRect(ParentWin,&r);

  hwnd=CreateWindow(L"EDIT",0,WS_CHILD | WS_VISIBLE | ES_READONLY,
    r.left,r.top,r.right-r.left,
    r.bottom-r.top,ParentWin,NULL,hinst,NULL);
  if(!hwnd)
    return NULL;

  InitializeAudio();

  sound.load(std::string(FileToLoad));
  // TODO: exception on sound loading error
  /*
  if(handle == 0)
  {
    BASS_Free();
    DestroyWindow(hwnd);
    return NULL;
  }
  */
  SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)&sound);
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
  Sound *oldsnd;
  Sound newsnd;
  oldsnd = (Sound*)GetWindowLongPtr(ListWin, GWLP_USERDATA);
  if(oldsnd != NULL)
    oldsnd->fade_out();
  SetWindowLongPtr(ListWin, GWLP_USERDATA, (LONG_PTR)NULL);
  newsnd.load(std::string(FileToLoad));
  // TODO: exception on sound loading error
  /*
  if(handle == 0)
    return LISTPLUGIN_ERROR;
  */
  SetWindowLongPtr(ListWin, GWLP_USERDATA, (LONG_PTR)&newsnd);
  return LISTPLUGIN_OK;
}
