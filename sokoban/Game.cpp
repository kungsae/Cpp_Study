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
    wsprintf(userMsg, _T("Q : ����, R : �ٽ� ����"));
    TextOut(hdc, 700, 40, userMsg, lstrlen(userMsg));
    wsprintf(userMsg, _T("N : ����, P : ����"));
    TextOut(hdc, 700, 60, userMsg, lstrlen(userMsg));
    wsprintf(userMsg, _T("�������� : %d"), nStage + 1);
    TextOut(hdc, 700, 80, userMsg, lstrlen(userMsg));
    wsprintf(userMsg, _T("�̵� Ƚ�� : %d"), nMove);
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

    hMemDC = CreateCompatibleDC(hdc); //�޸� DC����
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
    int dx = 0, dy = 0; //��ŸX, ��ŸY
    BOOL bWithPack = FALSE; //�Ѱ� �Բ� �������°�?

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
        if (ns[ny + dy][nx + dx] == 'O') //���ڸ� �о���
        {
            if (ns[ny + dy * 2][nx + dx * 2] == ' ' || ns[ny + dy * 2][nx + dx * 2] == '.')
            {
                //���� �и��� �ڽ��� ��ġ�� ���� ���������ٸ� �������������� �������� ��ȯ
                ErasePack(nx + dx, ny + dy);
                ns[ny + dy * 2][nx + dx * 2] = 'O'; //�ڽ��� �о�
                bWithPack = TRUE;
            }
            else { //���� ������ �̵��Ұ����ϴٸ� �̵���ü�� ����ϱ� ���� return
                return;
            }
        }

        nx += dx;
        ny += dy;
        nMove++;

        MoveInfo mInfo(dx, dy, bWithPack, imageDir);
        undoList->push_back(mInfo);
        //�̰Ű����� �� �غ���

        InvalidateRect(hWndMain, NULL, FALSE);
    }
}

void Game::InitStage()
{
    memcpy(ns, arStage[nStage], sizeof(ns)); //�޸� ī��
    undoList->clear();
    redoList->clear();
    for (int y = 0; y < 18; y++)
    {
        for (int x = 0; x < 20; x++)
        {
            if (ns[y][x] == '@') {  //�÷��̾��� ������ġ
                nx = x;
                ny = y;
                ns[y][x] = ' ';  //�ش���ġ�� �÷��̾ ������ ����ó��
            }
        }
    }

    //���͸� ���� �� ���� �����̳ʰ�ü�� ��ٸ�
    //���⼭ �ʱ�ȭ�ϴ� �۾��� �ݵ�� ����� �Ѵ�.

    nMove = 0;
    InvalidateRect(hWndMain, NULL, TRUE);
}

void Game::DrawBitmap(HDC hdc, int idx, int x, int y, HBITMAP hBitSheet)
{
    HDC hMemDC;
    HBITMAP hOldBitMap;
    int bx, by;
    BITMAP bit;

    hMemDC = CreateCompatibleDC(hdc); //�޸� DC����
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
        InitStage(); // �������� �ʱ�ȭ
        return TRUE;
    }
    return FALSE;
}

BOOL Game::GotoPrevStage()
{
    if (nStage > 0)
    {
        nStage--;
        InitStage(); // �������� �ʱ�ȭ
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
