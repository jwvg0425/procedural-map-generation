#include "bsp.h"
#include <fstream>

void pmg::BSP::toTextFile(const std::string& path, std::function<char(TileType)> outputFunc) const
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

void pmg::Leaf::fillData(int width, int height, std::vector<TileType>& data)
{
	if (mLeftChild != nullptr)
		mLeftChild->fillData(width, height, data);

	if (mRightChild != nullptr)
		mRightChild->fillData(width, height, data);

	for (auto& p : mHallways)
	{
		data[p.mX + p.mY * width] = TileType::Hall;
	}

	if (!hasChild())
	{
		mRoom.fillData(width, height, data);
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

pmg::Point pmg::Leaf::getDoorNextPos(const Point & door, const Room & room)
{
	int dx, dy;
	Point nextPos;

	if (door.mY == room.mY)
	{
		dx = 0; dy = -1;
	}
	else if (door.mX == room.getRight())
	{
		dx = 1; dy = 0;
	}
	else if (door.mY == room.getBottom())
	{
		dx = 0; dy = 1;
	}
	else // door.mX == room.mX
	{
		dx = -1; dy = 0;
	}

	nextPos.mX = door.mX + dx;
	nextPos.mY = door.mY + dy;

	return nextPos;

}

bool pmg::Leaf::isConnect(Point begin, Point end,
	const std::vector<Point>& hallways, std::vector<Point>& visited)
{
	if (begin == end)
		return true;

	visited.push_back(begin);

	std::vector<Point> cand;

	cand.emplace_back(begin.mX - 1, begin.mY);
	cand.emplace_back(begin.mX, begin.mY - 1);
	cand.emplace_back(begin.mX + 1, begin.mY);
	cand.emplace_back(begin.mX, begin.mY + 1);


	for (auto& c : cand)
	{
		if (!isContainPoint(visited, c) && isContainPoint(hallways, c))
		{
			if (isConnect(c, end, hallways, visited))
				return true;
		}
	}

	return false;
}
bool pmg::Leaf::isValidHallpos(const Point & pos, SideType side, Rectangle area,
	const std::vector<Rectangle>& rooms, const std::vector<Point>& otherHall, 
	const std::vector<Point>& visited)
{
	int dx, dy;

	switch (side)
	{
	case SideType::Top:
	case SideType::Bottom:
		dx = 1;
		dy = 0;
		break;
	case SideType::Left:
	case SideType::Right:
		dx = 0;
		dy = 1;
		break;
	}

	//방과 겹치거나 방문했던 곳 또 방문하면 안 되므로 이 두경우를 먼저 체크해서 제외한다.
	if (isContainPoint(rooms, pos) || isContainPoint(visited, pos))
		return false;

	//이미 존재하는 복도 위에 있는 경우 진행한다(해당 복도와 연결되는 경우)
	if (isContainPoint(otherHall, pos))
		return true;

	//그 외의 경우 주어진 범위 내에 있어야하며, 진행하는 방향 양쪽에 복도가 없어야한다(두께를 1로 유지하기 위해)
	if (area.isContain(pos) &&
		!isContainPoint(otherHall, pos.mX + dx, pos.mY + dy) &&
		!isContainPoint(otherHall, pos.mX - dx, pos.mY - dy))
	{
		return true;
	}

	return false;
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

void pmg::Room::fillData(int width, int height, std::vector<TileType>& data)
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
				if (isWallPos(x, y, width, height, allRooms))
				{
					data[x + y*width] = TileType::Wall;
				}
				else
				{
					data[x + y * width] = TileType::Room;
				}
			}
		}

		for (auto& door : room.mDoors)
		{
			data[door.mX + door.mY * width] = TileType::Door;
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

bool pmg::Room::isWallPos(int x, int y, int width, int height, const std::vector<Room*>& rooms)
{
	Point adjs[8] = 
	{ 
		{ x + 1, y },
		{ x + 1, y + 1 },
		{ x, y + 1 },
		{ x - 1, y + 1 },
		{ x - 1, y },
		{ x - 1, y - 1 },
		{ x, y - 1 },
		{ x + 1,y - 1 } 
	};

	for (int i = 0; i < 8; i++)
	{
		const Point& adj = adjs[i];
		if (std::all_of(rooms.begin(), rooms.end(), [&adj](const Room* room)
		{
			return !room->isContain(adj);
		}))
		{
			return true;
		}
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
