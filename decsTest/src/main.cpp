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
	}

	Position(float x, float y): X(x), Y(y)
	{
	}

	void TestFunc(int& i)
	{
		PrintLine("Is working");
		//i += 1;
	}

protected:
	void OnPreCreate(const decs::Entity& entity) override final
	{
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
	}

	// Inherited via DestroyComponentObserver
	void OnDestroyComponent(TestComponent& component, const decs::Entity& entity) override
	{
	}


	// Inherited via EnableComponentObserver
	void OnEnableComponent(TestComponent& component, const decs::Entity& entity) override
	{
	}


	// Inherited via DisableComponentObserver
	void OnDisableComponent(TestComponent& component, const decs::Entity& entity) override
	{
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

	decs::Entity prefab{};
	{
		decs::Container container = {};
		observerManager.FillContainerObservers(container);

		prefab = container.CreateEntity();
		prefab.AddComponent<TestComponent>();
		prefab.AddComponent<Renderer>();
		prefab.AddComponent<Position>();
		prefab.AddTag<FloatTag>();


		auto rendererRemoveCond = [](const Renderer& renderer)
		{
			return true;
		};

		//prefab.RemoveComponent<Renderer>();
		prefab.RemoveComponent_If<Renderer>(rendererRemoveCond);

		auto comp = prefab.GetComponent<TestComponent>();

		container.Spawn(prefab, 10, true);

		uint32_t counter = 0;
		auto testFunc = [&](const decs::ConstEntity& entity, const TestComponent&)
		{
			PrintLine(std::format("Entity: {0} TestComponent", entity.GetID()));
		};


		{
			decs::Query<TestComponent> query(&container);
			query.ForEach(testFunc);
			query.ForEachBackward(testFunc);
			query.ForEachBackward_Safe(testFunc);
			query.ForEach_IngoreEntityActiveState(testFunc);
		}

		{
			decs::MultiQuery<TestComponent> multiQuery{};
			multiQuery.AddContainer(&container);

			multiQuery.ForEachBackward_Safe(testFunc);


			/*std::vector<decs::MultiQuery<TestComponent> ::BatchIterator> iterators{};
			multiQuery.CreateBatchIteratorsWithMaxNumberPerBatch(iterators, 7);

			for (auto& it : iterators)
			{
				it.ForEach(testFunc);
			}*/
		}
	}

	if (prefab.IsValid())
	{
		PrintLine("Should not happend");
	}

	prefab = {};

	return 0;
}
