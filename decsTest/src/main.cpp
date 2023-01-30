#include <iostream>

#include "decs/decs.h"
#include "decs/ComponentContainers/StableContainer.h"


class Position
{
public:
	float X = 0, Y = 0;

	Position()
	{

	}

	Position(float x, float y) :X(x), Y(y)
	{

	}
};


int main()
{
	decs::StableContainer<Position> stableContainer(2);

	auto result1 = stableContainer.Emplace(1.f, 1.f);
	auto result2 = stableContainer.Emplace(1.f, 1.f);
	auto result3 = stableContainer.Emplace(1.f, 1.f);
	auto result4 = stableContainer.Emplace(1.f, 1.f);
	auto result5 = stableContainer.Emplace(1.f, 1.f);

	stableContainer.Remove(result3.m_ChunkIndex, result3.m_Index);
	stableContainer.Remove(result4.m_ChunkIndex, result4.m_Index);
	return 0;
}