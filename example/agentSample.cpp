#include "../src/pmg.h"

int main()
{
	pmg::Agent generator(50, 50, 80, 30, 0.05f, 0.05f);

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