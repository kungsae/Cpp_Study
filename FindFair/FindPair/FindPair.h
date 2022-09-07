#pragma once

#include "framework.h"
#include "resource.h"
#include <memory>
using namespace std;

enum class Status
{
	HIDDEN = 1,	//ī�尡 ������������
	FLIP = 2,	//ī�尡 �������� ������
	TEMPFLIP =3, //ī�尡 �ӽ÷� �������� ������
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
	int num;//���° ī������
	Status _status;//ī�� ���� ����	

};