#include "stdafx.h"
#include "bass.h"
#include "listplug.h"

#define DEBUG(text) MessageBox(NULL, text, L"Debug", MB_OK)

HINSTANCE hinst;

BOOL InitializeBass()
{
  BOOL success;
  success = BASS_Init(-1, 48000, BASS_DEVICE_STEREO, NULL, NULL);
  if(success == TRUE)
  {
  }
  return success;
}

HSTREAM CreateSound(char *filename)
{
  HSTREAM handle;
  handle = BASS_StreamCreateFile(FALSE, filename, 0, 0, BASS_STREAM_AUTOFREE);
  if(handle == 0)
  {
    DEBUG(L"error creating sound");
    return 0;
  }
  BASS_ChannelPlay(handle, TRUE);
  return handle;
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
           )
{
  BOOL success;
  switch (ul_reason_for_call)
  {
  case DLL_PROCESS_ATTACH:
    success = InitializeBass();
    if(!success)
      return FALSE;
    hinst=(HINSTANCE)hModule;
    break;
  case DLL_PROCESS_DETACH:
    BASS_Free();
    break;
  case DLL_THREAD_ATTACH:
  case DLL_THREAD_DETACH:
    break;
  }
  return TRUE;
}

char* strlcpy(char* p,char*p2,int maxlen)
{
  if ((int)strlen(p2)>=maxlen) {
    strncpy(p,p2,maxlen);
    p[maxlen]=0;
  } else
    strcpy(p,p2);
  return p;
}

void __stdcall ListGetDetectString(char *detectstring, int maxlen)
{
  strlcpy(detectstring, "MULTIMEDIA & (EXT=\"WAV\" | EXT=\"MP3\")", maxlen);
}

void __stdcall ListCloseWindow(HWND ListWin)
{
  HSTREAM handle;
  handle = (HSTREAM)GetWindowLongPtr(ListWin, GWLP_USERDATA);
  if(handle != 0)
    BASS_ChannelStop(handle);
  DestroyWindow(ListWin);
}

HWND __stdcall ListLoad(HWND ParentWin,char* FileToLoad,int ShowFlags)
{
  HSTREAM handle;
  HWND hwnd;
  RECT r;

  GetClientRect(ParentWin,&r);

  hwnd=CreateWindow(L"STATIC",0,WS_CHILD,
    r.left,r.top,r.right-r.left,
    r.bottom-r.top,ParentWin,NULL,hinst,NULL);
  if(!hwnd)
    return NULL;

  handle = CreateSound(FileToLoad);
  if(handle == 0)
    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)0);
  SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)handle);
  return hwnd;
}
