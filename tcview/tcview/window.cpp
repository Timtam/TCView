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

std::wstring format_time(double t)
{
  std::wstring st{0};
  int s = (int)t;
  int mins = s / 60;
  int hours = mins / 60;
  s -= mins * 60;
  st.resize(20);
  swprintf_s(&st.front(), 20, L"%02d:%02d:%02d", hours, mins, s);
  st.resize( wcslen( st.data() ));
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

void skip_start(Sound *snd)
{
  snd->set_position(.0);
}

void skip_end(Sound *snd)
{
  double len = snd->get_length();
  snd->set_position(len);
}

void pause_resume(Sound *snd)
{
  if(snd->is_paused())
    snd->play();
  else
    snd->pause();
}

void volume_up(Sound *snd)
{
  Configuration::instance()->volume = Configuration::instance()->volume + (float)0.05;
  if(Configuration::instance()->volume > 1.0)
    Configuration::instance()->volume = 1.0;
  if(snd == NULL)
    return;
  snd->set_volume(Configuration::instance()->volume);
}

void volume_down(Sound *snd)
{
  Configuration::instance()->volume = Configuration::instance()->volume - (float).05;
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
  std::wstring text{L""};
  sound = WindowGetSound(win);
  if(sound == NULL)
    text += L"(No File)";
  else
  {
    try
    {
      pos = sound->get_position();
      len = sound->get_length();
      text += format_time(pos) + L" / " + format_time(len) + L" ";
      text += sound->get_filename().filename();
    }
    catch (std::runtime_error &e)
    {
      (void)e;
      text += L"(No File)";
    }
  }
  if(Configuration::instance()->looping)
    text += L" (looping)";
  if(Configuration::instance()->continuous)
    text += L" (continuous playback)";
  SetWindowTextW(win, text.c_str());
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
    SetWindowLongPtrW(win, GWLP_WNDPROC, DefWinProc);
    DefWinProc = NULL;
    WindowSetSound(win, (Sound*)NULL);
    delete sound;
    PostMessageW(GetParent(win), WM_COMMAND, MAKELONG(NULL, itm_next), (LPARAM)win);
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
      case VK_HOME:
        skip_start(WindowGetSound(hwnd));
        return 0;
      case VK_END:
        skip_end(WindowGetSound(hwnd));
        return 0;
      case VK_SPACE:
        pause_resume(WindowGetSound(hwnd));
        return 0;
    }
  }
  if(DefWinProc != NULL)
    return CallWindowProcW((WNDPROC)DefWinProc, hwnd, msg, wParam, lParam);
  return 0;
}

void CALLBACK ReplaceWindowProcTimer(HWND hwnd, UINT msg, UINT_PTR idEvent, DWORD dwTime)
{
  LONG_PTR winproc = GetWindowLongPtrW(hwnd, GWLP_WNDPROC);
  if(winproc != DefWinProc)
  {
    DefWinProc = winproc;
    SetWindowLongPtrW(hwnd, GWLP_WNDPROC, (LONG_PTR)WinProc);
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

  hwnd=CreateWindowW(L"EDIT",0,WS_CHILD | WS_VISIBLE | ES_READONLY,
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
  SetWindowLongPtrW(win, GWLP_USERDATA, (LONG_PTR)snd);
}

Sound *WindowGetSound(HWND win)
{
  return (Sound*)GetWindowLongPtrW(win, GWLP_USERDATA);
}

void WindowShow(HWND win)
{
  ShowWindow(win, SW_SHOW);
  DefWinProc = GetWindowLongPtrW(win, GWLP_WNDPROC);
  SetTimer(win, IDT_TIMER1, 50, (TIMERPROC)ReplaceWindowProcTimer);
  SetTimer(win, IDT_TIMER2, 20, (TIMERPROC)UpdateWindowTimer);
}
