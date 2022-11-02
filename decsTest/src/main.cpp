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
	entity.AddComponent<C1>(1.f, 2.f);

	return 0;
}