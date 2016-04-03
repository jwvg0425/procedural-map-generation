#pragma once
#include <type_traits>
#include <fstream>

#include "bsp.h"
#include "agent.h"
#include "cellularAutomata.h"

namespace pmg
{
template<typename Generator>
void toTextFile(const Generator& generator,
	const std::string& path, std::function<char(TileType)> outputFunc)
{
	static_assert(std::is_same<Generator, BSP>::value ||
				  std::is_same<Generator, Agent>::value ||
				  std::is_same<Generator, CellularAutomata>::value,
		"Generator must be BSP or Agent or CellularAutomata");

	std::ofstream stream(path);

	if (!stream.is_open())
		return;

	for (int y = 0; y < generator.getHeight(); y++)
	{
		for (int x = 0; x < generator.getWidth(); x++)
		{
			stream << outputFunc(generator.getData(x, y));
		}

		stream << std::endl;
	}

	stream.close();
}

}