#include <iostream>

#include "decs/decs.h"
#include "decs/ComponentContainers/StableContainer.h"


template<typename T>
class StableComponent
{

};

template<typename T>
struct is_stable_component
{
	static constexpr bool value = false;
};

template<typename T>
struct is_stable_component<StableComponent<T>>
{
	static constexpr bool value = true;
};




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
	std::cout << decs::is_stable_component<decs::Stable<float>>::value << "\n";
	std::cout << decs::is_stable_component<Position>::value << "\n";

	decs::Container container;

	auto e = container.CreateEntity();
	e.AddComponent<Position>(1.f, 2.f);

	decs::View<Position> view = { container };

	auto lambda = [] (Position& pos)
	{
		std::cout << "Fuck \n";
	};

	view.ForEach(lambda);

	return 0;
}