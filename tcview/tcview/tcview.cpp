#include "stdafx.h"
#include "listplug.h"

HINSTANCE hinst;

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
           )
{
  switch (ul_reason_for_call)
  {
  case DLL_PROCESS_ATTACH:
    hinst=(HINSTANCE)hModule;
    break;
  case DLL_THREAD_ATTACH:
  case DLL_THREAD_DETACH:
  case DLL_PROCESS_DETACH:
    break;
  }
  return TRUE;
}

void __stdcall ListCloseWindow(HWND ListWin)
{
  DestroyWindow(ListWin);
}

HWND __stdcall ListLoad(HWND ParentWin,char* FileToLoad,int ShowFlags)
{
  HWND hwnd;
  RECT r;

  GetClientRect(ParentWin,&r);

  hwnd=CreateWindow(L"RichEdit20A",L"",WS_CHILD | ES_READONLY,
    r.left,r.top,r.right-r.left,
    r.bottom-r.top,ParentWin,NULL,hinst,NULL);
  if(!hwnd)
    return NULL;
  return hwnd;
}
