#pragma once
#include <memory>
#include <random>
#include <queue>
#include <functional>
#include <sstream>

#ifndef OUT
#define OUT
#endif

namespace pmg
{

enum class SideType
{
	Top,
	Right,
	Bottom,
	Left
};

enum class TileType
{
	Wall,
	Hall,
	Room
};

struct Point
{
	Point()
		: mX(0), mY(0)
	{
	}

	Point(int x, int y)
		:mX(x), mY(y)
	{
	}

	int mX;
	int mY;
};

struct Rectangle
{
	Rectangle()
		: mX(0), mY(0), mWidth(0), mHeight(0)
	{
	}

	Rectangle(int x, int y, int width, int height)
		: mX(x), mY(y), mWidth(width), mHeight(height)
	{
	}

	int mX;
	int mY;
	int mWidth;
	int mHeight;

	bool isConnect(const Rectangle& other) const
	{
		if (mX == other.mX + other.mWidth ||
			other.mX == mX + mWidth)
		{
			return (mY >= other.mY && mY < other.mY + other.mHeight) ||
				(other.mY >= mY && other.mY < mY + mHeight);
		}
		
		if (mY == other.mY + other.mHeight ||
			other.mY == mY + mHeight)
		{
			return (mX >= other.mX && mX < other.mX + other.mWidth) ||
				(other.mX >= mX && other.mX < mX + mHeight);
		}

		return false;
	}
};

class Leaf
{
public:
	Leaf(int x, int y, int width, int height)
		: mInfo(x, y, width, height), mRoom(), mLeftChild(nullptr), mRightChild(nullptr)
	{
	}

	//0.5 +- splitRange 범위에서 해당 범위를 두 부분으로 나눈다.
	template<typename RandomGenerator = std::mt19937>
	void split(float splitRange, RandomGenerator& generator)
	{
		if (splitRange < 0.1f || splitRange > 0.4f)
		{
			return;
		}

		//이미 분할된 노드
		if (hasChild())
		{
			return;
		}

		std::uniform_int_distribution<int> dirDist(0, 1);

		//분할 방향 결정.
		if (dirDist(generator) == 0) // 너비 절반 분할
		{
			if (!widthSplit(splitRange, generator))
			{
				heightSplit(splitRange, generator);
			}
		}
		else //높이 절반 분할
		{
			if (!heightSplit(splitRange, generator))
			{
				widthSplit(splitRange, generator);
			}
		}
	}

	//리프 노드에 방을 만들어준다. 리프 노드 높이 / 너비의 0.9 +- sizeDist 크기에서 생성. 
	//sizeDist는 최소 0.0 최대 0.1
	template<typename RandomGenerator = std::mt19937>
	void makeRoom(float sizeRange, RandomGenerator& generator)
	{
		if (sizeRange < 0.0f || sizeRange > 0.1f)
		{
			return;
		}

		if (hasChild())
		{
			if (mLeftChild != nullptr)
				mLeftChild->makeRoom(sizeRange, generator);

			if (mRightChild != nullptr)
				mRightChild->makeRoom(sizeRange, generator);

			return;
		}

		std::uniform_real_distribution<float> sizeDist(0.9f - sizeRange, 0.9f + sizeRange);
		int roomWidth = static_cast<int>(sizeDist(generator) * mInfo.mWidth);
		int roomHeight = static_cast<int>(sizeDist(generator) * mInfo.mHeight);
		std::uniform_int_distribution<int> xDist(mInfo.mX, mInfo.mX + mInfo.mWidth - roomWidth);
		std::uniform_int_distribution<int> yDist(mInfo.mY, mInfo.mY + mInfo.mHeight - roomHeight);

		mRoom.mX = xDist(generator);
		mRoom.mY = yDist(generator);
		mRoom.mWidth = roomWidth;
		mRoom.mHeight = roomHeight;
	}


