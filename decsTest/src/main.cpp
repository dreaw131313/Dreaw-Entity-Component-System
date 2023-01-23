#include <iostream>

#include "decs/decs.h"

#include "decs/PackedComponentContainer.h"


void Print(const std::string& message)
{
	std::cout << message << std::endl;
}

class C1
{
public:
	decs::Entity entityToDestroy;

public:
	C1()
	{

	}
};

class C1Observer :
	public decs::CreateComponentObserver<C1>,
	public decs::DestroyComponentObserver<C1>
{
public:
	decs::Container* m_ECSContainer;
	C1Observer(decs::Container* ecsContainer) :
		m_ECSContainer(ecsContainer)
	{

	}
public:
	void OnCreateComponent(C1& c, decs::Entity& e) override
	{
		Print("C1 Create");

		m_ECSContainer->Spawn(c.entityToDestroy);
		if (!c.entityToDestroy.IsNull())
		{
			c.entityToDestroy.RemoveComponent<C1>();
			c.entityToDestroy.AddComponent<C1>();
			c.entityToDestroy.RemoveComponent<C1>();
		}
	}

	void OnDestroyComponent(C1& c, decs::Entity& e) override
	{
		Print("C1 Destroy");
	}

};

int main()
{
	Print("Start:");


	decs::Container container = {};
	decs::Entity entity = container.CreateEntity();

	C1* comp = entity.AddComponent<C1>();



	/*if (!entity.HasComponent<C1>())
	{
		Print("Entity has not component!");
	}

	if (!entity.RemoveComponent<C1>())
	{
		Print("Failed to remove component!");
	}


	using ViewType = decs::View<C1>;

	ViewType testView = { container };

	auto lambda1 = [&] (decs::Entity& entity, C1& c1)
	{

	};
	auto lambda2 = [&] (C1& c1)
	{

	};

	testView.ForEach(lambda1);
	testView.ForEach(lambda2);
	testView.ForEach(lambda1, decs::IterationType::Backward);
	testView.ForEach(lambda2, decs::IterationType::Backward);
	testView.ForEach(lambda1, decs::IterationType::Forward);
	testView.ForEach(lambda2, decs::IterationType::Forward);


	std::vector<ViewType::BatchIterator> iterators;
	testView.CreateBatchIterators(iterators, 10, 1000);
	for (auto& it : iterators)
	{
		it.ForEach(lambda1);
		it.ForEach(lambda2);
	}

	container.DestroyOwnedEntities();*/


	decs::PackedContainer<float> packedContainerTest = { 1000 };

	//std::cin.get();
	return 0;
}