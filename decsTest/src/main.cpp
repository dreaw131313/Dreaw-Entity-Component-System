#include <iostream>

#include "decs/decs.h"
#include "decs/ComponentContainers/StableContainer.h"

void Print(std::string message)
{
	std::cout << message << "\n";
}

class A
{
	float X = 0;
};
class B
{
	float X = 0;
};
class C
{
	float X = 0;
};
class D
{
	float X = 0;
};
class E
{
	float X = 0;
};
class F
{
	float X = 0;
};
class G
{
	float X = 0;
};
class H
{
	float X = 0;
};
class I
{
	float X = 0;
};


int main()
{
	decs::Container prefabContainer = {};
	decs::Container container = {};

	auto entity1 = prefabContainer.CreateEntity();
	entity1.AddComponent<A>();
	entity1.AddComponent<B>();
	entity1.AddComponent<C>();
	entity1.AddComponent<D>();
	entity1.AddComponent<E>();

	auto entity2 = container.Spawn(entity1, 3, true);


	using ViewType = decs::View<A, B, C, D, E>;
	ViewType view = { container };


	uint64_t iterationCount = 0;
	auto lambda = [&] (A& a, B& b, C& c, D& d, E& e)
	{
		iterationCount += 1;
	};

	view.ForEachForward(lambda);

	std::cout << "Iterated entites count: " << iterationCount << "\n";

	return 0;
}