	//분할된 각 노드들의 방을 모두 연결해서 하나의 맵으로 만든다.
	template<typename RandomGenerator = std::mt19937>
	void merge(RandomGenerator& generator)
	{
		//연결할 자식이 없다.
		if (!hasChild())
		{
			return;
		}

		// 자식의 자식 먼저 연결
		if (mLeftChild != nullptr)
		{
			mLeftChild->merge(generator);
		}

		if (mRightChild != nullptr)
		{
			mRightChild->merge(generator);
		}

		//두 자식 연결. 서로 맞닿은 위치에서의 노드들을 모두 구해서, 그 중 랜덤한 두 방 or 복도를 연결
		if (mLeftChild == nullptr || mRightChild == nullptr)
			return;

		std::vector<Rectangle> leftCand;
		std::vector<Rectangle> rightCand;
		std::function<bool(const Rectangle& lhs, const Rectangle& rhs)> comp;
		
		if (mIsWidthSplit)
		{
			mLeftChild->getSideRoom(SideType::Right, leftCand);
			mRightChild->getSideRoom(SideType::Left, rightCand);
			comp = [](const Rectangle& lhs, const Rectangle& rhs)
			{
				return lhs.mY < rhs.mY;
			};
		}
		else
		{
			mLeftChild->getSideRoom(SideType::Bottom, leftCand);
			mRightChild->getSideRoom(SideType::Top, rightCand);
			comp = [](const Rectangle& lhs, const Rectangle& rhs)
			{
				return lhs.mX < rhs.mX;
			};
		}

		//이미 연결된 방이 하나라도 있다면 pass
		for (auto& leftRoom : leftCand)
		{
			for (auto& rightRoom : rightCand)
			{
				if (leftRoom.isConnect(rightRoom))
					return;
			}
		}

		// 랜덤하게 두 쌍을 고른다. 그게 서로 연결이 가능하다면 연결하고, 연결이 불가능하다면 다른 두 쌍을 찾는다.
		
		//연결 가능성 테스트를 쉽게 하기 위해 각 축에 맞게 정렬
		std::sort(leftCand.begin(), leftCand.end(), comp);
		std::sort(rightCand.begin(), rightCand.end(), comp);

		std::uniform_int_distribution<int> leftDist(0, leftCand.size() - 1);
		std::uniform_int_distribution<int> rightDist(0, rightCand.size() - 1);

		int leftIdx = 0; 
		int rightIdx = 0;

		//연결 성공할 때까지 반복해서 연결 시도.
		do
		{
			leftIdx = leftDist(generator);
			rightIdx = rightDist(generator);
		} while (!connect(leftIdx, rightIdx, leftCand, rightCand, generator));
	}

	template<typename RandomGenerator>
	bool connect(int leftIdx, int rightIdx,
		const std::vector<Rectangle>& leftCand, const std::vector<Rectangle>& rightCand,
		RandomGenerator& generator)
	{
		const Rectangle& left = leftCand.at(leftIdx);
		const Rectangle& right = rightCand.at(rightIdx);

		if (mIsWidthSplit)
		{
			//case 1 : 서로 축에서 겹치는 게 있는 경우.
			//이 경우 반드시 연결 가능. 연결한다.
			//방이 연결되는 경우 때문에 양 끝 점은 제외한다. 그래서 끝 쪽에서 겹칠 경우 약간 더 공간 필요.
			if (widthOverlapConnect(leftIdx, rightIdx, leftCand, rightCand, generator))
			{
				return true;
			}

			//case 2 : left 맨 끝과 right 맨 끝 사이 간격이 3 이상인 경우. 이 경우도 반드시 연결 가능.
			if (widthSeparateConnect(leftIdx, rightIdx, leftCand, rightCand, generator))
			{
				return true;
			}
		}
		else // 마찬가지. 축만 대칭적.
		{
			//case 1 : 서로 축에서 겹치는 게 있는 경우.
			//이 경우 반드시 연결 가능. 연결한다.
			//방이 연결되는 경우 때문에 양 끝 점은 제외한다. 그래서 끝 쪽에서 겹칠 경우 약간 더 공간 필요.
			if (heightOverlapConnect(leftIdx, rightIdx, leftCand, rightCand, generator))
			{
				return true;
			}

			//case 2 : left 맨 끝과 right 맨 끝 사이 간격이 3 이상인 경우. 이 경우도 반드시 연결 가능.
			if (heightSeparateConnect(leftIdx, rightIdx, leftCand, rightCand, generator))
			{
				return true;
			}
		}

		return false;
	}

	bool hasChild() const
	{
		return mLeftChild != nullptr || mRightChild != nullptr;
	}

	Leaf* getLeftChild() const
	{
		return mLeftChild.get();
	}

	Leaf* getRightChild() const
	{
		return mRightChild.get();
	}

