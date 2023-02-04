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
	std::cout << "A: " << decs::Type<A>::ID() << "\n";
	std::cout << "B: " << decs::Type<B>::ID() << "\n";
	std::cout << "C: " << decs::Type<C>::ID() << "\n";
	std::cout << "D: " << decs::Type<D>::ID() << "\n";
	std::cout << "E: " << decs::Type<E>::ID() << "\n";
	std::cout << "F: " << decs::Type<F>::ID() << "\n";
	std::cout << "G: " << decs::Type<G>::ID() << "\n";
	std::cout << "H: " << decs::Type<H>::ID() << "\n";

	decs::Container container = {};

	auto entity1 = container.CreateEntity();
	entity1.AddComponent<A>();
	entity1.AddComponent<B>();
	entity1.AddComponent<C>();
	entity1.AddComponent<D>();
	entity1.AddComponent<E>();

	auto entity2 = container.CreateEntity();
	entity2.AddComponent<G>();
	entity2.AddComponent<H>();
	entity2.AddComponent<A>();
	entity2.AddComponent<B>();
	entity2.AddComponent<C>();
	entity2.AddComponent<D>();
	entity2.AddComponent<E>();


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