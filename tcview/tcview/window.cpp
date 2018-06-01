#include "stdafx.h"
#include "audio.h"
#include "config.h"
#include "window.h"

#define IDT_TIMER1 100

LONG_PTR DefWinProc;
unsigned int WindowCount = 0;

void toggle_looping(HWND win)
{
  Sound *sound;
  Configuration::instance()->looping = !Configuration::instance()->looping;
  sound = WindowGetSound(win);
  if(sound != NULL)
    sound->set_looping(Configuration::instance()->looping);
}

LRESULT CALLBACK WinProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  if(msg == WM_KEYDOWN)
  {
    switch(wParam)
    {
      case 'L':
        toggle_looping(hwnd);
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

int WindowDestroy(HWND win)
{
  WindowCount--;
  DestroyWindow(win);
  return WindowCount;
}

std::pair<HWND, int> WindowCreateChild(HWND parent, HINSTANCE hinst)
{
  HWND hwnd;
  RECT r;

  GetClientRect(parent,&r);

  hwnd=CreateWindowA("EDIT",0,WS_CHILD | WS_VISIBLE | ES_READONLY,
    r.left,r.top,r.right-r.left,
    r.bottom-r.top,parent,NULL,hinst,NULL);
  if(!hwnd)
    return std::make_pair((HWND)NULL, -1);

  WindowCount++;

  WindowSetSound(hwnd, (Sound*)NULL);

  return std::make_pair(hwnd, WindowCount);
}

void WindowSetSound(HWND win, Sound *snd)
{
  SetWindowLongPtr(win, GWLP_USERDATA, (LONG_PTR)snd);
}

Sound *WindowGetSound(HWND win)
{
  return (Sound*)GetWindowLongPtr(win, GWLP_USERDATA);
}

void WindowShow(HWND win)
{
  ShowWindow(win, SW_SHOW);
  DefWinProc = GetWindowLongPtr(win, GWLP_WNDPROC);
  SetTimer(win, IDT_TIMER1, 50, (TIMERPROC)ReplaceWindowProcTimer);
}
