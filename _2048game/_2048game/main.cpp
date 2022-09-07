#include <iostream>
#include <windows.h>
#include <conio.h>
#include <time.h>

#include "Console.h"

#define MAP_X	4
#define MAP_Y	2

#define KEY_DIRECTION	224
#define KEY_LEFT		75
#define KEY_RIGHT		77
#define KEY_UP			72
#define KEY_DOWN		80

int board[4][4] = {
	{0, 0, 0, 0},
	{0, 0, 0, 0},
	{0, 0, 0, 0},
	{0, 0, 0, 0}
};

int score;
bool bChangeNumber;

using namespace std;

void setConsoleView();

void initData();

void initDraw();
void drawGame();

void createNumber();
bool getKey();
bool moveNumber();

void checkGameover();

int main()
{
	srand((unsigned)time(NULL));

	setNoCursor();
	setConsoleView();

	initData();
	initDraw();

	createNumber();
	createNumber();

	while (true)
	{
		if (getKey())
		{
			createNumber();
		}
		checkGameover();
	}

	return 0;
}

void setConsoleView()
{
	system("mode con:cols=32 lines=26");
	system("title 2048 GAME");
}

void initData()
{
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			board[i][j] = 0;
		}
	}

	score = 0;
	bChangeNumber = false;
}

void initDraw()
{
	gotoXY(MAP_X - 2, MAP_Y - 1);
	cout << "|----------------------|"; // ¦¡ 21°³

	for (int i = 0; i < 12; i++)
	{
		gotoXY(MAP_X - 2, MAP_Y + i);
		cout << "|                      |";
	}

	gotoXY(MAP_X - 2, MAP_Y + 12);
	cout << "|----------------------|";

	gotoXY(MAP_X + 2, MAP_Y + 14);
	cout << "¡æ, ¡ç, ¡è, ¡é : ÀÌµ¿";
}

void drawGame()
{
	int num;


	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			if (board[i][j] == 0)
			{
				setTextColor(8);
				gotoXY(MAP_X + j * 5, MAP_Y + i * 3);
				cout << "     ";
				gotoXY(MAP_X + j * 5, MAP_Y + i * 3 + 1);
				cout << "  +  ";
				gotoXY(MAP_X + j * 5, MAP_Y + i * 3 + 2);
				cout << "     ";
			}
			else
			{
				num = board[i][j] > 10000 ? board[i][j] - 10000 : board[i][j];
				int sum = 1;

				for (int k = 0; k < 12; k++)
				{
					sum *= 2;

					if (sum == board[i][j])
					{
						setTextColor(k + 1);
						break;
					}
				}

				gotoXY(MAP_X + j * 5, MAP_Y + i * 3);
				cout << "|---|";
				gotoXY(MAP_X + j * 5, MAP_Y + i * 3 + 1);
				printf("%4d ", num);
				gotoXY(MAP_X + j * 5, MAP_Y + i * 3 + 2);
				cout << "|---|";
			}
		}
	}

	Sleep(100);
}

void createNumber()
{
	int* empty[16] = { 0, };
	int count = 0;

	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			if (board[i][j] == 0)
			{
				empty[count] = &board[i][j];
				count++;
			}
		}
	}

	int random = rand() % count;
	int number = rand() % 100 < 20 ? 4 : 2;

	*empty[random] = number;
	drawGame();
	*empty[random] = 0;
	drawGame();
	*empty[random] = number;
	drawGame();
}

bool moveNumber(int sourceX, int sourceY, int targetX, int targetY)
{
	if (board[sourceY][sourceX] == 0 || board[sourceY][sourceX] > 10000) return false;
	if (board[targetY][targetX] != 0 && board[targetY][targetX] != board[sourceY][sourceX]) return false;

	if (board[targetY][targetX] == 0)
	{
		board[targetY][targetX] = board[sourceY][sourceX];
	}
	else if (board[targetY][targetX] == board[sourceY][sourceX])
	{
		board[targetY][targetX] *= 2;
		board[targetY][targetX] += 10000;

		bChangeNumber = true;
	}

	board[sourceY][sourceX] = 0;

	return true;
}

bool getKey()
{
	bool bMove = false;
	bChangeNumber = false;

	int key = _getch();

	if (key == KEY_DIRECTION)
	{
		key = _getch();

		if (key == KEY_LEFT) {
			do {
				bMove = false;

				for (int i = 0; i < 4; i++) {
					for (int j = 1; j < 4; j++) {
						if (moveNumber(j, i, j - 1, i))
						{
							bMove = true;
							bChangeNumber = true;
						}
					}
				}
				if (bMove) drawGame();
			} while (bMove);
		}
		else if (key == KEY_RIGHT) {
			do {
				bMove = false;

				for (int i = 0; i < 4; i++) {
					for (int j = 2; j >= 0; j--) {
						if (moveNumber(j, i, j + 1, i))
						{
							bMove = true;
							bChangeNumber = true;
						}
					}
				}
				if (bMove) drawGame();
			} while (bMove);
		}
		else if (key == KEY_UP) {
			do {
				bMove = false;

				for (int i = 1; i < 4; i++) {
					for (int j = 0; j < 4; j++) {
						if (moveNumber(j, i, j, i - 1))
						{
							bMove = true;
							bChangeNumber = true;
						}
					}
				}
				if (bMove) drawGame();
			} while (bMove);
		}
		else if (key == KEY_DOWN) {
			do {
				bMove = false;

				for (int i = 2; i >= 0; i--) {
					for (int j = 0; j < 4; j++) {
						if (moveNumber(j, i, j, i +1))
						{
							bMove = true;
							bChangeNumber = true;
						}
					}
				}
				if (bMove) drawGame();
			} while (bMove);
		}
	}

	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			if (board[i][j] > 10000)
			{
				board[i][j] -= 10000;
			}
		}
	}

	return bChangeNumber;
}
void checkGameover()
{
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			if (board[i][j] == 0)return;
		}
	}

	for (int i = 0; i < 3; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			if (board[i][j] == board[i][j + 1] || board[i][j] == board[i + 1][j])return;
		}
	}
	for (int i = 0; i < 3; i++)
	{
		if (board[i][3] == board[i + 1][3])return;
	}
	for (int j = 0; j < 3; j++)
	{
		if (board[3][j] == board[3][j+1])return;
	}
	gotoXY(MAP_X + 6, MAP_Y + 21);
	cout << "GAME OVER";
}
