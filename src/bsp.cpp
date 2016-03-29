#include "bsp.h"
#include <fstream>

void pmg::BSP::toTextFile(const std::string& path)
{
	std::ofstream stream(path);

	if (!stream.is_open())
		return;

	for (int y = 0; y < mHeight; y++)
	{
		for (int x = 0; x < mWidth; x++)
		{
			switch(static_cast<TileType>(mData[y*mWidth + x]))
			{
			case TileType::Hall:
				stream << "*";
				break;
			case TileType::Room:
				stream << ".";
				break;
			case TileType::Wall:
				stream << "#";
				break;
			}
		}

		stream << std::endl;
	}

	stream.close();
}