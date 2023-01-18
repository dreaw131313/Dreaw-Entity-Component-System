#include <iostream>

#include "decs/decs.h"


class C1
{
public:
	float X;
	float Y;

public:
	C1(float x, float y) :X(x), Y(y)
	{

	}
};

int main()
{
	std::cout << "Hello world" << std::endl;

	decs::Container container = {};

	decs::Entity entity = container.CreateEntity();
	std::cout << container.GetEmptyEntitiesCount() << std::endl;

	entity.AddComponent<float>();
	entity.AddComponent<int>();
	std::cout << container.GetEmptyEntitiesCount() << std::endl;

	entity.RemoveComponent<int>();
	entity.RemoveComponent<float>();
	std::cout << container.GetEmptyEntitiesCount() << std::endl;

	std::cin.get();
	return 0;
}