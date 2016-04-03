#pragma once
#include <random>
#include "types.h"

namespace pmg
{

class CellularAutomata
{
public:
	CellularAutomata(int width, int height, int iteration, float initialWallRate, int wallCriterionNum)
		:mWidth(width), mHeight(height), 
		mIterationNum(iteration), mInitialWallRate(initialWallRate), mWallCriterionNum(wallCriterionNum)
	{
		mData.resize(mWidth * mHeight, TileType::Wall);
	}

	template<typename RandomGenerator = std::mt19937>
	void createMap()
	{
		std::random_device rd;
		RandomGenerator generator(rd());

		std::uniform_real_distribution<float> probDist(0.0f, 1.0f);

		for (int y = 0; y < mHeight; y++)
		{
			for (int x = 0; x < mWidth; x++)
			{
				if (probDist(generator) < mInitialWallRate)
				{
					mData[x + y * mWidth] = TileType::Wall;
				}
				else
				{
					mData[x + y * mWidth] = TileType::Room;
				}
			}
		}

		std::vector<TileType> nextData;

		nextData.resize(mWidth * mHeight, TileType::Wall);

		for (int i = 0; i < mIterationNum; i++)
		{
			for (int y = 0; y < mHeight; y++)
			{
				for (int x = 0; x < mWidth; x++)
				{
					int adjust = getAdjustWallNum(x, y);

					if (adjust >= mWallCriterionNum)
					{
						nextData[x + y*mWidth] = TileType::Wall;
					}
					else
					{
						nextData[x + y*mWidth] = TileType::Room;
					}
				}
			}

			std::swap(mData, nextData);
		}
	}

	int getWidth() const { return mWidth; }
	int getHeight() const { return mHeight; }
	TileType getData(int x, int y) const { return mData[x + y * mWidth]; }

private:
	int getAdjustWallNum(int x, int y);

	int mWidth;
	int mHeight;
	int mIterationNum;
	float mInitialWallRate;
	int mWallCriterionNum;
	
	std::vector<TileType> mData;
};

}