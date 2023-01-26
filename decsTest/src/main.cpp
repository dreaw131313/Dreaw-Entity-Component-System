#include <iostream>

#include "decs/decs.h"

#include "decs/PackedComponentContainer.h"


void Print(const std::string& message)
{
	std::cout << message << "\n";
}

class Position
{
public:
	float X = 0, Y = 0;

public:
	Position()
	{
		//Print("Position Constructor");
	}

	Position(float x, float y) : X(x), Y(y)
	{
		//Print("Position Constructor");
	}

	~Position()
	{
		//Print("Position Destructor");
	}

};

int main()
{
	decs::Container container = { };

	for (int i = 0; i < 5; i++)
	{
		decs::Entity e = container.CreateEntity();
		e.AddComponent<Position>(i, 2 * i);
		e.AddComponent<int>(i);
	}

	decs::Entity entity = container.CreateEntity();
	
	decs::ComponentRef<float> compRef = { entity };

	if (compRef)
	{
		std::cout << "Float com exist = " << *compRef.Get() << "\n";
	}

	entity.AddComponent<float>(10);

	if (compRef)
	{
		std::cout << "Float com exist = " << *compRef.Get() << "\n";
	}

	using ViewType = decs::View<int, Position>;
	ViewType testView = { container };

	auto lambda = [&] (decs::Entity& e, int& i, Position& position)
	{
		std::cout << "Entity ID: " << e.ID() << ": int = " << i << std::endl;
	};
	auto lambda2 = [&] (int& i, Position& position)
	{
		std::cout << "int = " << i << std::endl;
	};

	testView.ForEachForward(lambda);
	testView.ForEachBackward(lambda);
	std::cout << std::endl;
	testView.ForEachBackward(lambda2);
	testView.ForEachForward(lambda2);

	std::vector< ViewType::BatchIterator> iterators;
	testView.CreateBatchIterators(iterators, 2, 3);

	for (auto& it : iterators)
	{
		it.ForEach(lambda);
	}
	std::cout << "\n";
	for (auto& it : iterators)
	{
		it.ForEach(lambda2);
	}

	container.DestroyOwnedEntities();

	return 0;
}