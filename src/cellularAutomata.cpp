#include "cellularAutomata.h"

int pmg::CellularAutomata::getAdjustWallNum(int x, int y)
{
	Point adjs[9] =
	{
		{ x, y },
		{ x + 1, y },
		{ x + 1, y + 1 },
		{ x, y + 1 },
		{ x - 1, y + 1 },
		{ x - 1, y },
		{ x - 1, y - 1 },
		{ x, y - 1 },
		{ x + 1,y - 1 }
	};

	int res = 0;

	for (int i = 0; i < 9; i++)
	{
		const Point& adj = adjs[i];

		//ȭ�� ���̸� ���� �ִ� ������ ����
		if (adj.mX < 0 || adj.mX >= mWidth || adj.mY < 0 || adj.mY >= mHeight)
		{
			res++;
		}
		else if (mData[adj.mX + adj.mY * mWidth] == TileType::Wall)
		{
			res++;
		}
	}

	return res;
}
