#pragma once
#include"FindPair.h"
#define CS 64
#define random(n) (rand() % n)


class Game
{
public:
	Game(HINSTANCE hInst, HWND hWnd);
	void Update(ULONGLONG tick);
	void HandleInput(shared_ptr<POINT> mouseCoord);
	void InitGame();
	void DrawScreen(HDC hdc);
	shared_ptr<POINT> GetTempFlip();//현재 보여진 카드를 반환
	int GetRemainPair();//아직 못맞춘 갯수를 반환
	void DrawBitmap(HDC hdc, int x, int y, int idx);
	~Game();


private:
	HINSTANCE _hInst;
	HWND _hWnd;
	CardCell _cardCell[4][4];
	int _count;
	HBITMAP _hBitSheet;
	GameStatus _gStatus;
	float _viewTimer;
	float _hintTimer;
	float _timer;
	float _timer1;
};

