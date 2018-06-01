#include "stdafx.h"
#include "audio.h"
#include "config.h"
#include "window.h"

#include <stdlib.h>

#define IDT_TIMER1 100
#define IDT_TIMER2 200

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

void WindowUpdateText(HWND win)
{
  Sound *sound;
  std::string text{""};
  std::string file,ext;
  file.resize(MAX_PATH);
  ext.resize(MAX_PATH);
  sound = WindowGetSound(win);
  if(sound == NULL)
    text += "(No File)";
  else
  {
    try
    {
      _splitpath_s(sound->get_filename().c_str(), NULL, 0, NULL, 0, &file.front(), MAX_PATH, &ext.front(), MAX_PATH);
      file.resize( strlen(file.data()));
      file.shrink_to_fit();
      ext.resize(strlen(ext.data()));
      ext.shrink_to_fit();
      text += file + ext;
    }
    catch (std::runtime_error &e)
    {
      text += "(No File)";
    }
  }
  if(Configuration::instance()->looping)
    text += " (looping)";
  SetWindowTextA(win, text.c_str());
}

void CALLBACK UpdateWindowTextTimer(HWND win, UINT msg, UINT_PTR idEvent, DWORD dwTime)
{
  WindowUpdateText(win);
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
  SetTimer(win, IDT_TIMER2, 1000, (TIMERPROC)UpdateWindowTextTimer);
  WindowUpdateText(win);
}
