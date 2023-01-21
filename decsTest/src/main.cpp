#include <iostream>

#include "decs/decs.h"


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
	if (comp == nullptr)
	{
		Print("component is not added!");
	}

	comp->entityToDestroy = container.CreateEntity();
	comp->entityToDestroy.AddComponent<C1>();

	decs::ObserversManager observerManager;

	C1Observer c1Observer = { &container };
	observerManager.SetComponentCreateObserver<C1>(&c1Observer);
	observerManager.SetComponentDestroyObserver<C1>(&c1Observer);

	container.SetObserversManager(&observerManager);
	container.InvokeEntitesOnCreateListeners();


	if (!entity.HasComponent<C1>())
	{
		Print("Entity has not component!");
	}

	if (!entity.RemoveComponent<C1>())
	{
		Print("Failed to remove component!");
	}


	using ViewType = decs::View<C1>;

	ViewType testView = { container };

	auto lambda = [&] (decs::Entity& entity, C1& c1)
	{

	};

	testView.ForEachWithEntity(lambda);
	testView.ForEachWithEntity(decs::IterationType::Forward, lambda);


	container.DestroyOwnedEntities();

	//std::cin.get();
	return 0;
}