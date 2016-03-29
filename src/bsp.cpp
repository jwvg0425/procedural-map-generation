#include "bsp.h"
#include <fstream>

void pmg::BSP::toTextFile(const std::string & path)
{
	std::ofstream stream(path);

	if (!stream.is_open())
		return;

	for (int y = 0; y < mHeight; y++)
	{
		for (int x = 0; x < mWidth; x++)
		{
			if (mIsWall[y*mWidth + x])
			{
				stream << "#";
			}
			else
			{
				stream << ".";
			}
		}

		stream << std::endl;
	}

	stream.close();
}
