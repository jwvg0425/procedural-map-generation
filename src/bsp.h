#pragma once
#include <memory>
#include <random>
#include <queue>
#include <functional>
#include <sstream>
#include <algorithm>
#include <iterator>

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
	Room,
	Door
};

struct Point
{
	Point() : mX(0), mY(0) { }
	Point(int x, int y) :mX(x), mY(y) {	}

	bool operator ==(const Point& rhs) const { return mX == rhs.mX && mY == rhs.mY; }

	bool operator !=(const Point& rhs) const { return !(*this == rhs); }

	int mX;
	int mY;
};

struct Rectangle
{
	Rectangle() : mX(0), mY(0), mWidth(0), mHeight(0) { }
	Rectangle(int x, int y, int width, int height) : mX(x), mY(y), mWidth(width), mHeight(height) { }

	bool isConnect(const Rectangle& other) const;
	bool isContain(const Point& pos) const;

	int getRight() const { return mX + mWidth - 1; }
	int getBottom() const { return mY + mHeight - 1; }

	int mX;
	int mY;
	int mWidth;
	int mHeight;
};

struct Room : Rectangle
{
	Room() : Rectangle(), mIsVisited(false) { }
	Room(int x, int y, int width, int height) : Rectangle(x, y, width, height), mIsVisited(false) { }
	
	void fillData(int width, int height, std::vector<TileType>& data);

	bool isWallPos(int x, int y, int width, int height, const std::vector<Room*>& rooms);
	
	std::vector<Room*> getAllRooms();

	std::vector<Room*> mConnectedRooms;
	std::vector<Point> mDoors;
	bool mIsVisited;
};

class Leaf
{
public:
	Leaf(int x, int y, int width, int height) 
		: mInfo(x, y, width, height), mRoom(), mLeftChild(nullptr), mRightChild(nullptr)
	{
	}

	void reset(int x, int y, int width, int height)
	{
		mInfo.mX = x;
		mInfo.mY = y;
		mInfo.mWidth = width;
		mInfo.mHeight = height;

		mHallways.clear();
		mRoom = Room();
		mLeftChild.reset(nullptr);
		mRightChild.reset(nullptr);
	}

