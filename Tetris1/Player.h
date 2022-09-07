#pragma once

#include "Tetris1.h"

class Game; //전방선언 - 아직 설명 안함.

class Player
{
public:
	Player(int x, int y, shared_ptr<Game> game) : _x(x), _y(y), _game(game) {}

	int GetBrickNum();
	void SetBrickNum(int value);
	Type GetBrickType();
	POINT GetPoint();
	int GetRot();
	void HandleInput(WPARAM keyCode);
	void SetPos(int x, int y);
	void SetRot(int value);

	BOOL MoveDown();
private:
	int _nowBrick = -1;
	int _x;
	int _y;
	int _rot = 0;
	shared_ptr<Game> _game;
};

