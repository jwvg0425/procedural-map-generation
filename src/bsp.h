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
	Room,
	Door
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

	bool isConnect(const Rectangle& other) const;

	bool isContain(const Point& pos) const;
};

struct Room : Rectangle
{
	Room() : Rectangle(), mIsVisited(false)
	{
	}

	Room(int x, int y, int width, int height)
		: Rectangle(x, y, width, height), mIsVisited(false)
	{
	}

	void fillData(int width, std::vector<int>& data);

	bool isOnLine(int pos, SideType type) const;

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

	//0.5 +- splitRange �������� �ش� ������ �� �κ����� ������.
	template<typename RandomGenerator>
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

	//���� ��忡 ���� ������ش�. ���� ��� ���� / �ʺ��� sizeMid +- sizeDist ũ�⿡�� ����. 
	template<typename RandomGenerator>
	void makeRoom(float sizeMid, float sizeRange, RandomGenerator& generator)
	{
		if (sizeMid - sizeRange < 0.0f || sizeMid + sizeRange > 1.0f)
		{
			return;
		}

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

		std::uniform_int_distribution<int> xDist(mInfo.mX, mInfo.mX + mInfo.mWidth - roomWidth);
		std::uniform_int_distribution<int> yDist(mInfo.mY, mInfo.mY + mInfo.mHeight - roomHeight);

		mRoom.mX = xDist(generator);
		mRoom.mY = yDist(generator);
		mRoom.mWidth = roomWidth;
		mRoom.mHeight = roomHeight;
	}


	//���ҵ� �� ������ ���� ��� �����ؼ� �ϳ��� ������ �����.
	template<typename RandomGenerator>
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

		connect(leftCand, rightCand, generator);
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

