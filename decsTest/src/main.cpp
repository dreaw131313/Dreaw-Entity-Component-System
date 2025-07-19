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
		//observerManager.SetComponentObservers(&testComponentObserver, &testComponentObserver, &testComponentObserver, &testComponentObserver);
	}

	decs::Container container = {};
	observerManager.FillContainerObservers(container);

	// TAG TEST:
	{
		using FloatTag = decs::tag<float>;
		using IntTag = decs::tag<int>;
		using DoubleTag = decs::tag<double>;
		using BoolTag = decs::tag<bool>;

		PrintLine(decs::Type<FloatTag>::Name());
		PrintLine(decs::Type<std::vector<decs::Entity>>::Name());
	
		{
			auto prefabEntity1 = container.CreateEntity();

			prefabEntity1.AddComponent<Position>();

			prefabEntity1.AddTag<FloatTag>();
			prefabEntity1.AddComponent<TestComponent>();
			container.Spawn(prefabEntity1, 123, true);
		}
	
		{
			auto prefabEntity1 = container.CreateEntity();

			prefabEntity1.AddComponent<Position>();

			prefabEntity1.AddTag<IntTag>();
			prefabEntity1.AddComponent<TestComponent>();
			container.Spawn(prefabEntity1, 123, true);
		}
	
		{
			auto prefabEntity1 = container.CreateEntity();

			prefabEntity1.AddComponent<Position>();

			prefabEntity1.AddTag<DoubleTag>();
			prefabEntity1.AddComponent<TestComponent>();
			container.Spawn(prefabEntity1, 123, true);
		}
	
		{
			auto prefabEntity1 = container.CreateEntity();

			prefabEntity1.AddComponent<Position>();

			prefabEntity1.AddTag<BoolTag>();
			prefabEntity1.AddComponent<TestComponent>();
			container.Spawn(prefabEntity1, 123, true);
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
