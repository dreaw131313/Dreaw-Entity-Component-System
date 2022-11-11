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

	entity.Destroy();

	decs::View<C1> view = {};
	view.Fetch(container);
	view.ForEach([](C1& c)
	{
		c.X += 1;
	});

	entity.AddComponent<float>(1.f);
	entity.RemoveComponent<int>();
	entity.HasComponent<int>();


	if (entity.IsActiveSafe())
	{
		
	}

	std::cin.get();
	return 0;
}