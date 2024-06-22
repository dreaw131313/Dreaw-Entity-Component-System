#include <iostream>
#include <format>

#include "decs/decs.h"

void PrintLine(std::string message = "")
{
	std::cout << message << "\n";
}


struct Position : public decs::ComponentBase
{
	DECS_STABLE_COMPONENT()
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

struct TestComponent : public decs::ComponentBase
{
	DECS_COMPONENT()
public:
	int table[10];

};

struct Renderer :public decs::ComponentBase
{
	DECS_COMPONENT()
public:
	double mesh;
};


class TestComponetObserver :
	public decs::CreateComponentObserver<TestComponent>,
	public decs::DestroyComponentObserver<TestComponent>,
	public decs::EnableComponentObserver<TestComponent>,
	public decs::DisableComponentObserver<TestComponent>
{
public:

	// Inherited via CreateComponentObserver
	void OnCreateComponent(TestComponent& component, const decs::Entity& entity) override
	{
		PrintLine("TestComponent on create");
	}

	// Inherited via DestroyComponentObserver
	void OnDestroyComponent(TestComponent& component, const decs::Entity& entity) override
	{
		PrintLine("TestComponent on destroy");
	}


	// Inherited via EnableComponentObserver
	void OnEnableEntity(TestComponent& component, const decs::Entity& entity) override
	{
		PrintLine("TestComponent on enable");
	}


	// Inherited via DisableComponentObserver
	void OnDisableEntity(TestComponent& component, const decs::Entity& entity) override
	{
		PrintLine("TestComponent on disable");
	}
};

int main()
{
	TestComponetObserver testComponentObserver = {};

	decs::ObserversManager observerManager = {};
	{
		observerManager.SetComponentObservers(&testComponentObserver, &testComponentObserver, &testComponentObserver, &testComponentObserver);
	}

	decs::Container container = {};

	observerManager.FillContainerObservers(container);

	{
		auto entity = container.CreateEntity();

		auto pos = entity.AddComponent<Position>();
		auto test = entity.AddComponent<TestComponent>();
		auto rend = entity.AddComponent<Renderer>();

		Position* position = entity.GetComponent<Position>();
		TestComponent* testComp = entity.GetComponent<TestComponent>();
		Renderer* renderer = entity.GetComponentDynamic<Renderer>();

		container.Spawn(entity, 3, true);

		PrintLine();

		container.ForEach<Position>([](const decs::Entity& e, Position& p)
		{
			p.X += e.GetID();
			p.Y += 2 * e.GetID();
		});

		container.ForEach<Position>([](const decs::Entity& e, Position& p)
		{
			PrintLine(std::format("X: {0}, Y: {1}", p.X, p.Y));
		});

		decs::Query<Position> testQuery = { &container };

		testQuery.ForEach([&](Position& pos)
		{
			PrintLine("decs::Query::ForEach");
		});
		testQuery.ForEachBackward([&](decs::Entity& e, Position& pos)
		{
			PrintLine("decs::Query::ForEachBackward");
		});
		testQuery.ForEachSafe([&](Position& pos)
		{
			PrintLine("decs::Query::ForEachSafe");
		});

		decs::MultiQuery<TestComponent> testMultiQuery = {};
		testMultiQuery.AddContainer(&container);

		PrintLine();

		testMultiQuery.ForEach([&](TestComponent& pos)
		{
			PrintLine("decs::MultiQuery::ForEach");
		});
		testMultiQuery.ForEachBackward([&](decs::Entity& e, TestComponent& pos)
		{
			PrintLine("decs::MultiQuery::ForEachBackward");
		});
		testMultiQuery.ForEachSafe([&](TestComponent& pos)
		{
			PrintLine("decs::MultiQuery::ForEachSafe");
		});

		PrintLine();

		entity.Destroy();
	}
	

	// NO CALLBACK:
	//{
	//	auto entity_nc = container.CreateEntity();
	//
	//	entity_nc.AddComponent_NoCallback<TestComponent>();
	//
	//	container.InvokeEntitesOnCreateListeners();
	//	container.InvokeEntitesOnDestroyListeners();
	//}

	return 0;
}