	void fillData(int width, std::vector<int>& data);

private:
	void getSideRoom(SideType type, OUT std::vector<Room*>& rooms);

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
	void connect(const std::vector<Room*>& leftCand, const std::vector<Room*>& rightCand, 
		RandomGenerator& generator)
	{
		using BlockPair = std::pair<int, int>;
		std::vector<BlockPair> blocks;
		int nowl = 0, nowr = 0;
		int midline;

		if (mIsWidthSplit)
			midline = mInfo.mWidth;
		else
			midline = mInfo.mHeight;

		while (nowl < static_cast<int>(leftCand.size()) && 
			   nowr < static_cast<int>(rightCand.size()))
		{
			Room& left = *leftCand[nowl];
			Room& right = *rightCand[nowr];

			if (mIsWidthSplit)
			{
				if (left.mX + left.mWidth == midline && right.mX == midline)
				{
					if ((left.mY >= right.mY && left.mY <= right.mY + right.mHeight) ||
						(right.mY >= left.mY && right.mY <= left.mY + left.mHeight))
					{
						blocks.push_back(std::make_pair(nowl, nowr));
					}
				}

				if (right.mY + right.mHeight > left.mY + left.mHeight)
				{
					nowl++;
				}
				else
				{
					nowr++;
				}
			}
			else
			{
				if (left.mY + left.mHeight == midline && right.mY == midline)
				{
					if ((left.mX >= right.mX && left.mX <= right.mX + right.mWidth) ||
						(right.mX >= left.mX && right.mX <= left.mX + left.mWidth))
					{
						blocks.push_back(std::make_pair(nowl, nowr));
					}
				}

				if (right.mX + right.mWidth > left.mX + left.mWidth)
				{
					nowl++;
				}
				else
				{
					nowr++;
				}
			}
		}

		//leftCand �߿� �� �ϳ� ��� ���⸦ ���� ������ ����.
		std::uniform_int_distribution<int> beginDist(0, leftCand.size() - 1);
		int beginRoomIdx = beginDist(generator);
		auto& begin = *leftCand[beginRoomIdx];
		int firstBlock = 0, lastBlock = rightCand.size() - 1;

		for (int i = 0; i < static_cast<int>(blocks.size()); i++)
		{
			if (blocks[i].first <= beginRoomIdx)
			{
				if (blocks[i].second > firstBlock)
				{
					firstBlock = blocks[i].second;
				}
			}

			if (blocks[i].first >= beginRoomIdx)
			{
				if (blocks[i].second < lastBlock)
				{
					lastBlock = blocks[i].second;
				}
			}
		}

		std::uniform_int_distribution<int> endDist(firstBlock, lastBlock);
		int endRoomIdx = endDist(generator);
		auto& end = *rightCand[endRoomIdx];

		//begin�� end ��ġ�� ����. �� ��ġ�� ���� ���� �� ���� �����Ѵ�.
		auto beginDoor = getDoor(beginRoomIdx, true, leftCand, rightCand, generator);
		auto endDoor = getDoor(endRoomIdx, false, rightCand, leftCand, generator);

		begin.mDoors.push_back(beginDoor);
		end.mDoors.push_back(endDoor);

		Point beginHall, endHall;

		int dx, dy;

		if (mIsWidthSplit)
		{
			if (beginDoor.mY == begin.mY)
			{
				dx = 0; dy = -1;
			}
			else if (beginDoor.mX == begin.mX + begin.mWidth - 1)
			{
				dx = 1; dy = 0;
			}
			else
			{
				dx = 0; dy = 1;
			}
			beginHall.mX = beginDoor.mX + dx;
			beginHall.mY = beginDoor.mY + dy;

			if (endDoor.mY == end.mY)
			{
				dx = 0; dy = -1;
			}
			else if (endDoor.mX == end.mX)
			{
				dx = -1; dy = 0;
			}
			else
			{
				dx = 0; dy = 1;
			}
			endHall.mX = endDoor.mX + dx;
			endHall.mY = endDoor.mY + dy;
		}
		else
		{
			if (beginDoor.mX == begin.mX)
			{
				dx = -1; dy = 0;
			}
			else if (beginDoor.mY == begin.mY + begin.mHeight - 1)
			{
				dx = 0; dy = 1;
			}
			else
			{
				dx = 1; dy = 0;
			}
			beginHall.mX = beginDoor.mX + dx;
			beginHall.mY = beginDoor.mY + dy;

			if (endDoor.mX == end.mX)
			{
				dx = -1; dy = 0;
			}
			else if (endDoor.mY == end.mY)
			{
				dx = 0; dy = -1;
			}
			else
			{
				dx = 1; dy = 0;
			}
			endHall.mX = endDoor.mX + dx;
			endHall.mY = endDoor.mY + dy;
		}

		while (beginHall.mX != endHall.mX || beginHall.mY != endHall.mY)
		{
			mHallways.push_back(beginHall);

			int dx, dy;
			if (mIsWidthSplit)
			{
				if (beginHall.mY < endHall.mY)
				{
					dx = 0;
					dy = 1;
				}
				else if (beginHall.mY == endHall.mY)
				{
					dx = 1;
					dy = 0;
				}
				else
				{
					dx = 0;
					dy = -1;
				}

				if (std::any_of(leftCand.begin(), leftCand.end(), [&beginHall,dx,dy](Room* room)
				{
					return room->isContain(Point(beginHall.mX + dx, beginHall.mY + dy));
				}))
				{
					dx = 1;
					dy = 0;
				}
				else if (std::any_of(rightCand.begin(), rightCand.end(), [&beginHall, dx, dy](Room* room)
				{
					return room->isContain(Point(beginHall.mX + dx, beginHall.mY + dy));
				}))
				{
					dx = -1;
					dy = 0;
				}
			}
			else
			{
				if (beginHall.mX < endHall.mX)
				{
					dx = 1;
					dy = 0;
				}
				else if (beginHall.mX == endHall.mX)
				{
					dx = 0;
					dy = 1;
				}
				else
				{
					dx = -1;
					dy = 0;
				}

				if (std::any_of(leftCand.begin(), leftCand.end(), [&beginHall, dx, dy](Room* room)
				{
					return room->isContain(Point(beginHall.mX + dx, beginHall.mY + dy));
				}))
				{
					dx = 0;
					dy = 1;
				}
				else if (std::any_of(rightCand.begin(), rightCand.end(), [&beginHall, dx, dy](Room* room)
				{
					return room->isContain(Point(beginHall.mX + dx, beginHall.mY + dy));
				}))
				{
					dx = 0;
					dy = -1;
				}
			}

			beginHall.mX += dx;
			beginHall.mY += dy;
		}

		mHallways.push_back(endHall);
	}