	void fillData(int width, std::vector<int>& data) const
	{
		if (mLeftChild != nullptr)
			mLeftChild->fillData(width, data);

		if (mRightChild != nullptr)
			mRightChild->fillData(width, data);

		for (auto& p : hallways)
		{
			data[p.mX + p.mY * width] = static_cast<int>(TileType::Hall);
		}

		if (!hasChild())
		{
			for (int y = mRoom.mY; y < mRoom.mY + mRoom.mHeight; y++)
			{
				for (int x = mRoom.mX; x < mRoom.mX + mRoom.mWidth; x++)
				{
					data[x + y * width] = static_cast<int>(TileType::Room);
				}
			}
		}
	}

private:

	void getSideRoom(SideType type, OUT std::vector<Rectangle>& rooms)
	{
		if (!hasChild())
		{
			rooms.push_back(mRoom);
			return;
		}

		switch (type)
		{
		case SideType::Top:
			if (mLeftChild != nullptr)
				mLeftChild->getSideRoom(type, rooms);
			if (mRightChild != nullptr && mIsWidthSplit)
				mRightChild->getSideRoom(type, rooms);
			break;
		case SideType::Right:
			if (mLeftChild != nullptr && !mIsWidthSplit)
				mLeftChild->getSideRoom(type, rooms);
			if (mRightChild != nullptr)
				mRightChild->getSideRoom(type, rooms);
			break;
		case SideType::Bottom:
			if (mLeftChild != nullptr && mIsWidthSplit)
				mLeftChild->getSideRoom(type, rooms);
			if (mRightChild != nullptr)
				mRightChild->getSideRoom(type, rooms);
			break;
		case SideType::Left:
			if (mLeftChild != nullptr)
				mLeftChild->getSideRoom(type, rooms);
			if (mRightChild != nullptr && !mIsWidthSplit)
				mRightChild->getSideRoom(type, rooms);
			break;
		}
	}

	//너비 분할. 분할 후 너비가 MINIMUM_SIZE 보다 작은 부분이 존재할 경우 false 리턴.
	template<typename RandomGenerator = std::mt19937>
	bool widthSplit(float splitRange, RandomGenerator& generator)
	{
		std::uniform_real_distribution<float> rangeDist(0.5f - splitRange, 0.5f + splitRange);

		int leftWidth = static_cast<int>(rangeDist(generator) * mInfo.mWidth);
		int rightWidth = mInfo.mWidth - leftWidth;

		if (leftWidth < MINIMUM_SIZE || rightWidth < MINIMUM_SIZE)
			return false;

		mLeftChild.reset(new Leaf(mInfo.mX, mInfo.mY, leftWidth, mInfo.mHeight));
		mRightChild.reset(new Leaf(mInfo.mX + leftWidth, mInfo.mY, rightWidth, mInfo.mHeight));
		mIsWidthSplit = true;

		return true;
	}

	//높이 분할. 분할 후 너비가 MINIMUM_SIZE보다 작은 부분이 존재할 경우 false 리턴.
	template<typename RandomGenerator = std::mt19937>
	bool heightSplit(float splitRange, RandomGenerator& generator)
	{
		std::uniform_real_distribution<float> rangeDist(0.5f - splitRange, 0.5f + splitRange);

		int topHeight = static_cast<int>(rangeDist(generator) * mInfo.mHeight);
		int bottomHeight = mInfo.mHeight - topHeight;

		if (topHeight < MINIMUM_SIZE || bottomHeight < MINIMUM_SIZE)
			return false;

		mLeftChild.reset(new Leaf(mInfo.mX, mInfo.mY, mInfo.mWidth, topHeight));
		mRightChild.reset(new Leaf(mInfo.mX, mInfo.mY + topHeight, mInfo.mWidth, bottomHeight));
		mIsWidthSplit = false;

		return true;
	}

