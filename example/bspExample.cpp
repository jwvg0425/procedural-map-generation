#include "../src/pmg.h"

int main()
{
	pmg::BSP generator;

	generator.setWidth(50);
	generator.setHeight(50);
	generator.setSplitNum(4);
	generator.setSplitRange(0.1f);
	generator.setSizeMid(0.95f);
	generator.setSizeRange(0.05f);
	generator.setComplexity(2);

	generator.createMap();

	generator.toTextFile("test.txt", [](pmg::TileType data)
	{
		switch (data)
		{
		case pmg::TileType::Hall:
			return '*';
		case pmg::TileType::Room:
			return '.';
		case pmg::TileType::Wall:
			return '#';
		case pmg::TileType::Door:
			return 'D';
		}

		return '?';
	});

	return 0;
}