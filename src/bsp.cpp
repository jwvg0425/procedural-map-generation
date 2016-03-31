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

void pmg::Leaf::getAllRooms(OUT std::vector<Rectangle>& rooms)
{
	if (!hasChild())
	{
		rooms.emplace_back(mRoom.mX, mRoom.mY, mRoom.mWidth, mRoom.mHeight);
	}

	if (mLeftChild != nullptr)
		mLeftChild->getAllRooms(rooms);

	if (mRightChild != nullptr)
		mRightChild->getAllRooms(rooms);
}

void pmg::Leaf::getAllHallways(OUT std::vector<Point>& hallways)
{
	if (mLeftChild != nullptr)
		mLeftChild->getAllHallways(hallways);

	if (mRightChild != nullptr)
		mRightChild->getAllHallways(hallways);

	for (auto& h : mHallways)
	{
		hallways.push_back(h);
	}
}

bool pmg::Rectangle::isConnect(const Rectangle & other) const
{
	if (mX == other.getRight() + 1 ||
		other.mX == getRight() + 1)
	{
		return (mY >= other.mY && mY < other.getBottom() - 1) ||
			(other.mY >= mY && other.mY < getBottom() - 1);
	}

	if (mY == other.mY + other.mHeight ||
		other.mY == mY + mHeight)
	{
		return (mX >= other.mX && mX < other.getRight() - 1) ||
			(other.mX >= mX && other.mX < getRight() - 1);
	}

	return false;
}

bool pmg::Rectangle::isContain(const Point & pos) const
{
	return pos.mX >= mX && pos.mX <= getRight() &&
		pos.mY >= mY && pos.mY <= getBottom();
}

bool pmg::Rectangle::isOverlap(const Rectangle & other) const
{
	return !(getRight() < other.mX || mX > other.getRight() ||
		getBottom() < other.mY || mY > other.getBottom());
}

void pmg::Room::fillData(int width, std::vector<int>& data)
{
	if (mIsVisited)
		return;
	
	std::vector<Room*> allRooms = getAllRooms();

	for (auto r : allRooms)
	{
		const auto& room = *r;

		for (int y = room.mY; y < room.getBottom() + 1; y++)
		{
			for (int x = room.mX; x < room.getRight() + 1; x++)
			{
				TileType type;

				if (x == room.mX)
				{
					if (room.isOnLine(y, SideType::Left))
					{
						type = TileType::Room;
					}
					else
					{
						type = TileType::Wall;
					}
				}
				else if (x == room.getRight())
				{
					if (room.isOnLine(y, SideType::Right))
					{
						type = TileType::Room;
					}
					else
					{
						type = TileType::Wall;
					}
				}
				else if (y == room.mY)
				{
					if (room.isOnLine(x, SideType::Top))
					{
						type = TileType::Room;
					}
					else
					{
						type = TileType::Wall;
					}
				}
				else if (y == room.getBottom())
				{
					if (room.isOnLine(x, SideType::Bottom))
					{
						type = TileType::Room;
					}
					else
					{
						type = TileType::Wall;
					}
				}
				else
				{
					type = TileType::Room;
				}

				for (auto& d : room.mDoors)
				{
					if (x == d.mX && y == d.mY)
					{
						type = TileType::Door;
						break;
					}
				}

				data[x + y * width] = static_cast<int>(type);
			}
		}
	}
}

bool pmg::Room::isOnLine(int pos, SideType type) const
{
	switch (type)
	{
	case SideType::Left:
	case SideType::Right:
		if (pos == mY || pos == getBottom())
			return false;
	case SideType::Top:
	case SideType::Bottom:
		if (pos == mX || pos == getRight())
			return false;
	}

	for (auto r : mConnectedRooms)
	{
		const auto& room = *r;
		int first, last;

		switch (type)
		{
		case SideType::Left:
			if (room.getRight() + 1 != mX)
			{
				continue;
			}
			first = room.mY + 1;
			last = room.getBottom() - 1;
			break;
		case SideType::Right:
			if (getRight() + 1!= room.mX)
			{
				continue;
			}
			first = room.mY + 1;
			last = room.getBottom() - 1;
			break;
		case SideType::Top:
			if (room.getBottom() + 1 != mY)
			{
				continue;
			}
			first = room.mX + 1;
			last = room.getRight() - 1;
			break;
		case SideType::Bottom:
			if (getBottom() + 1 != room.mY)
			{
				continue;
			}
			first = room.mX + 1;
			last = room.getRight() - 1;
			break;
		}

		if (pos >= first && pos <= last)
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
