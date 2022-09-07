#include "Game.h"

Game::Game(HWND main, HINSTANCE hInst)
{
	hWndMain = main;
	hBitSheet = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_SHEET));
    clear = LoadBitmap(hInst, MAKEINTRESOURCE(137));
	nStage = 0;
	undoList = new vector<MoveInfo>();
	redoList = new vector<MoveInfo>();
}

Game::~Game() {
	DeleteObject(hBitSheet);
	delete undoList;
}

void Game::DrawScreen(HDC hdc, PAINTSTRUCT& ps)
{
	int iBit = 3;    
    TCHAR userMsg[256];
    //EMPTY, MAN, PACK, TARGET, WALL
    for (int y = 0; y < 18; y++)
    {
        for (int x = 0; x < 20; x++) {
            switch (ns[y][x])
            {
            case '#':
                iBit = 0;
                break;
            case '.':
                iBit = 2;
                break;
            case 'O':
                iBit = 1;
                break;
            case ' ':
                iBit = 7;
                break;
            }

            DrawBitmap(hdc, iBit, x * BW, y * BH, hBitSheet);
        }
    }
    DrawBitmap(hdc, imageDir, nx * BW, ny * BH, hBitSheet);


    wsprintf(userMsg, _T("SOKOBAN !"));
    TextOut(hdc, 700, 20, userMsg, lstrlen(userMsg));
    wsprintf(userMsg, _T("Q : 종료, R : 다시 시작"));
    TextOut(hdc, 700, 40, userMsg, lstrlen(userMsg));
    wsprintf(userMsg, _T("N : 다음, P : 이전"));
    TextOut(hdc, 700, 60, userMsg, lstrlen(userMsg));
    wsprintf(userMsg, _T("스테이지 : %d"), nStage + 1);
    TextOut(hdc, 700, 80, userMsg, lstrlen(userMsg));
    wsprintf(userMsg, _T("이동 횟수 : %d"), nMove);
    TextOut(hdc, 700, 100, userMsg, lstrlen(userMsg));
    wsprintf(userMsg, _T("Z : undo, X : redo"));
    TextOut(hdc, 700, 120, userMsg, lstrlen(userMsg));
}

void Game::DrawClear(HDC hdc)
{
    HDC hMemDC;
    HBITMAP hOldBitMap;
    int bx, by;
    BITMAP bit;

    hMemDC = CreateCompatibleDC(hdc); //메모리 DC생성
    hOldBitMap = static_cast<HBITMAP>(SelectObject(hMemDC, clear));

    GetObject(clear, sizeof(BITMAP), &bit);

    bx = bit.bmWidth;
    by = bit.bmHeight;

    StretchBlt(hdc, 0, 0, bx, by, hMemDC, 0,0,bx,by, SRCCOPY);

    //BitBlt(hdc, x, y, bx, by, hMemDC, 0, 0, SRCCOPY);
    SelectObject(hMemDC, hOldBitMap);
    DeleteDC(hMemDC);
}

BOOL Game::TestEnd()
{
    for (int y = 0; y < 18; y++)
    {
        for (int x = 0; x < 20; x++)
        {
            if (arStage[nStage][y][x] == '.' && ns[y][x] != 'O') {
                return FALSE;
            }
        }
    }
    return TRUE;
}

void Game::Move(int dir)
{
    int dx = 0, dy = 0; //델타X, 델타Y
    BOOL bWithPack = FALSE; //팩과 함께 움직였는가?

    switch (dir)
    {
    case VK_LEFT:
        dx = -1;
        imageDir = 5;
        break;
    case VK_RIGHT:
        dx = 1;
        imageDir = 6;
        break;
    case VK_UP:
        dy = -1;
        imageDir = 3;
        break;
    case VK_DOWN:
        dy = 1;
        imageDir = 4;

        break;
    default:
        break;
    }

    if (ns[ny + dy][nx + dx] != '#')
    {
        redoList->clear();
        if (ns[ny + dy][nx + dx] == 'O') //상자를 밀었다
        {
            if (ns[ny + dy * 2][nx + dx * 2] == ' ' || ns[ny + dy * 2][nx + dx * 2] == '.')
            {
                //지금 밀리는 박스의 위치가 원래 목적지였다면 스테이지정보도 목적지로 변환
                ErasePack(nx + dx, ny + dy);
                ns[ny + dy * 2][nx + dx * 2] = 'O'; //박스를 밀어
                bWithPack = TRUE;
            }
            else { //상자 뒤쪽이 이동불가능하다면 이동자체를 취소하기 위해 return
                return;
            }
        }

        nx += dx;
        ny += dy;
        nMove++;

        MoveInfo mInfo(dx, dy, bWithPack, imageDir);
        undoList->push_back(mInfo);
        //이거가지고 잘 해봐봐

        InvalidateRect(hWndMain, NULL, FALSE);
    }
}

