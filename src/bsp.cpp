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

void pmg::Leaf::fillData(int width, std::vector<int>& data) const
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

bool pmg::Leaf::getXRange(int leftIdx, int rightIdx, 
	const std::vector<Rectangle>& leftCand, const std::vector<Rectangle>& rightCand, 
	OUT int & start, OUT int & end) const
{
	const Rectangle& left = leftCand.at(leftIdx);
	const Rectangle& right = rightCand.at(rightIdx);

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
		}

		for (int r = rightIdx; r >= 0; r--)
		{
			if (rightCand[r].mX + rightCand[r].mWidth < left.mX + left.mWidth - 2)
			{
				xStart = std::max(xStart, rightCand[r].mX + rightCand[r].mWidth + 3);
				break;
			}
		}

		if (xStart > left.mX + left.mWidth - 2 ||
			right.mX + 1 > xEnd)
		{
			return false;
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

			for (int r = rightIdx; r < static_cast<int>(rightCand.size()); r++)
			{
				if (rightCand[r].mX > left.mX + 2)
				{
					xEnd = std::min(xEnd, rightCand[r].mX - 2);
					break;
				}
			}
		}

		if (xStart > right.mX + right.mWidth - 2 ||
			left.mX + 1 > xEnd)
		{
			return false;
		}
	}

	start = xStart;
	end = xEnd;

	return true;
}

bool pmg::Leaf::getYRange(int leftIdx, int rightIdx, 
	const std::vector<Rectangle>& leftCand, const std::vector<Rectangle>& rightCand, 
	OUT int & start, OUT int & end) const
{
	const Rectangle& left = leftCand.at(leftIdx);
	const Rectangle& right = rightCand.at(rightIdx);

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
	}

	start = yStart;
	end = yEnd;

	return true;
}

void pmg::Leaf::getSideRoom(SideType type, OUT std::vector<Rectangle>& rooms) const
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

bool pmg::Rectangle::isConnect(const Rectangle & other) const
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