	//0.5 +- splitRange �������� �ش� ������ �� �κ����� ������.
	template<typename RandomGenerator>
	void split(float splitRange, RandomGenerator& generator)
	{
		if (splitRange < 0.1f || splitRange > 0.4f)
			return;

		//�̹� ���ҵ� ���
		if (hasChild())
			return;

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

	//���� ��忡 ���� ������ش�. ���� ��� ���� / �ʺ��� sizeMid +- sizeDist ũ�⿡�� ����. 
	template<typename RandomGenerator>
	void makeRoom(float sizeMid, float sizeRange, RandomGenerator& generator)
	{
		if (sizeMid - sizeRange < 0.0f || sizeMid + sizeRange > 1.0f)
			return;

		if (hasChild())
		{
			if (mLeftChild != nullptr)
				mLeftChild->makeRoom(sizeMid, sizeRange, generator);

			if (mRightChild != nullptr)
				mRightChild->makeRoom(sizeMid, sizeRange, generator);

			return;
		}

		std::uniform_real_distribution<float> sizeDist(sizeMid - sizeRange, sizeMid + sizeRange);
		int roomWidth = static_cast<int>(sizeDist(generator) * mInfo.mWidth);
		int roomHeight = static_cast<int>(sizeDist(generator) * mInfo.mHeight);

		if (roomWidth < ROOM_MINIMUM_SIZE)
			roomWidth = ROOM_MINIMUM_SIZE;

		if (roomHeight < ROOM_MINIMUM_SIZE)
			roomHeight = ROOM_MINIMUM_SIZE;

		std::uniform_int_distribution<int> xDist(mInfo.mX, mInfo.getRight() + 1 - roomWidth);
		std::uniform_int_distribution<int> yDist(mInfo.mY, mInfo.getBottom() + 1 - roomHeight);

		mRoom.mX = xDist(generator);
		mRoom.mY = yDist(generator);
		mRoom.mWidth = roomWidth;
		mRoom.mHeight = roomHeight;
	}


	//���ҵ� �� ������ ���� ��� �����ؼ� �ϳ��� ������ �����.
	template<typename RandomGenerator>
	void merge(int complexity, RandomGenerator& generator)
	{
		//������ �ڽ��� ����.
		if (!hasChild())
			return;

		// �ڽ��� �ڽ� ���� ����
		if (mLeftChild != nullptr)
			mLeftChild->merge(complexity, generator);

		if (mRightChild != nullptr)
			mRightChild->merge(complexity, generator);

		//�� �ڽ� ����. ���� �´��� ��ġ������ ������ ��� ���ؼ�, �� �� ������ �� ���� ����
		if (mLeftChild == nullptr || mRightChild == nullptr)
			return;

		std::vector<Room*> leftCand;
		std::vector<Room*> rightCand;
		
		if (mIsWidthSplit)
		{
			mLeftChild->getSideRoom(SideType::Right, leftCand);
			mRightChild->getSideRoom(SideType::Left, rightCand);
		}
		else
		{
			mLeftChild->getSideRoom(SideType::Bottom, leftCand);
			mRightChild->getSideRoom(SideType::Top, rightCand);
		}

		//�̹� ����� ���� �ϳ��� �ִٸ� pass
		bool alreadyConnected = false;
		for (auto& leftRoom : leftCand)
		{
			for (auto& rightRoom : rightCand)
			{
				if (leftRoom->isConnect(*rightRoom))
				{
					alreadyConnected = true;
					leftRoom->mConnectedRooms.push_back(rightRoom);
					rightRoom->mConnectedRooms.push_back(leftRoom);
				}
			}
		}

		if (alreadyConnected)
			return;

		connect(complexity, leftCand, rightCand, generator);
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

	void fillData(int width, int height, std::vector<TileType>& data);

private:
	void getSideRoom(SideType type, OUT std::vector<Room*>& rooms);
	void getAllRooms(OUT std::vector<Rectangle>& rooms);
	void getAllHallways(OUT std::vector<Point>& hallways);
	Point getDoorNextPos(const Point& door, const Room& room);

	bool isConnect(Point begin, Point end,
		const std::vector<Point>& hallways, std::vector<Point>& visited);

	bool isContainPoint(const std::vector<Rectangle>& rects, const Point& point)
	{
		return std::any_of(rects.begin(), rects.end(), [&point](const Rectangle& r)
		{
			return r.isContain(point);
		});
	}

	bool isContainPoint(const std::vector<Rectangle>& rects, int x, int y)
	{
		return isContainPoint(rects, { x, y });
	}

	bool isContainPoint(const std::vector<Point>& points, const Point& point)
	{
		return std::any_of(points.begin(), points.end(), [&point](const Point& p)
		{
			return p == point;
		});
	}

	bool isContainPoint(const std::vector<Point>& points, int x, int y)
	{
		return isContainPoint(points, { x, y });
	}

	bool isValidHallpos(const Point& pos, SideType side,
		Rectangle area, const std::vector<Rectangle>& rooms,
		const std::vector<Point>& otherHall, const std::vector<Point>& visited);

	//�ʺ� ����. ���� �� �ʺ� LEAF_MINIMUM_SIZE ���� ���� �κ��� ������ ��� false ����.
	template<typename RandomGenerator>
	bool widthSplit(float splitRange, RandomGenerator& generator)
	{
		std::uniform_real_distribution<float> rangeDist(0.5f - splitRange, 0.5f + splitRange);

		int leftWidth = static_cast<int>(rangeDist(generator) * mInfo.mWidth);
		int rightWidth = mInfo.mWidth - leftWidth;

		if (leftWidth < LEAF_MINIMUM_SIZE || rightWidth < LEAF_MINIMUM_SIZE)
			return false;

		mLeftChild.reset(new Leaf(mInfo.mX, mInfo.mY, leftWidth, mInfo.mHeight));
		mRightChild.reset(new Leaf(mInfo.mX + leftWidth, mInfo.mY, rightWidth, mInfo.mHeight));
		mIsWidthSplit = true;

		return true;
	}

	//���� ����. ���� �� �ʺ� LEAF_MINIMUM_SIZE���� ���� �κ��� ������ ��� false ����.
	template<typename RandomGenerator>
	bool heightSplit(float splitRange, RandomGenerator& generator)
	{
		std::uniform_real_distribution<float> rangeDist(0.5f - splitRange, 0.5f + splitRange);

		int topHeight = static_cast<int>(rangeDist(generator) * mInfo.mHeight);
		int bottomHeight = mInfo.mHeight - topHeight;

		if (topHeight < LEAF_MINIMUM_SIZE || bottomHeight < LEAF_MINIMUM_SIZE)
			return false;

		mLeftChild.reset(new Leaf(mInfo.mX, mInfo.mY, mInfo.mWidth, topHeight));
		mRightChild.reset(new Leaf(mInfo.mX, mInfo.mY + topHeight, mInfo.mWidth, bottomHeight));
		mIsWidthSplit = false;

		return true;
	}

	template<typename RandomGenerator>
	void connect(int complexity, const std::vector<Room*>& leftCand, const std::vector<Room*>& rightCand, 
		RandomGenerator& generator)
	{
		std::vector<Point> hallways;
		getAllHallways(hallways);

		std::vector<Rectangle> rooms;
		getAllRooms(rooms);

		Point beginDoor;
		Point endDoor;
		Point beginHall, endHall;
		std::vector<Point> visited;
		
		Rectangle area;
		int beginRoomIdx;
		int endRoomIdx;
		int prevSize = hallways.size();

		do
		{
			hallways.resize(prevSize);

			//leftCand �߿� �� �ϳ� ��� ���⸦ ���� ������ ����.
			std::uniform_int_distribution<int> beginDist(0, leftCand.size() - 1);
			beginRoomIdx = beginDist(generator);
			auto& begin = *leftCand[beginRoomIdx];

			//���������� rightCand �߿� �� �ϳ� ��� �� ������ ����.
			std::uniform_int_distribution<int> endDist(0, rightCand.size() - 1);
			endRoomIdx = endDist(generator);
			auto& end = *rightCand[endRoomIdx];

			//begin�� end ��ġ�� ����. �� ��ġ�� ���� ���� �� ���� �����Ѵ�.
			beginDoor = getRandomDoor(begin, true,generator);
			endDoor = getRandomDoor(end, false, generator);
			visited.clear();

			beginHall = getDoorNextPos(beginDoor, begin);
			endHall = getDoorNextPos(endDoor, end);

			int ax = std::min(beginHall.mX, endHall.mX) - 2;
			int ay = std::min(beginHall.mY, endHall.mY) - 2;
			int awidth = std::max(beginHall.mX, endHall.mX) - ax + 3;
			int aheight = std::max(beginHall.mY, endHall.mY) - ay + 3;
			area = Rectangle(ax, ay, awidth, aheight);

			//���� ������ ������ �����ϰ� �ٲ㰡�鼭 ��� �õ�.
		} while (!isConnect(beginHall, endHall, hallways, visited) &&
			!makeHallway(beginHall, endHall, area, complexity, rooms, visited, hallways, generator));
		//�̹� �� ���� �����ϴ� ������ �����ϰų�, �� �� ���̿� ������ ����� �Ϳ� �����ϸ� ��������.

		leftCand[beginRoomIdx]->mDoors.push_back(beginDoor);
		rightCand[endRoomIdx]->mDoors.push_back(endDoor);
	}

	//dfs ������� �� ������ Ž���ϸ� ������ ������.
	template<typename RandomGenerator>
	bool makeHallway(Point begin, Point end, const Rectangle& area, int complexity,
		const std::vector<Rectangle>& rooms, std::vector<Point>& visited, std::vector<Point>& otherHall,
		RandomGenerator& generator)
	{
		Rectangle bound(mInfo.mX + 1, mInfo.mY + 1, mInfo.mWidth - 2, mInfo.mHeight - 2);
		if (!bound.isContain(begin))
		{
			return false;
		}

		visited.push_back(begin);
		otherHall.push_back(begin);

		if (begin == end)
		{
			mHallways.push_back(begin);
			return true;
		}

		//�ֺ� 4 ���� �׽�Ʈ.
		
		std::vector<Point> cand;
		Point left(begin.mX - 1, begin.mY);
		Point up(begin.mX, begin.mY - 1);
		Point right(begin.mX + 1, begin.mY);
		Point down(begin.mX, begin.mY + 1);

		if (isValidHallpos(left, SideType::Left, area, rooms, otherHall, visited))
				cand.push_back(left);

		if (isValidHallpos(up, SideType::Top, area, rooms, otherHall, visited))
			cand.push_back(up);

		if (isValidHallpos(right, SideType::Right, area, rooms, otherHall, visited))
			cand.push_back(right);

		if (isValidHallpos(down, SideType::Bottom, area, rooms, otherHall, visited))
			cand.push_back(down);

		std::sort(cand.begin(), cand.end(), [&end](const Point& lhs, const Point& rhs)
		{
			int l = (lhs.mX - end.mX) * (lhs.mX - end.mX) + (lhs.mY - end.mY) * (lhs.mY - end.mY);
			int r = (rhs.mX - end.mX) * (rhs.mX - end.mX) + (rhs.mY - end.mY) * (rhs.mY - end.mY);

			return l < r;
		});

		//���ĵ� ��ġ����, ���⵵�� ���� �迭�� �Ϻκи� �����ϰ� ���´�.
		int offset = std::max<int>(0, cand.size() - complexity - 1);

		std::shuffle(cand.begin() + offset, cand.end(), generator);

		for (auto& c : cand)
		{
			if (makeHallway(c, end, area, complexity, rooms, visited, otherHall, generator))
			{
				mHallways.push_back(begin);
				return true;
			}
		}

		return false;
	}


	//�־��� �濡�� �����ϰ� ���� �� �� �ִ� ��ġ �ϳ��� ��ȯ�Ѵ�.
	template<typename RandomGenerator>
	Point getRandomDoor(const Room& room, bool isBegin, RandomGenerator& generator)
	{
		std::vector<Point> cand;

		if (mIsWidthSplit)
		{
			int ys[2] = { room.mY, room.getBottom() };

			for (int idx = 0; idx < 2; idx++)
			{
				int y = ys[idx];

				for (int x = room.mX + 1; x < room.getRight(); x++)
				{
					//���� ���޾� 2���� �پ� ������ �̻���.
					if (isContainPoint(room.mDoors, { x - 1, y }) ||
						isContainPoint(room.mDoors, { x + 1, y }))
					{
						continue;
					}

					cand.emplace_back(x, y);
				}
			}

			int x;

			if (isBegin)
				x = room.getRight();
			else
				x = room.mX;

			for (int y = room.mY + 1; y < room.getBottom(); y++)
			{
				//���� ���޾� 2���� �پ� ������ �̻���.
				if (isContainPoint(room.mDoors, { x, y - 1 }) ||
					isContainPoint(room.mDoors, { x, y + 1 }))
				{
					continue;
				}

				cand.emplace_back(x, y);
			}
		}
		else // x,y ��Ī�̰� �ڵ�� ����.
		{
			int xs[2] = { room.mX, room.getRight() };

			for (int idx = 0; idx < 2; idx++)
			{
				int x = xs[idx];

				for (int y = room.mY + 1; y < room.getBottom(); y++)
				{
					//���� ���޾� 2���� �پ� ������ �̻���.
					if (isContainPoint(room.mDoors, { x, y - 1 }) ||
						isContainPoint(room.mDoors, { x, y + 1 }))
					{
						continue;
					}

					cand.emplace_back(x, y);
				}
			}

			int y;

			if (isBegin)
				y = room.getBottom();
			else
				y = room.mY;

			for (int x = room.mX + 1; x < room.mX + room.mWidth - 1; x++)
			{
				//���� ���޾� 2���� �پ� ������ �̻���.
				if (isContainPoint(room.mDoors, { x - 1, y }) ||
					isContainPoint(room.mDoors, { x + 1, y }))
				{
					continue;
				}

				cand.emplace_back(x, y);
			}
		}

		std::uniform_int_distribution<int> candDist(0, cand.size() - 1);
		Point door = cand[candDist(generator)];

		return door;
	}

	const int LEAF_MINIMUM_SIZE = 10;
	const int ROOM_MINIMUM_SIZE = 5;

	std::vector<Point> mHallways;
	Rectangle mInfo;
	Room mRoom;
	std::unique_ptr<Leaf> mLeftChild;
	std::unique_ptr<Leaf> mRightChild;
	bool mIsWidthSplit;
};

class BSP
{
public:
	BSP()
		:mRoot(0, 0, mWidth, mHeight)
	{
		mData.resize(mWidth * mHeight, TileType::Wall);
	}

	BSP(int width, int height, int splitNum, float splitRange, float sizeMid, float sizeRange, int complexity)
		: mWidth(width), mHeight(height), 
		mSplitNum(splitNum), mSplitRange(splitRange), 
		mSizeMid(sizeMid), mSizeRange(sizeRange),
		mComplexity(complexity),
		mRoot(0, 0, width, height),
		mData()
	{
		mData.resize(width * height, TileType::Wall);
	}

	template<typename RandomGenerator = std::mt19937>
	void createMap()
	{
		if (mIsCreated)
		{
			mRoot.reset(0, 0, mWidth, mHeight);
		}

		mIsCreated = true;

		std::random_device rd;
		RandomGenerator generator(rd());

		split(generator);
		mRoot.makeRoom(mSizeMid, mSizeRange, generator);
		mRoot.merge(mComplexity, generator);
		mRoot.fillData(mWidth, mHeight, mData);
	}

	TileType getData(int x, int y) const { return mData[x + y*mWidth]; }

	int getWidth() const { return mWidth; }
	int getHeight() const { return mHeight; }

	int getComplexity() const { return mComplexity; }

	void setWidth(int width) 
	{
		mWidth = width;
		mRoot.reset(0, 0, mWidth, mHeight);
		mData.resize(mWidth*mHeight, TileType::Wall);
	}

	void setHeight(int height)
	{
		mHeight = height;
		mRoot.reset(0, 0, mWidth, mHeight);
		mData.resize(mWidth*mHeight, TileType::Wall);
	}

	void setSplitNum(int splitNum) { mSplitNum = splitNum; }
	void setSplitRange(float range) { mSplitRange = range; }
	void setSizeMid(float sizeMid) { mSizeMid = sizeMid; }
	void setSizeRange(float sizeRange) { mSizeRange = sizeRange; }

	void setComplexity(int complexity) { mComplexity = complexity; }

	void toTextFile(const std::string& path, std::function<char(TileType)> outputFunc) const;

private:

	//Ʈ���� �����ؼ� ���� ���� ���� ���� �����Ѵ�.
	template<typename RandomGenerator>
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

	int mWidth = 100;
	int mHeight = 100;
	int mSplitNum = 6;
	int mComplexity = 1;
	float mSplitRange = 0.2f;
	float mSizeMid = 0.6f;
	float mSizeRange = 0.2f;
	Leaf mRoot;
	bool mIsCreated = false;
	std::vector<TileType> mData;
};

}