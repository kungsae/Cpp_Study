#pragma once

#include "framework.h"
#include "resource.h"
#include <memory>
using namespace std;

enum class Status
{
	HIDDEN = 1,	//카드가 가려져있을때
	FLIP = 2,	//카드가 보여지고 있을때
	TEMPFLIP =3, //카드가 임시로 보여지고 있을때
};

enum class GameStatus
{
	PAUSED = 0,
	RUN = 1,
	HINT = 2,
	VIEW = 3
};

struct CardCell
{
	int num;//몇번째 카드인지
	Status _status;//카드 현재 상태	

};