#pragma once
#include <memory>
#include <random>
#include <queue>
#include <functional>

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

	//0.5 +- splitRange �������� �ش� ������ �� �κ����� ������.
	template<typename RandomGenerator = std::mt19937>
	void split(float splitRange, RandomGenerator& generator)
	{
		if (splitRange < 0.1f || splitRange > 0.4f)
		{
			return;
		}

		//�̹� ���ҵ� ���
		if (hasChild())
		{
			return;
		}

		std::uniform_int_distribution<int> dirDist(0, 1);

		//���� ���� ����.
		if (dirDist(generator) == 0) // �ʺ� ���� ����
		{
			if (!widthSplit(splitRange, generator))
			{
				heightSplit(splitRange, generator);
			}
		}
		else //���� ���� ����
		{
			if (!heightSplit(splitRange, generator))
			{
				widthSplit(splitRange, generator);
			}
		}
	}

	//���� ��忡 ���� ������ش�. ���� ��� ���� / �ʺ��� 0.9 +- sizeDist ũ�⿡�� ����. 
	//sizeDist�� �ּ� 0.0 �ִ� 0.1
	template<typename RandomGenerator = std::mt19937>
	void makeRoom(float sizeRange, RandomGenerator& generator)
	{
		if (sizeRange < 0.0f || sizeRange > 1.0f)
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
		std::uniform_int_distribution<int> xDist(mInfo.mX, mInfo.mX + mInfo.mWidth - roomWidth - 1);
		std::uniform_int_distribution<int> yDist(mInfo.mY, mInfo.mY + mInfo.mHeight - roomHeight - 1);

		mRoom.mX = xDist(generator);
		mRoom.mY = yDist(generator);
		mRoom.mWidth = roomWidth;
		mRoom.mHeight = roomHeight;
	}


	//���ҵ� �� ������ ���� ��� �����ؼ� �ϳ��� ������ �����.
	template<typename RandomGenerator = std::mt19937>
	void merge(RandomGenerator& generator)
	{
		//������ �ڽ��� ����.
		if (!hasChild())
		{
			return;
		}

		// �ڽ��� �ڽ� ���� ����
		if (mLeftChild != nullptr)
		{
			mLeftChild->merge(generator);
		}

		if (mRightChild != nullptr)
		{
			mRightChild->merge(generator);
		}

		//�� �ڽ� ����. ���� �´��� ��ġ������ ������ ��� ���ؼ�, �� �� ������ �� �� or ������ ����
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

		//�̹� ����� ���� �ϳ��� �ִٸ� pass
		for (auto& leftRoom : leftCand)
		{
			for (auto& rightRoom : rightCand)
			{
				if (leftRoom.isConnect(rightRoom))
					return;
			}
		}

		// �����ϰ� �� ���� ����. �װ� ���� ������ �����ϴٸ� �����ϰ�, ������ �Ұ����ϴٸ� �ٸ� �� ���� ã�´�.
		
		//���� ���ɼ� �׽�Ʈ�� ���� �ϱ� ���� �� �࿡ �°� ����
		std::sort(leftCand.begin(), leftCand.end(), comp);
		std::sort(rightCand.begin(), rightCand.end(), comp);

		std::uniform_int_distribution<int> leftDist(0, leftCand.size() - 1);
		std::uniform_int_distribution<int> rightDist(0, rightCand.size() - 1);

		int leftIdx = 0; 
		int rightIdx = 0;

		//���� ������ ������ �ݺ��ؼ� ���� �õ�.
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
			//case 1 : ���� �࿡�� ��ġ�� �� �ִ� ���.
			//�� ��� �ݵ�� ���� ����. �����Ѵ�.
			if ((left.mY >= right.mY && left.mY < right.mY + right.mHeight) ||
				(right.mY >= left.mY && right.mY < left.mY + left.mHeight))
			{
				//y ������ 3 �̻� ������ �ִ� ��� - ���� ���� ����.
				if (right.mX - (left.mX + left.mWidth) >= 3)
				{
					std::uniform_int_distribution<int> leftDist(left.mY, left.mY + left.mHeight - 1);
					std::uniform_int_distribution<int> rightDist(right.mY, right.mY + right.mHeight - 1);

					int leftStart = leftDist(generator);
					int rightStart = rightDist(generator);
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
				else // 3���� ������ ���� ��� - ��ġ�� �������� ����.
				{
					int start = 0;
					int end = std::min(left.mY + left.mHeight, right.mY + right.mHeight);

					if (left.mY >= right.mY && left.mY < right.mY + right.mHeight)
					{
						start = left.mY;
					}
					else
					{
						start = right.mY;
					}

					std::uniform_int_distribution<int> dist(start, end - 1);

					int y = dist(generator);

					for (int x = left.mX + left.mWidth; x < right.mX; x++)
					{
						hallways.emplace_back(x, y);
					}
				}
				
				return true;
			}
			//case 2 : left �� ���� right �� �� ���� ������ 3 �̻��� ���. �� ��쵵 �ݵ�� ���� ����.
			int leftMax = left.mX + left.mWidth;
			int rightMin = right.mX;
			int yStart = std::min(left.mY, right.mY);
			int yEnd = std::max(left.mY + left.mHeight, right.mY + right.mHeight);

			//left side�� �� ��
			if (yStart == left.mY)
			{
				for (int l = leftIdx; l < leftCand.size(); l++)
				{
					if (leftCand[l].mY > right.mY + 1)
					{
						yEnd = std::min(yEnd, leftCand[l].mY);
						break;
					}

					if (leftCand[l].mX + leftCand[l].mWidth > leftMax)
					{
						leftMax = leftCand[l].mX + leftCand[l].mWidth;
					}
				}

				for (int r = rightIdx; r >= 0; r--)
				{
					if (rightCand[r].mY + rightCand[r].mHeight < left.mY + left.mHeight - 1)
					{
						yStart = std::max(yStart, rightCand[r].mY + rightCand[r].mHeight - 1);
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
					if (leftCand[l].mY + leftCand[l].mHeight < right.mY + right.mHeight - 1)
					{
						yStart = std::max(yStart, leftCand[l].mY + leftCand[l].mHeight - 1);
						break;
					}
					
					if (leftCand[l].mX + leftCand[l].mWidth > leftMax)
					{
						leftMax = leftCand[l].mX + leftCand[l].mWidth;
					}
				}

				for (int r = rightIdx; r < rightCand.size(); r++)
				{
					if (rightCand[r].mY > left.mY + 1)
					{
						yEnd = std::min(yEnd, rightCand[r].mY);
						break;
					}

					if (rightCand[r].mX < rightMin)
					{
						rightMin = rightCand[r].mX;
					}
				}
			}

			if (rightMin - leftMax >= 3)
			{
				int mid = (leftMax + rightMin) / 2;
				

				//left side�� �� ��
				if (yStart == left.mY)
				{
					std::uniform_int_distribution<int> startDist(yStart, left.mY + left.mHeight - 1);
					std::uniform_int_distribution<int> endDist(right.mY, yEnd);

					int start = startDist(generator);
					int end = endDist(generator);

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
					std::uniform_int_distribution<int> startDist(yStart, right.mY + right.mHeight - 1);
					std::uniform_int_distribution<int> endDist(left.mY, yEnd);

					int start = startDist(generator);
					int end = endDist(generator);

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
		}
		else // ��������. �ุ ��Ī��.
		{
			//case 1 : ���� �࿡�� ��ġ�� �� �ִ� ���.
			//�� ��� �ݵ�� ���� ����. �����Ѵ�.
			if ((left.mX >= right.mX && left.mX < right.mX + right.mWidth) ||
				(right.mX >= left.mX && right.mX < left.mX + left.mWidth))
			{
				//y ������ 3 �̻� ������ �ִ� ��� - ���� ���� ����.
				if (right.mY - (left.mY + left.mHeight) >= 3)
				{
					std::uniform_int_distribution<int> leftDist(left.mX, left.mX + left.mWidth - 1);
					std::uniform_int_distribution<int> rightDist(right.mX, right.mX + right.mWidth - 1);

					int leftStart = leftDist(generator);
					int rightStart = rightDist(generator);
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
				else // 3���� ������ ���� ��� - ��ġ�� �������� ����.
				{
					int start = 0;
					int end = std::min(left.mX + left.mWidth, right.mX + right.mWidth);

					if (left.mX >= right.mX && left.mX < right.mX + right.mWidth)
					{
						start = left.mX;
					}
					else
					{
						start = right.mX;
					}

					std::uniform_int_distribution<int> dist(start, end - 1);

					int x = dist(generator);

					for (int y = left.mY + left.mHeight; y < right.mY; y++)
					{
						hallways.emplace_back(x, y);
					}
				}

				return true;
			}

			//case 2 : left �� ���� right �� �� ���� ������ 3 �̻��� ���. �� ��쵵 �ݵ�� ���� ����.
			int leftMax = left.mY + left.mHeight;
			int rightMin = right.mY;
			int xStart = std::min(left.mX, right.mX);
			int xEnd = std::max(left.mX + left.mWidth, right.mX + right.mWidth);

			//left side�� �� ��
			if (xStart == left.mX)
			{
				for (int l = leftIdx; l < leftCand.size(); l++)
				{
					if (leftCand[l].mX > right.mX + 1)
					{
						xEnd = std::min(xEnd, leftCand[l].mX);
						break;
					}

					if (leftCand[l].mY + leftCand[l].mHeight > leftMax)
					{
						leftMax = leftCand[l].mY + leftCand[l].mHeight;
					}
				}

				for (int r = rightIdx; r >= 0; r--)
				{
					if (rightCand[r].mX + rightCand[r].mWidth < left.mX + left.mWidth - 1)
					{
						xStart = std::max(xStart, rightCand[r].mX + rightCand[r].mWidth - 1);
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
					if (leftCand[l].mX + leftCand[l].mWidth < right.mX + right.mWidth - 1)
					{
						xStart = std::max(xStart, leftCand[l].mX + leftCand[l].mWidth - 1);
						break;
					}

					if (leftCand[l].mY + leftCand[l].mHeight > leftMax)
					{
						leftMax = leftCand[l].mY + leftCand[l].mHeight;
					}
				}

				for (int r = rightIdx; r < rightCand.size(); r++)
				{
					if (rightCand[r].mX > left.mX + 1)
					{
						xEnd = std::min(xEnd, rightCand[r].mX);
						break;
					}

					if (rightCand[r].mY < rightMin)
					{
						rightMin = rightCand[r].mY;
					}
				}
			}

			if (rightMin - leftMax >= 3)
			{
				int mid = (leftMax + rightMin) / 2;
				//left side�� �� ��
				if (xStart == left.mX)
				{
					std::uniform_int_distribution<int> startDist(xStart, left.mX + left.mWidth - 1);
					std::uniform_int_distribution<int> endDist(right.mX, xEnd);
				
					int start = startDist(generator);
					int end = endDist(generator);

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
					std::uniform_int_distribution<int> startDist(xStart, right.mX + right.mWidth - 1);
					std::uniform_int_distribution<int> endDist(left.mX, xEnd);

					int start = startDist(generator);
					int end = endDist(generator);

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

	void fillData(int width, std::vector<bool>& isWall) const
	{
		if (mLeftChild != nullptr)
			mLeftChild->fillData(width, isWall);

		if (mRightChild != nullptr)
			mRightChild->fillData(width, isWall);

		for (auto& p : hallways)
		{
			isWall[p.mX + p.mY * width] = false;
		}

		if (!hasChild())
		{
			for (int y = mRoom.mY; y < mRoom.mY + mRoom.mHeight; y++)
			{
				for (int x = mRoom.mX; x < mRoom.mX + mRoom.mWidth; x++)
				{
					isWall[x + y * width] = false;
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

	//�ʺ� ����. ���� �� �ʺ� MINIMUM_SIZE ���� ���� �κ��� ������ ��� false ����.
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

	//���� ����. ���� �� �ʺ� MINIMUM_SIZE���� ���� �κ��� ������ ��� false ����.
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
		mIsWall()
	{
		mIsWall.resize(width * height, true);
	}

	template<typename RandomGenerator = std::mt19937>
	void createMap()
	{
		std::random_device rd;
		RandomGenerator generator(rd());

		split(generator);
		mRoot.makeRoom(mSizeRange, generator);
		mRoot.merge(generator);
		mRoot.fillData(mWidth, mIsWall);
	}

	void toTextFile(const std::string& path);

private:

	//Ʈ���� �����ؼ� ���� ���� ���� ���� �����Ѵ�.
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
	std::vector<bool> mIsWall;
};

}