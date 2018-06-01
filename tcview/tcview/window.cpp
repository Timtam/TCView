#include "stdafx.h"
#include "audio.h"
#include "config.h"
#include "listplug.h"
#include "window.h"

#include <stdlib.h>
#include <time.h>
#include <windef.h>

#define IDT_TIMER1 100
#define IDT_TIMER2 200

LONG_PTR DefWinProc;
unsigned int WindowCount = 0;
time_t TextUpdateTime = 0;

// helpers

std::string format_time(double t)
{
  std::string st{""};
  int s = (int)t;
  int mins = s / 60;
  int hours = mins / 60;
  s -= mins * 60;
  st.resize(20);
  sprintf_s(&st.front(), 20, "%02d:%02d:%02d", hours, mins, s);
  st.resize( strlen( st.data() ));
  st.shrink_to_fit();
  return st;
}

void toggle_looping(Sound *snd)
{
  Configuration::instance()->looping = !Configuration::instance()->looping;
  if(snd == NULL)
    return;
  snd->set_looping(Configuration::instance()->looping);
}

void skip_forward(Sound *snd)
{
  double t;
  if(snd == NULL)
    return;
  t = snd->get_position();
  t += 1.0;
  if(t > snd->get_length())
    t = snd->get_length();
  snd->set_position(t);
}

void skip_backward(Sound *snd)
{
  double t;
  if(snd == NULL)
    return;
  t = snd->get_position();
  t -= 1.0;
  if(t < 0)
    t = .0;
  snd->set_position(t);
}

void volume_up(Sound *snd)
{
  Configuration::instance()->volume = Configuration::instance()->volume + 0.05;
  if(Configuration::instance()->volume > 1.0)
    Configuration::instance()->volume = 1.0;
  if(snd == NULL)
    return;
  snd->set_volume(Configuration::instance()->volume);
}

void volume_down(Sound *snd)
{
  Configuration::instance()->volume = Configuration::instance()->volume - .05;
  if(Configuration::instance()->volume < 0)
    Configuration::instance()->volume = .0;
  if(snd == NULL)
    return;
  snd->set_volume(Configuration::instance()->volume);
}

void WindowUpdateText(HWND win)
{
  double pos, len;
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
      pos = sound->get_position();
      len = sound->get_length();
      text += format_time(pos) + " / " + format_time(len) + " ";
      text += file + ext;
    }
    catch (std::runtime_error &e)
    {
      text += "(No File)";
    }
  }
  if(Configuration::instance()->looping)
    text += " (looping)";
  if(Configuration::instance()->continuous)
    text += " (continuous playback)";
  SetWindowTextA(win, text.c_str());
}

void CALLBACK UpdateWindowTimer(HWND win, UINT msg, UINT_PTR idEvent, DWORD dwTime)
{
  Sound *sound;
  if(time(NULL) - TextUpdateTime >= 1)
  {
    WindowUpdateText(win);
    TextUpdateTime = time(NULL);
  }
  sound = WindowGetSound(win);
  if(sound != NULL && sound->is_stopped() && Configuration::instance()->continuous)
  {
    SetWindowLongPtr(win, GWLP_WNDPROC, DefWinProc);
    DefWinProc = NULL;
    WindowSetSound(win, (Sound*)NULL);
    delete sound;
    PostMessage(GetParent(win), WM_COMMAND, MAKELONG(NULL, itm_next), (LPARAM)win);
  }
}

LRESULT CALLBACK WinProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  if(msg == WM_KEYDOWN)
  {
    switch(wParam)
    {
      case 'L':
        toggle_looping(WindowGetSound(hwnd));
        return 0;
      case 'C':
        Configuration::instance()->continuous = !Configuration::instance()->continuous;
        return 0;
      case VK_LEFT:
        skip_backward(WindowGetSound(hwnd));
        return 0;
      case VK_RIGHT:
        skip_forward(WindowGetSound(hwnd));
        return 0;
      case VK_UP:
        volume_up(WindowGetSound(hwnd));
        return 0;
      case VK_DOWN:
        volume_down(WindowGetSound(hwnd));
        return 0;
    }
  }
  if(DefWinProc != NULL)
    return CallWindowProc((WNDPROC)DefWinProc, hwnd, msg, wParam, lParam);
  return 0;
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
  SetTimer(win, IDT_TIMER2, 20, (TIMERPROC)UpdateWindowTimer);
}
