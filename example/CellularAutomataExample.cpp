#include "../src/pmg.h"

int main()
{
	pmg::CellularAutomata generator(50, 50, 5, 0.45f, 5);

	generator.createMap();

	pmg::toTextFile(generator, "test.txt", [](pmg::TileType data)
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