	template<typename RandomGenerator>
	Point getDoor(int roomIdx, bool isBegin,
		const std::vector<Room*>& sideRooms, const std::vector<Room*>& otherSideRooms,
		RandomGenerator& generator)
	{
		std::vector<Point> cand;
		int beginLine, endLine;
		Room& room = *sideRooms[roomIdx];

		if (mIsWidthSplit)
		{
			if (roomIdx == 0)
				beginLine = mInfo.mY;
			else
				beginLine = sideRooms[roomIdx - 1]->mY + sideRooms[roomIdx - 1]->mHeight - 1;

			if (roomIdx == sideRooms.size() - 1)
				endLine = mInfo.mY + mInfo.mHeight - 1;
			else
				endLine = sideRooms[roomIdx + 1]->mY;

			if (room.mY > beginLine + 1)
			{
				for (int x = room.mX + 1; x < room.mX + room.mWidth - 1; x++)
				{
					if (std::any_of(room.mDoors.begin(), room.mDoors.end(), [&room, x](const Point& door)
					{
						return door.mY == room.mY && (door.mX - 1 == x || door.mX + 1 == x);
					}))
					{
						continue;
					}

					cand.emplace_back(x, room.mY);
				}
			}

			if (room.mY + room.mHeight < endLine)
			{
				for (int x = room.mX + 1; x < room.mX + room.mWidth - 1; x++)
				{
					if (std::any_of(room.mDoors.begin(), room.mDoors.end(), [&room, x](const Point& door)
					{
						return door.mY == room.mY + room.mHeight - 1 && (door.mX - 1 == x || door.mX + 1 == x);
					}))
					{
						continue;
					}

					cand.emplace_back(x, room.mY + room.mHeight - 1);
				}
			}
		}
		else
		{
			if (roomIdx == 0)
				beginLine = mInfo.mX;
			else
				beginLine = sideRooms[roomIdx - 1]->mX + sideRooms[roomIdx - 1]->mWidth - 1;

			if (roomIdx == sideRooms.size() - 1)
				endLine = mInfo.mX + mInfo.mWidth - 1;
			else
				endLine = sideRooms[roomIdx + 1]->mX;

			if (room.mX > beginLine + 1)
			{
				for (int y = room.mY + 1; y < room.mY + room.mHeight - 1; y++)
				{
					if (std::any_of(room.mDoors.begin(), room.mDoors.end(), [&room, y](const Point& door)
					{
						return door.mX == room.mX && (door.mY - 1 == y || door.mY + 1 == y);
					}))
					{
						continue;
					}

					cand.emplace_back(room.mX,y);
				}
			}

			if (room.mX + room.mWidth < endLine)
			{
				for (int y = room.mY + 1; y < room.mY + room.mHeight - 1; y++)
				{
					if (std::any_of(room.mDoors.begin(), room.mDoors.end(), [&room, y](const Point& door)
					{
						return door.mX == room.mX + room.mWidth - 1 && (door.mY - 1 == y || door.mY + 1 == y);
					}))
					{
						continue;
					}

					cand.emplace_back(room.mX + room.mWidth - 1, y);
				}
			}
		}

		if (mIsWidthSplit)
		{
			if (isBegin)
			{
				for (int y = room.mY + 1; y < room.mY + room.mHeight - 1; y++)
				{
					if (std::any_of(otherSideRooms.begin(), otherSideRooms.end(), [&room, y](Room* r)
					{
						return r->mX == room.mX + room.mWidth && y >= r->mY && y < r->mY + r->mHeight;
					}))
					{
						continue;
					}

					if (std::any_of(room.mDoors.begin(), room.mDoors.end(), [&room, y](const Point& door)
					{
						return door.mX == room.mX + room.mWidth - 1 && (door.mY - 1 == y || door.mY + 1 == y);
					}))
					{
						continue;
					}

					cand.emplace_back(room.mX + room.mWidth - 1, y);
				}
			}
			else
			{
				for (int y = room.mY + 1; y < room.mY + room.mHeight - 1; y++)
				{
					if (std::any_of(otherSideRooms.begin(), otherSideRooms.end(), [&room, y](Room* r)
					{
						return room.mX == r->mX + r->mWidth && y >= r->mY && y < r->mY + r->mHeight;
					}))
					{
						continue;
					}

					if (std::any_of(room.mDoors.begin(), room.mDoors.end(), [&room, y](const Point& door)
					{
						return door.mX == room.mX && (door.mY - 1 == y || door.mY + 1 == y);
					}))
					{
						continue;
					}

					cand.emplace_back(room.mX, y);
				}
			}
		}
		else
		{
			if (isBegin)
			{
				for (int x = room.mX + 1; x < room.mX + room.mWidth - 1; x++)
				{
					if (std::any_of(otherSideRooms.begin(), otherSideRooms.end(), [&room, x](Room* r)
					{
						return r->mY == room.mY + room.mHeight && x >= r->mX && x < r->mX + r->mWidth;
					}))
					{
						continue;
					}

					if (std::any_of(room.mDoors.begin(), room.mDoors.end(), [&room, x](const Point& door)
					{
						return door.mY == room.mY + room.mHeight - 1 && (door.mX - 1 == x || door.mX + 1 == x);
					}))
					{
						continue;
					}

					cand.emplace_back(x, room.mY + room.mHeight - 1);
				}
			}
			else
			{
				for (int x = room.mX + 1; x < room.mX + room.mWidth - 1; x++)
				{
					if (std::any_of(otherSideRooms.begin(), otherSideRooms.end(), [&room, x](Room* r)
					{
						return room.mY == r->mY + r->mHeight && x >= r->mX && x < r->mX + r->mWidth;
					}))
					{
						continue;
					}

					if (std::any_of(room.mDoors.begin(), room.mDoors.end(), [&room, x](const Point& door)
					{
						return door.mY == room.mY && (door.mX - 1 == x || door.mX + 1 == x);
					}))
					{
						continue;
					}

					cand.emplace_back(x, room.mY);
				}
			}
		}

		std::uniform_int_distribution<int> candDist(0, cand.size() - 1);
		Point door = cand[candDist(generator)];

		return door;
	}

	const int LEAF_MINIMUM_SIZE = 10;
	const int ROOM_MINIMUM_SIZE = 3;

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
		mData.resize(mWidth * mHeight, static_cast<int>(TileType::Wall));
	}

	BSP(int width, int height, int splitNum, float splitRange, float sizeMid, float sizeRange)
		: mWidth(width), mHeight(height), 
		mSplitNum(splitNum), mSplitRange(splitRange), 
		mSizeMid(sizeMid), mSizeRange(sizeRange),
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
		mRoot.makeRoom(mSizeMid, mSizeRange, generator);
		mRoot.merge(generator);
		mRoot.fillData(mWidth, mData);
	}

	void toTextFile(const std::string& path, std::function<char(int)> outputFunc) const;

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
	float mSplitRange = 0.2f;
	float mSizeMid = 0.6f;
	float mSizeRange = 0.2f;
	Leaf mRoot;
	std::vector<int> mData;
};

}