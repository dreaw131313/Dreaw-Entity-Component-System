#include <iostream>

#include "decs/decs.h"

#include "decs/PackedComponentContainer.h"


void Print(const std::string& message)
{
	// std::cout << message << std::endl;
}

class Position
{
public:
	float X = 0, Y = 0;

public:
	Position()
	{
		Print("Position Constructor");
	}

	Position(float x, float y) : X(x), Y(y)
	{
		Print("Position Constructor");
	}

	/*Position(const Position& other)
	{
		Print("Position Copy Constructor");
	}

	Position(Position&& other) noexcept
	{
		Print("Position Move Constructor");
	}

	~Position()
	{
		Print("Position Destructor");
	}

	Position& operator = (const Position& other)
	{
		Print("Position Copy Assigment");
		return *this;
	}

	Position& operator = (Position&& other) noexcept
	{
		Print("Position Move Assigment");
		return *this;
	}*/
};

int main()
{
	Print("Start:");

	decs::Container container = {};

	for (int i = 0; i < 5; i++)
	{
		decs::Entity e = container.CreateEntity();
		e.AddComponent<Position>(i, 2 * i);
		e.AddComponent<float>(i * 3);
		e.RemoveComponent<int>();
	}

	using ViewType = decs::View<Position, float>;
	ViewType testView = { container };

	auto lambda = [&] (decs::Entity& e, Position& position, float& f)
	{
		std::cout << "Entity ID: " << e.ID() << " - float = " << f << std::endl;
	};
	auto lambda2 = [&] (Position& position, float& f)
	{
		std::cout << "float = " << f << std::endl;
	};

	/*testView.ForEach(lambda, decs::IterationType::Forward);
	std::cout << std::endl;
	testView.ForEach(lambda2, decs::IterationType::Backward);*/

	std::vector< ViewType::BatchIterator> iterators;

	testView.CreateBatchIterators(iterators, 10, 1000);

	for (auto& it : iterators)
	{
		it.ForEach(lambda);
		it.ForEach(lambda2);
	}


	container.DestroyOwnedEntities();

	return 0;
}