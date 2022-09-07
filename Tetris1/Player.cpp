#include "Player.h"
#include "Game.h"

int Player::GetBrickNum()
{
	return _nowBrick;
}

void Player::SetBrickNum(int value)
{
	_nowBrick = value;
}

Type Player::GetBrickType()
{
	return static_cast<Type>(_nowBrick + 1);
}

POINT Player::GetPoint()
{
	return POINT{_x, _y};
}

int Player::GetRot()
{
	return _rot;
}

void Player::HandleInput(WPARAM keyCode)
{
    if (_nowBrick == -1) return;

    POINT p = GetPoint();
    switch (keyCode) {
    case VK_LEFT:
        p.x -= 1;
        if (_game->GetAround(p, _nowBrick, _rot) == Type::EMPTY) {
            _x--;
            _game->UpdateBoard();
        }
        break;
    case VK_RIGHT:
        p.x += 1;
        if (_game->GetAround(p, _nowBrick, _rot) == Type::EMPTY) {
            _x++;
            _game->UpdateBoard();
        }
        break;
    case VK_UP:
    {
        int targetRot = (_rot + 1) % 4;
        if (_game->GetAround(p, _nowBrick, targetRot) == Type::EMPTY) {
            _rot = targetRot;
            _game->UpdateBoard();
        }
    }
        break;
    case VK_DOWN:
        if (MoveDown() == TRUE) {
            _game->MakeNewBrick();
        }
        break;
    case VK_SPACE:
        while (MoveDown() == FALSE) { ; }
        _game->MakeNewBrick();
        break;
    }
}

void Player::SetPos(int x, int y)
{
    _x = x;
    _y = y;
}

void Player::SetRot(int value)
{
    _rot = value;
}

BOOL Player::MoveDown()
{
    POINT p = GetPoint();
    p.y += 1;
    if (_game->GetAround(p, _nowBrick, _rot) != Type::EMPTY)
    {
        _game->TestFull();
        return TRUE;
    }
    _y++;
    _game->UpdateBoard();
    return FALSE;
}
