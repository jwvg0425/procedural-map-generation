#include "../src/pmg.h"

int main()
{
	pmg::BSP generator;

	generator.createMap();

	generator.toTextFile("test.txt", [](int data)
	{
		switch (static_cast<pmg::TileType>(data))
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