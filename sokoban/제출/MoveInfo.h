#pragma once
class MoveInfo
{
public:
	char dx : 3; //��Ʈ �ʵ�
	char dy : 3;
	char bWithPack : 2; 
	int imageDir;
	MoveInfo(int dx, int dy, bool bWithPack,int imageDir)
	{
		this->dx = dx;
		this->dy = dy;
		this->bWithPack = bWithPack;
		this->imageDir = imageDir;
		//��Ʈ �ʵ�� ���ϴ� ��Ʈ�� �߶� �������� ������ �ϴ� ��Ʈ�� �߷�������.
	}
};

