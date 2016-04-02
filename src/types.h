#pragma once

namespace pmg
{

enum class Direction
{
	Top = 0,
	Right = 1,
	Bottom = 2,
	Left = 3
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

}