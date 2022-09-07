#pragma once
#include <Windows.h>
#include <conio.h>

extern void gotoXY(int x, int y);
extern void setTextColor(int color);
extern void clrscr();
extern void beep(int tone, int delay);
extern int getKeyDown();
extern void setNoCursor();