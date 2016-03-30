#include "bsp.h"
#include <fstream>

void pmg::BSP::toTextFile(const std::string& path, std::function<char(int)> outputFunc) const
{
	std::ofstream stream(path);

	if (!stream.is_open())
		return;

	for (int y = 0; y < mHeight; y++)
	{
		for (int x = 0; x < mWidth; x++)
		{
			stream << outputFunc(mData[y*mWidth + x]);
		}

		stream << std::endl;
	}

	stream.close();
}

void pmg::Leaf::fillData(int width, std::vector<int>& data)
{
	if (mLeftChild != nullptr)
		mLeftChild->fillData(width, data);

	if (mRightChild != nullptr)
		mRightChild->fillData(width, data);

	for (auto& p : mHallways)
	{
		data[p.mX + p.mY * width] = static_cast<int>(TileType::Hall);
	}

	if (!hasChild())
	{
		mRoom.fillData(width, data);
	}
}

void pmg::Leaf::getSideRoom(SideType type, OUT std::vector<Room*>& rooms)
{
	if (!hasChild())
	{
		rooms.push_back(&mRoom);
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

bool pmg::Rectangle::isConnect(const Rectangle & other) const
{
	if (mX == other.mX + other.mWidth ||
		other.mX == mX + mWidth)
	{
		return (mY >= other.mY && mY < other.mY + other.mHeight - 2) ||
			(other.mY >= mY && other.mY < mY + mHeight - 2);
	}

	if (mY == other.mY + other.mHeight ||
		other.mY == mY + mHeight)
	{
		return (mX >= other.mX && mX < other.mX + other.mWidth - 2) ||
			(other.mX >= mX && other.mX < mX + mHeight - 2);
	}

	return false;
}

bool pmg::Rectangle::isContain(const Point & pos) const
{
	return pos.mX >= mX && pos.mX < mX + mWidth &&
		pos.mY >= mY && pos.mY < mY + mHeight;
}

void pmg::Room::fillData(int width, std::vector<int>& data)
{
	if (mIsVisited)
		return;
	
	mIsVisited = true;
	std::vector<Room*> allRooms = getAllRooms();

	for (auto r : allRooms)
	{
		const auto& room = *r;

		for (int y = room.mY; y < room.mY + room.mHeight; y++)
		{
			for (int x = room.mX; x < room.mX + room.mWidth; x++)
			{
				if (x == room.mX)
				{
					if (room.isOnLine(y, SideType::Left))
					{
						data[x + y * width] = static_cast<int>(TileType::Room);
					}
					else
					{
						data[x + y * width] = static_cast<int>(TileType::Wall);
					}
				}
				else if (x == room.mX + room.mWidth - 1)
				{
					if (room.isOnLine(y, SideType::Right))
					{
						data[x + y * width] = static_cast<int>(TileType::Room);
					}
					else
					{
						data[x + y * width] = static_cast<int>(TileType::Wall);
					}
				}
				else if (y == room.mY)
				{
					if (room.isOnLine(x, SideType::Top))
					{
						data[x + y * width] = static_cast<int>(TileType::Room);
					}
					else
					{
						data[x + y * width] = static_cast<int>(TileType::Wall);
					}
				}
				else if (y == room.mY + room.mHeight - 1)
				{
					if (room.isOnLine(x, SideType::Bottom))
					{
						data[x + y * width] = static_cast<int>(TileType::Room);
					}
					else
					{
						data[x + y * width] = static_cast<int>(TileType::Wall);
					}
				}
				else
				{
					data[x + y * width] = static_cast<int>(TileType::Room);
				}
			}
		}

		for (auto& d : room.mDoors)
		{
			data[d.mX + d.mY*width] = static_cast<int>(TileType::Door);
		}
	}
}

bool pmg::Room::isOnLine(int pos, SideType type) const
{
	switch (type)
	{
	case SideType::Left:
	case SideType::Right:
		if (pos == mY || pos == mY + mHeight - 1)
			return false;
	case SideType::Top:
	case SideType::Bottom:
		if (pos == mX || pos == mX + mWidth - 1)
			return false;
	}

	for (auto r : mConnectedRooms)
	{
		const auto& room = *r;
		int first, last;

		switch (type)
		{
		case SideType::Left:
			if (room.mX + room.mWidth != mX)
			{
				continue;
			}
			first = room.mY + 1;
			last = room.mY + room.mHeight - 1;
			break;
		case SideType::Right:
			if (mX + mWidth != room.mX)
			{
				continue;
			}
			first = room.mY + 1;
			last = room.mY + room.mHeight - 1;
			break;
		case SideType::Top:
			if (room.mY + room.mHeight != mY)
			{
				continue;
			}
			first = room.mX + 1;
			last = room.mX + room.mWidth - 1;
			break;
		case SideType::Bottom:
			if (mY + mHeight != room.mY)
			{
				continue;
			}
			first = room.mX + 1;
			last = room.mX + room.mWidth - 1;
			break;
		}

		if (pos >= first && pos < last)
			return true;
	}

	return false;
}

std::vector<pmg::Room*> pmg::Room::getAllRooms()
{
	std::vector<Room*> rooms;
	std::queue<Room*> queue;

	queue.push(this);

	while (!queue.empty())
	{
		auto now = queue.front();
		queue.pop();

		now->mIsVisited = true;

		rooms.push_back(now);

		for (auto adj : now->mConnectedRooms)
		{
			if (adj->mIsVisited)
				continue;

			queue.push(adj);
		}
	}

	return rooms;
}