	template<typename RandomGenerator>
	bool widthOverlapConnect(int leftIdx, int rightIdx,
		const std::vector<Rectangle>& leftCand, const std::vector<Rectangle>& rightCand,
		RandomGenerator& generator)
	{
		const Rectangle& left = leftCand.at(leftIdx);
		const Rectangle& right = rightCand.at(rightIdx);

		if ((left.mY < right.mY || left.mY >= right.mY + right.mHeight - 2) &&
			(right.mY < left.mY || right.mY >= left.mY + left.mHeight - 2))
		{
			return false;
		}

		//y 공간이 3 이상 여유가 있는 경우 - 완전 랜덤 범위.
		if (right.mX - (left.mX + left.mWidth) >= 3)
		{
			int yStart = std::min(left.mY + 1, right.mY + 1);
			int yEnd = std::max(left.mY + left.mHeight - 2, right.mY + right.mHeight - 2);
			int leftStart = 0;
			int rightStart = 0;

			//left side가 더 앞
			if (left.mY < right.mY)
			{
				for (int l = leftIdx; l < static_cast<int>(leftCand.size()); l++)
				{
					if (leftCand[l].mY > right.mY + 2)
					{
						yEnd = std::min(yEnd, leftCand[l].mY - 2);
						break;
					}
				}

				for (int r = rightIdx; r >= 0; r--)
				{
					if (rightCand[r].mY + rightCand[r].mHeight < left.mY + left.mHeight - 2)
					{
						yStart = std::max(yStart, rightCand[r].mY + rightCand[r].mHeight + 3);
						break;
					}
				}

				if (yStart > left.mY + left.mHeight - 2 ||
					right.mY + 1 > yEnd)
				{
					return false;
				}

				std::uniform_int_distribution<int> leftDist(yStart, left.mY + left.mHeight - 2);
				std::uniform_int_distribution<int> rightDist(right.mY + 1, yEnd);

				leftStart = leftDist(generator);
				rightStart = rightDist(generator);
			}
			else
			{
				for (int l = leftIdx; l >= 0; l--)
				{
					if (leftCand[l].mY + leftCand[l].mHeight < right.mY + right.mHeight - 2)
					{
						yStart = std::max(yStart, leftCand[l].mY + leftCand[l].mHeight + 3);
						break;
					}
				}

				for (int r = rightIdx; r < static_cast<int>(rightCand.size()); r++)
				{
					if (rightCand[r].mY > left.mY + 2)
					{
						yEnd = std::min(yEnd, rightCand[r].mY - 2);
						break;
					}
				}

				if (left.mY + 1 > yEnd ||
					yStart > right.mY + right.mHeight - 2)
				{
					return false;
				}

				std::uniform_int_distribution<int> leftDist(left.mY + 1, yEnd);
				std::uniform_int_distribution<int> rightDist(yStart, right.mY + right.mHeight - 2);

				leftStart = leftDist(generator);
				rightStart = rightDist(generator);
			}

			int mid = (left.mX + left.mWidth + right.mX) / 2;

			for (int l = left.mX + left.mWidth; l < mid; l++)
			{
				hallways.emplace_back(l, leftStart);
			}
			for (int r = right.mX - 1; r > mid; r--)
			{
				hallways.emplace_back(r, rightStart);
			}

			int midStart = std::min(leftStart, rightStart);
			int midEnd = std::max(leftStart, rightStart);

			for (int m = midStart; m <= midEnd; m++)
			{
				hallways.emplace_back(mid, m);
			}
		}
		else // 3보다 여유가 없는 경우 - 겹치는 범위에서 랜덤.
		{
			int start = 0;
			int end = std::min(left.mY + left.mHeight - 2, right.mY + right.mHeight - 2);

			if (left.mY >= right.mY && left.mY < right.mY + right.mHeight)
			{
				start = left.mY + 1;
			}
			else
			{
				start = right.mY + 1;
			}

			_ASSERT(start <= end);

			std::uniform_int_distribution<int> dist(start, end);

			int y = dist(generator);

			for (int x = left.mX + left.mWidth; x < right.mX; x++)
			{
				hallways.emplace_back(x, y);
			}
		}

		return true;
	}

