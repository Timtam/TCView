#pragma once

#include "audio.h"
#include <windows.h>
#include <utility>

std::pair<HWND, int> WindowCreateChild(HWND parent, HINSTANCE hinst);
int WindowDestroy(HWND win);
void WindowSetSound(HWND win, Audio::Sound *snd);
Audio::Sound *WindowGetSound(HWND win);
void WindowShow(HWND win);
void WindowUpdateText(HWND win);
