#include <iostream>
#include <format>

#include "decs/decs.h"

void PrintLine(std::string message = "")
{
	std::cout << message << "\n";
}

struct Position : public decs::StableComponent
{
public:
	float X = 0;
	float Y = 0;

public:
	Position()
	{

	}

	Position(float x, float y) : X(x), Y(y)
	{

	}

	void TestFunc(int& i)
	{
		PrintLine("Is working");
		//i += 1;
	}
};

struct TestComponent : public decs::Component
{
public:
	int table[10];

};

struct Renderer :public decs::StableComponent
{
public:
	double mesh;
};


int main()
{
	decs::Container container = {};

	auto entity = container.CreateEntity();

	auto pos = entity.AddComponent<Position>();
	auto test = entity.AddComponent<TestComponent>();
	auto rend = entity.AddComponent<Renderer>();

	Position* position = entity.GetComponent<Position>();
	TestComponent* testComp = entity.GetComponent<TestComponent>();
	Renderer* renderer = entity.GetComponentDynamic<Renderer>();

	//container.Spawn(entity, 10, true);

	decs::Query<Position> testQuery = { &container };

	testQuery.ForEach([&](Position& pos)
	{
		PrintLine("ForEach");
	});
	testQuery.ForEachBackward([&](decs::Entity& e, Position& pos)
	{
		PrintLine("ForEachBackward");
	});
	testQuery.ForEachSafe([&](Position& pos)
	{
		PrintLine("ForEachSafe");
	});

	return 0;
}