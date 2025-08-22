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

		entity.AddComponent<Renderer>();
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
	using FloatTag = decs::tag<float>;
	using IntTag = decs::tag<int>;
	using DoubleTag = decs::tag<double>;
	using BoolTag = decs::tag<bool>;

	TestComponetObserver testComponentObserver = {};

	decs::ObserversManager observerManager = {};
	{
		observerManager.SetComponentObservers(&testComponentObserver, &testComponentObserver, &testComponentObserver, &testComponentObserver);
	}

	decs::Container container = {};
	observerManager.FillContainerObservers(container);

	{
		decs::Entity prefab = container.CreateEntity_NoCallbacks();
		prefab.AddComponent_NoCallback<TestComponent>();
		prefab.AddTag<FloatTag>();

		decs::Container secondContainer{};
		secondContainer.Spawn_NoCallback(prefab);
	}

	// TAG TEST:
	{
		PrintLine(decs::Type<FloatTag>::Name());
		PrintLine(decs::Type<std::vector<decs::Entity>>::Name());

		{
			auto prefabEntity1 = container.CreateEntity();

			prefabEntity1.AddComponent<Position>();

			prefabEntity1.AddTag<FloatTag>();
			prefabEntity1.AddComponent<TestComponent>();
			auto spawnedEntity = container.Spawn(prefabEntity1, true);

			if (spawnedEntity.HasTag<FloatTag>())
			{
				PrintLine("Spawned entity with tag!");
			}
			spawnedEntity.SetActive(false);
			spawnedEntity.SetActive(true);

		}


		uint32_t counter = 0;
		auto testFunc = [&](const decs::ConstEntity& entity, const TestComponent&)
		{
			PrintLine(std::format("Entity: {0} TestComponent", entity.GetID()));
		};

		decs::MultiQuery<TestComponent> query{};
		query.AddContainer(&container);


		std::vector<decs::MultiQuery<TestComponent> ::BatchIterator> iterators{};
		query.CreateBatchIteratorsWithMaxNumberPerBatch(iterators, 7);

		for (auto& it : iterators)
		{
			it.ForEach(testFunc);
		}


	}

	return 0;
}
