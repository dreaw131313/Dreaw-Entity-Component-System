#include <iostream>
#include <format>

#include "decs/decs.h"

void PrintLine(std::string message = "")
{
	std::cout << message << "\n";
}


struct Position : public decs::ComponentBase
{
public:
	float X = 0;
	float Y = 0;

public:
	Position()
	{
		PrintLine("Position constructior");
	}

	Position(float x, float y): X(x), Y(y)
	{
		PrintLine("Position constructior");
	}

	void TestFunc(int& i)
	{
		PrintLine("Is working");
		//i += 1;
	}

protected:
	void OnPreCreate(const decs::Entity& entity) override final
	{
		PrintLine(std::format("Position X: {0}, Y: {1}", X, Y));
	}
};

struct TestComponent : public decs::ComponentBase
{
public:
	int table[10];

	TestComponent() = default;

};

struct Renderer :public decs::ComponentBase
{
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
	void OnEnableComponent(TestComponent& component, const decs::Entity& entity) override
	{
		PrintLine("TestComponent on enable");
	}


	// Inherited via DisableComponentObserver
	void OnDisableComponent(TestComponent& component, const decs::Entity& entity) override
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

	// TAG TEST:
	{
		using FloatTag = decs::tag<float>;
		using IntTag = decs::tag<int>;
		using DoubleTag = decs::tag<double>;
		using BoolTag = decs::tag<bool>;
	
		auto prefabEntity = container.CreateEntity();

		prefabEntity.AddComponent<Position>();

		prefabEntity.AddTag<FloatTag>();
		prefabEntity.AddTag<IntTag>();
		prefabEntity.AddTag<DoubleTag>();
		prefabEntity.AddTag<BoolTag>();

		prefabEntity.AddComponent<TestComponent>();

		auto spawnedEntity = container.Spawn(prefabEntity);

		if (spawnedEntity.HasTag<FloatTag>())
		{
			PrintLine("Spawned has FloatTag");
		}
		if (spawnedEntity.HasTag<IntTag>())
		{
			PrintLine("Spawned has IntTag");
		}
		if (spawnedEntity.HasTag<DoubleTag>())
		{
			PrintLine("Spawned has DoubleTag");
		}
		if (spawnedEntity.HasTag<BoolTag>())
		{
			PrintLine("Spawned has BoolTag");
		}
		if (spawnedEntity.HasComponent<Position>())
		{
			PrintLine("Spawned has Position");
		}
		if (spawnedEntity.HasTag<TestComponent>())
		{
			PrintLine("Spawned has TestComponent");
		}

		if (spawnedEntity.RemoveTag<DoubleTag>())
		{
			PrintLine("Spawned removed DoubleTag");
		}

		decs::Query<TestComponent> query{&container};
		query.With<DoubleTag>();

		PrintLine();
		query.ForEach([](const decs::ConstEntity& entity, const TestComponent& )
		{
			PrintLine(std::format("Entity: {0} TestComponent", entity.GetID()));
		});
		PrintLine();
	}

	return 0;
}
