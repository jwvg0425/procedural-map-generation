#pragma once
#include <random>
#include <algorithm>
#include "types.h"

namespace pmg
{
class Agent
{
	struct Node
	{
		int mEnergy;
		float mRotate;
		float mDig;
		Direction mDir;
		int mX;
		int mY;
	};

public:
	Agent(int width, int height, int agentNum, int energy, float rotateDelta, float digDelta)
		:mWidth(width), mHeight(height),
		mAgentNum(agentNum), mEnergy(energy),
		mRotateDelta(rotateDelta), mDigDelta(digDelta)
	{
		mData.resize(mWidth*mHeight, TileType::Wall);
	}

	template<typename RandomGenerator = std::mt19937>
	void createMap()
	{
		std::random_device rd;
		RandomGenerator generator(rd());

		std::vector<Node> agents;

		for (int i = 0; i < mAgentNum; i++)
		{
			agents.push_back(createNode(generator));
		}

		std::uniform_real_distribution<float> probDist(0.0f, 1.0f);
		std::uniform_int_distribution<int> clockwiseDist(0, 1);

		while (!agents.empty())
		{
			for (int i = 0; i < static_cast<int>(agents.size());)
			{
				if (agents[i].mEnergy <= 0)
				{
					agents.erase(agents.begin() + i);
					continue;
				}

				if (probDist(generator) < agents[i].mRotate)
				{
					//시계방향으로 방향 전환
					if (clockwiseDist(generator) == 1)
					{
						agents[i].mDir = static_cast<Direction>((static_cast<int>(agents[i].mDir) + 1) % 3);
					}
					else // 반시계
					{
						if (static_cast<int>(agents[i].mDir) == 0)
						{
							agents[i].mDir = Direction::Left;
						}
						else
						{
							agents[i].mDir = static_cast<Direction>(static_cast<int>(agents[i].mDir) - 1);
						}
					}

					agents[i].mEnergy--;
					agents[i].mRotate = 0.0f;
				}
				else
				{
					agents[i].mRotate += mRotateDelta;
				}

				if (probDist(generator) < agents[i].mDig)
				{
					Point next(agents[i].mX, agents[i].mY);

					switch (agents[i].mDir)
					{
					case Direction::Left:
						next.mX--;
						break;
					case Direction::Top:
						next.mY--;
						break;
					case Direction::Right:
						next.mX++;
						break;
					case Direction::Bottom:
						next.mY++;
						break;
					}

					//화면 넘어가는 경우 무시
					if (next.mX < 0 || next.mY < 0 || next.mX >= mWidth || next.mY >= mHeight)
						continue;

					if (mData[next.mX + next.mY * mWidth] == TileType::Wall)
					{
						mData[next.mX + next.mY * mWidth] = TileType::Room;
						agents[i].mEnergy--;
						agents[i].mDig = 0.0f;
					}

					agents[i].mX = next.mX;
					agents[i].mY = next.mY;
				}
				else
				{
					agents[i].mDig += mDigDelta;
				}

				i++;
			}
		}
	}

	int getWidth() const { return mWidth; }
	int getHeight() const { return mHeight; }
	TileType getData(int x, int y) const { return mData[x + y*mWidth]; }

private:
	template<typename RandomGenerator>
	Node createNode(RandomGenerator& generator)
	{
		Node res;

		res.mEnergy = mEnergy;
		res.mRotate = 0.0f;
		res.mDig = 0.0f;

		std::uniform_int_distribution<int> dirDist(0, 3);
		res.mDir = static_cast<Direction>(dirDist(generator));

		std::uniform_int_distribution<int> xDist(0, mWidth - 1);
		std::uniform_int_distribution<int> yDist(0, mHeight - 1);

		res.mX = xDist(generator);
		res.mY = yDist(generator);

		return res;
	}

	int mWidth;
	int mHeight;
	int mAgentNum;
	int mEnergy;
	float mRotateDelta;
	float mDigDelta;
	std::vector<TileType> mData;
};

}