	template<typename RandomGenerator>
	bool heightOverlapConnect(int leftIdx, int rightIdx,
		const std::vector<Rectangle>& leftCand, const std::vector<Rectangle>& rightCand,
		RandomGenerator& generator)
	{
		const Rectangle& left = leftCand.at(leftIdx);
		const Rectangle& right = rightCand.at(rightIdx);

		if ((left.mX < right.mX || left.mX >= right.mX + right.mWidth - 2) &&
			(right.mX < left.mX || right.mX >= left.mX + left.mWidth - 2))
		{
			return false;
		}

		//y 공간이 3 이상 여유가 있는 경우 - 완전 랜덤 범위.
		if (right.mY - (left.mY + left.mHeight) >= 3)
		{
			int xStart = std::min(left.mX + 1, right.mX + 1);
			int xEnd = std::max(left.mX + left.mWidth - 2, right.mX + right.mWidth - 2);
			int leftStart = 0;
			int rightStart = 0;

			//left side가 더 앞
			if (left.mX < right.mX)
			{
				for (int l = leftIdx; l < static_cast<int>(leftCand.size()); l++)
				{
					if (leftCand[l].mX > right.mX + 2)
					{
						xEnd = std::min(xEnd, leftCand[l].mX - 2);
						break;
					}
				}

				for (int r = rightIdx; r >= 0; r--)
				{
					if (rightCand[r].mX + rightCand[r].mWidth < left.mX + left.mWidth - 2)
					{
						xStart = std::max(xStart, rightCand[r].mX + rightCand[r].mWidth + 3);
						break;
					}
				}

				std::uniform_int_distribution<int> leftDist(xStart, left.mX + left.mWidth - 2);
				std::uniform_int_distribution<int> rightDist(right.mX + 1, xEnd);

				leftStart = leftDist(generator);
				rightStart = rightDist(generator);
			}
			else
			{
				for (int l = leftIdx; l >= 0; l--)
				{
					if (leftCand[l].mX + leftCand[l].mWidth < right.mX + right.mWidth - 2)
					{
						xStart = std::max(xStart, leftCand[l].mX + leftCand[l].mWidth + 3);
						break;
					}

					for (int r = rightIdx; r < static_cast<int>(rightCand.size()); r++)
					{
						if (rightCand[r].mX > left.mX + 2)
						{
							xEnd = std::min(xEnd, rightCand[r].mX - 2);
							break;
						}
					}
				}

				std::uniform_int_distribution<int> leftDist(left.mX + 1, xEnd);
				std::uniform_int_distribution<int> rightDist(xStart, right.mX + right.mWidth - 2);

				leftStart = leftDist(generator);
				rightStart = rightDist(generator);
			}

			int mid = (left.mY + left.mHeight + right.mY) / 2;

			for (int l = left.mY + left.mHeight; l < mid; l++)
			{
				hallways.emplace_back(leftStart, l);
			}
			for (int r = right.mY - 1; r > mid; r--)
			{
				hallways.emplace_back(rightStart, r);
			}

			int midStart = std::min(leftStart, rightStart);
			int midEnd = std::max(leftStart, rightStart);

			for (int m = midStart; m <= midEnd; m++)
			{
				hallways.emplace_back(m, mid);
			}
		}
		else // 3보다 여유가 없는 경우 - 겹치는 범위에서 랜덤.
		{
			int start = 0;
			int end = std::min(left.mX + left.mWidth - 2, right.mX + right.mWidth - 2);

			if (left.mX >= right.mX && left.mX < right.mX + right.mWidth)
			{
				start = left.mX + 1;
			}
			else
			{
				start = right.mX + 1;
			}

			_ASSERT(start <= end);

			std::uniform_int_distribution<int> dist(start, end);

			int x = dist(generator);

			for (int y = left.mY + left.mHeight; y < right.mY; y++)
			{
				hallways.emplace_back(x, y);
			}
		}

		return true;
	}