void Game::InitStage()
{
    memcpy(ns, arStage[nStage], sizeof(ns)); //메모리 카피
    undoList->clear();
    redoList->clear();
    for (int y = 0; y < 18; y++)
    {
        for (int x = 0; x < 20; x++)
        {
            if (ns[y][x] == '@') {  //플레이어의 시작위치
                nx = x;
                ny = y;
                ns[y][x] = ' ';  //해당위치에 플레이어가 있으니 공백처리
            }
        }
    }

    //벡터를 쓰든 뭘 쓰든 컨테이너객체를 썼다면
    //여기서 초기화하는 작업을 반드시 해줘야 한다.

    nMove = 0;
    InvalidateRect(hWndMain, NULL, TRUE);
}

void Game::DrawBitmap(HDC hdc, int idx, int x, int y, HBITMAP hBitSheet)
{
    HDC hMemDC;
    HBITMAP hOldBitMap;
    int bx, by;
    BITMAP bit;

    hMemDC = CreateCompatibleDC(hdc); //메모리 DC생성
    hOldBitMap = static_cast<HBITMAP>(SelectObject(hMemDC, hBitSheet));

    GetObject(hBitSheet, sizeof(BITMAP), &bit);

    bx = bit.bmWidth;
    by = bit.bmHeight;

    StretchBlt(hdc, x, y, 32, 32, hMemDC, idx * 32, 0, 32, 32, SRCCOPY);

    //BitBlt(hdc, x, y, bx, by, hMemDC, 0, 0, SRCCOPY);
    SelectObject(hMemDC, hOldBitMap);
    DeleteDC(hMemDC);
}
BOOL Game::GotoNextStage()
{
    if (nStage < MAX_STAGE - 1)
    {
        nStage++;
        InitStage(); // 스테이지 초기화
        return TRUE;
    }
    return FALSE;
}

BOOL Game::GotoPrevStage()
{
    if (nStage > 0)
    {
        nStage--;
        InitStage(); // 스테이지 초기화
        return TRUE;
    }
    return FALSE;
}

void Game::ErasePack(int x, int y)
{
    ns[y][x] = arStage[nStage][y][x] == '.' ? '.' : ' ';
}

int Game::GetCurrentStage()
{
    return nStage;
}

void Game::Undo()
{
    if (undoList->size() <= 0) return;
    int dx = undoList->back().dx;
    int dy = undoList->back().dy;

    if(undoList->back().bWithPack)
    {
        ErasePack(nx + dx, ny + dy);
        ns[ny][nx] = 'O';
    }
    nx -= dx;
    ny -= dy;


    imageDir = undoList->back().imageDir;

    redoList->push_back(undoList->back());
    undoList->pop_back();
    InvalidateRect(hWndMain, NULL, FALSE);

    //return true;
}
void Game::Redo()
{
    if (redoList->size() <= 0) return;
    int dx = redoList->back().dx;
    int dy = redoList->back().dy;

    if (redoList->back().bWithPack)
    {
        ErasePack(nx + dx, ny + dy);
        //ErasePack(nx, ny);
        ns[ny+dy*2][nx+dx*2] = 'O';
    }
    nx += dx;
    ny += dy;


    imageDir = redoList->back().imageDir;

    undoList->push_back(redoList->back());
    redoList->pop_back();
    InvalidateRect(hWndMain, NULL, FALSE);
}

void Game::ClearTest()
{
    for (int y = 0; y < 18; y++)
    {
        for (int x = 0; x < 20; x++)
        {           
            if (arStage[nStage][y][x] == 'O')
            {
                ns[y][x] = ' ';
            }
            else if (arStage[nStage][y][x] == '.')
            {
                ns[y][x] = 'O';
            }
        }
    }
    InvalidateRect(hWndMain, NULL, FALSE);

}
