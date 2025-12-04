#include <iostream>
#include <format>

#include "decs/decs.h"

void PrintLine(std::string message = "")
{
	std::cout << message << "\n";
}


struct Position : public decs::EntityComponent
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

};

struct TestComponent : public decs::EntityComponent
{
public:
	int table[10];

	TestComponent()
	{
		PrintLine("TestComponent::TestComponent");
	}

	TestComponent(const TestComponent& other)
	{
		PrintLine("TestComponent::TestComponent(const TestComponent&)");
	}

	~TestComponent()
	{
		PrintLine("TestComponent::~TestComponent");
	}

};

struct Renderer :public decs::EntityComponent
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


	std::cout << decs::Type< decs::drop_const_t<const float>>::ID() << "\n";
	std::cout << decs::Type<float>::ID() << "\n";

	decs::Entity prefab{};
	{
		decs::ContainerConfig containerConfig{
			.EntityChunkSize = 1000,
			.DefaultComponentChunkSize = 200,
			.ArchetypeChunkSize = 200,
		};
		decs::Container container = { containerConfig };
		observerManager.FillContainerObservers(container);

		prefab = container.CreateEntity();

		if (prefab)
		{

		}

		decs::ConstEntity constPrefab = prefab;
		if (constPrefab)
		{

		}

		prefab.AddComponent<TestComponent>();


		prefab.AddComponent<Renderer>();
		prefab.AddTag<FloatTag>();
		prefab.AddTag<IntTag>();
		prefab.AddTag<BoolTag>();

		prefab.AddComponent<Position>(10.f, 10.f);
		auto position = prefab.GetComponent<const Position>();

		auto rendererRemoveCond = [](const Renderer& renderer)
		{
			return true;
		};

		//prefab.RemoveComponent<Renderer>();
		prefab.RemoveComponent_If<Renderer>(rendererRemoveCond);
		prefab.RemoveTag<IntTag>();

		auto comp = prefab.GetComponent<TestComponent>();

		container.Spawn(prefab, 10, true);

		uint32_t counter = 0;
		auto testFunc = [&](const decs::ConstEntity& entity, const TestComponent&)
		{
			PrintLine(std::format("Entity: {0} TestComponent", entity.GetID()));
		};


		{
			using QueryType = decs::Query< const TestComponent>;
			QueryType query(&container);
			/*query.ForEach(testFunc);
			query.ForEach_Safe(testFunc);
			query.ForEachBackward(testFunc);
			query.ForEachBackward_Safe(testFunc);
			query.ForEach_IngoreEntityActiveState(testFunc);*/

			uint64_t queryEntityCount = query.GetEntityCount();

			std::vector<QueryType::BatchIterator> iterators{};
			query.CreateBatchIterators(iterators, 10, 3);

			for (auto& it : iterators)
			{
				it.ForEach(testFunc);
				it.ForEach_IngoreEntityActiveState(testFunc);
			}
		}

		{
			using QueryType = decs::MultiQuery<const TestComponent>;
			QueryType multiQuery{};
			multiQuery.WithAny<FloatTag, IntTag>();
			multiQuery.AddContainer(&container);

			uint64_t queryEntityCount = multiQuery.GetEntityCount();

			multiQuery.ForEach(testFunc);
			multiQuery.ForEach_Safe(testFunc);
			multiQuery.ForEachBackward(testFunc);
			multiQuery.ForEachBackward_Safe(testFunc);
			multiQuery.ForEach_IngoreEntityActiveState(testFunc);


			std::vector<QueryType::BatchIterator> iterators{};
			multiQuery.CreateBatchIteratorsWithMaxNumberPerBatch(iterators, 7);

			for (auto& it : iterators)
			{
				it.ForEach(testFunc);
				it.ForEach_IngoreEntityActiveState(testFunc);
			}
		}

	}

	return 0;
}