	template<typename RandomGenerator>
	bool widthSeparateConnect(int leftIdx, int rightIdx,
		const std::vector<Rectangle>& leftCand, const std::vector<Rectangle>& rightCand,
		RandomGenerator& generator)
	{
		const Rectangle& left = leftCand.at(leftIdx);
		const Rectangle& right = rightCand.at(rightIdx);

		int leftMax = left.mX + left.mWidth;
		int rightMin = right.mX;
		int yStart = std::min(left.mY + 1, right.mY + 1);
		int yEnd = std::max(left.mY + left.mHeight - 2, right.mY + right.mHeight - 2);

		//left side가 더 앞
		if (left.mY < right.mY)
		{
			for (int l = leftIdx; l < static_cast<int>(leftCand.size()); l++)
			{
				if (leftCand[l].mY > right.mY + 2)
				{
					yEnd = std::min(yEnd, leftCand[l].mY - 2);
					break;
				}

				if (leftCand[l].mX + leftCand[l].mWidth > leftMax)
				{
					leftMax = leftCand[l].mX + leftCand[l].mWidth;
				}
			}

			for (int r = rightIdx; r >= 0; r--)
			{
				if (rightCand[r].mY + rightCand[r].mHeight < left.mY + left.mHeight - 2)
				{
					yStart = std::max(yStart, rightCand[r].mY + rightCand[r].mHeight + 3);
					break;
				}

				if (rightCand[r].mX < rightMin)
				{
					rightMin = rightCand[r].mX;
				}
			}
		}
		else
		{
			for (int l = leftIdx; l >= 0; l--)
			{
				if (leftCand[l].mY + leftCand[l].mHeight < right.mY + right.mHeight - 2)
				{
					yStart = std::max(yStart, leftCand[l].mY + leftCand[l].mHeight + 3);
					break;
				}

				if (leftCand[l].mX + leftCand[l].mWidth > leftMax)
				{
					leftMax = leftCand[l].mX + leftCand[l].mWidth;
				}
			}

			for (int r = rightIdx; r < static_cast<int>(rightCand.size()); r++)
			{
				if (rightCand[r].mY > left.mY + 2)
				{
					yEnd = std::min(yEnd, rightCand[r].mY - 2);
					break;
				}

				if (rightCand[r].mX < rightMin)
				{
					rightMin = rightCand[r].mX;
				}
			}
		}

		//공간 부족
		if (rightMin - leftMax < 3)
		{
			return false;
		}

		int mid = (leftMax + rightMin) / 2;

		//left side가 더 앞
		if (left.mY < right.mY)
		{
			if (yStart > left.mY + left.mHeight - 2 ||
				right.mY + 1 > yEnd)
			{
				return false;
			}

			std::uniform_int_distribution<int> startDist(yStart, left.mY + left.mHeight - 2);
			std::uniform_int_distribution<int> endDist(right.mY + 1, yEnd);

			int start = startDist(generator);
			int end = endDist(generator);
			_ASSERT(start <= end);

			for (int l = left.mX + left.mWidth; l < mid; l++)
			{
				hallways.emplace_back(l, start);
			}

			for (int r = right.mX - 1; r > mid; r--)
			{
				hallways.emplace_back(r, end);
			}

			for (int y = start; y <= end; y++)
			{
				hallways.emplace_back(mid, y);
			}
		}
		else
		{
			if (yStart > right.mY + right.mHeight - 2 ||
				left.mY + 1 > yEnd)
			{
				return false;
			}

			std::uniform_int_distribution<int> startDist(yStart, right.mY + right.mHeight - 2);
			std::uniform_int_distribution<int> endDist(left.mY + 1, yEnd);

			int start = startDist(generator);
			int end = endDist(generator);
			_ASSERT(start <= end);

			for (int r = right.mX - 1; r > mid; r--)
			{
				hallways.emplace_back(r, start);
			}

			for (int l = left.mX + left.mWidth; l < mid; l++)
			{
				hallways.emplace_back(l, end);
			}

			for (int y = start; y <= end; y++)
			{
				hallways.emplace_back(mid, y);
			}
		}

		return true;
	}

	template<typename RandomGenerator>
	bool heightSeparateConnect(int leftIdx, int rightIdx,
		const std::vector<Rectangle>& leftCand, const std::vector<Rectangle>& rightCand,
		RandomGenerator& generator)
	{
		const Rectangle& left = leftCand.at(leftIdx);
		const Rectangle& right = rightCand.at(rightIdx);

		int leftMax = left.mY + left.mHeight;
		int rightMin = right.mY;
		int xStart = std::min(left.mX + 1, right.mX + 1);
		int xEnd = std::max(left.mX + left.mWidth - 2, right.mX + right.mWidth - 2);

		//left side가 더 앞
		if (left.mX < right.mX)
		{
			for (int l = leftIdx; l < static_cast<int>(leftCand.size()); l++)
			{
				if (leftCand[l].mX > right.mX + 2)
				{
					xEnd = std::min(xEnd, leftCand[l].mX - 2);
					break;
				}

				if (leftCand[l].mY + leftCand[l].mHeight > leftMax)
				{
					leftMax = leftCand[l].mY + leftCand[l].mHeight;
				}
			}

			for (int r = rightIdx; r >= 0; r--)
			{
				if (rightCand[r].mX + rightCand[r].mWidth < left.mX + left.mWidth - 2)
				{
					xStart = std::max(xStart, rightCand[r].mX + rightCand[r].mWidth + 3);
					break;
				}

				if (rightCand[r].mY < rightMin)
				{
					rightMin = rightCand[r].mY;
				}
			}
		}
		else
		{
			for (int l = leftIdx; l >= 0; l--)
			{
				if (leftCand[l].mX + leftCand[l].mWidth < right.mX + right.mWidth - 2)
				{
					xStart = std::max(xStart, leftCand[l].mX + leftCand[l].mWidth + 3);
					break;
				}

				if (leftCand[l].mY + leftCand[l].mHeight > leftMax)
				{
					leftMax = leftCand[l].mY + leftCand[l].mHeight;
				}
			}

			for (int r = rightIdx; r < static_cast<int>(rightCand.size()); r++)
			{
				if (rightCand[r].mX > left.mX + 2)
				{
					xEnd = std::min(xEnd, rightCand[r].mX - 2);
					break;
				}

				if (rightCand[r].mY < rightMin)
				{
					rightMin = rightCand[r].mY;
				}
			}
		}

		//공간 부족
		if (rightMin - leftMax < 3)
		{
			return false;
		}

		int mid = (leftMax + rightMin) / 2;
		//left side가 더 앞
		if (left.mX < right.mX)
		{
			//가능한 위치가 없는 경우
			if (xStart > left.mX + left.mWidth - 2 ||
				right.mX + 1 > xEnd)
			{
				return false;
			}

			std::uniform_int_distribution<int> startDist(xStart, left.mX + left.mWidth - 2);
			std::uniform_int_distribution<int> endDist(right.mX + 1, xEnd);

			int start = startDist(generator);
			int end = endDist(generator);
			_ASSERT(start <= end);

			for (int l = left.mY + left.mHeight; l < mid; l++)
			{
				hallways.emplace_back(start, l);
			}

			for (int r = right.mY - 1; r > mid; r--)
			{
				hallways.emplace_back(end, r);
			}

			for (int x = start; x <= end; x++)
			{
				hallways.emplace_back(x, mid);
			}
		}
		else
		{
			//가능한 위치가 없는 경우
			if (xStart > right.mX + right.mWidth - 2 ||
				left.mX + 1 > xEnd)
			{
				return false;
			}

			std::uniform_int_distribution<int> startDist(xStart, right.mX + right.mWidth - 2);
			std::uniform_int_distribution<int> endDist(left.mX + 1, xEnd);

			int start = startDist(generator);
			int end = endDist(generator);
			_ASSERT(start <= end);

			for (int r = right.mY - 1; r > mid; r--)
			{
				hallways.emplace_back(start, r);
			}

			for (int l = left.mY + left.mHeight; l < mid; l++)
			{
				hallways.emplace_back(end, l);
			}

			for (int x = start; x <= end; x++)
			{
				hallways.emplace_back(x, mid);
			}
		}

		return true;
	}

	const int MINIMUM_SIZE = 10;

	std::vector<Point> hallways;
	Rectangle mInfo;
	Rectangle mRoom;
	std::unique_ptr<Leaf> mLeftChild;
	std::unique_ptr<Leaf> mRightChild;
	bool mIsWidthSplit;
};

class BSP
{
public:
	BSP(int width, int height, int splitNum, int sizeRange, int splitRange)
		: mWidth(width), mHeight(height), 
		mSplitNum(splitNum), mSplitRange(static_cast<float>(splitRange) / 100.0f), 
		mSizeRange(static_cast<float>(sizeRange) / 100.0f),
		mRoot(0, 0, width, height),
		mData()
	{
		mData.resize(width * height, static_cast<int>(TileType::Wall));
	}

	template<typename RandomGenerator = std::mt19937>
	void createMap()
	{
		std::random_device rd;
		RandomGenerator generator(rd());

		split(generator);
		mRoot.makeRoom(mSizeRange, generator);
		mRoot.merge(generator);
		mRoot.fillData(mWidth, mData);
	}

	void toTextFile(const std::string& path);

private:

	//트리를 분할해서 맵을 여러 개의 노드로 분할한다.
	template<typename RandomGenerator = std::mt19937>
	void split(RandomGenerator& generator)
	{
		std::queue<Leaf*> leaves;

		leaves.push(&mRoot);

		for (int i = 0; i < mSplitNum; i++)
		{
			int size = leaves.size();

			for (int s = 0; s < size; s++)
			{
				auto top = leaves.front();
				leaves.pop();

				top->split(mSplitRange, generator);

				if (top->getLeftChild() != nullptr)
					leaves.push(top->getLeftChild());

				if (top->getRightChild() != nullptr)
					leaves.push(top->getRightChild());
			}
		}
	}

	int mWidth;
	int mHeight;
	int mSplitNum;
	float  mSplitRange;
	float mSizeRange;
	Leaf mRoot;
	std::vector<int> mData;
};

}