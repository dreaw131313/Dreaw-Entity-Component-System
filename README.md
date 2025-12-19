# Dreaw-Entity-Component-Systems
**Dreaw-Entity-Component-Systems** in short **decs** it is archetype based archetype based ECS library written in **C++ 20**.
Libarry offers two versions of ecs solution:
1.	Called "**Normal**", available in **decs** namespace, and is more heavier version which porvide much features and some drawbacks like:
	*	components must inherits from base class **decs::EntityComponent**
  	*	each component type is allocated in chunks, where number of component in one chunk can be specified in **decs::Container** object, if one chunk is fully occupied, new component chunk is allocated
	*	all components have stable memory addreses
	*	entities can be enabled and disabled
	*	observers for entity and component **creation, destruction, enable and disable** callbacks can be specified
 	*	offers separate methods for creating/destroying entities, adding/removing components without callbacks
 	*	**decs::Entity** which outlive parent **decs::Container** behaves empty or destroyed entity
3.	Called "**Light**", available in **decs::light** namespace, where each archetype stores each component type in **std::vector**
   	*	components do not have stable memory addresses, and can be each type except **bool**
	*	entities cannot be enabled and disabled
 	*	there is no way to specify entity or components observers
  	*	offers better iteration performance than first version

API for using both version are same, but **Light** version use **decs::light** namespace

## How to use **decs**
To start using decs, copy the **decs** folder to your project and include the header file **decs.h**.

### Creating and storing entities, components and tags
All entites, components and tags are stored in class **decs::Container** or **decs::light::Container**.<br/>

####Tags
Each type can be tag. To add/remove tag to/from entity, decs::Entity methods like:
```cpp
template<TTagConcept TTag>
bool decs::Entity::AddTag();
template<TTagConcept TTag>
bool decs::Entity::RemoveTag();
```
can be used.

**TTagConcept** can be any class decorated with **decs::tag<>** template.

#### Example
```cpp
class Component1 : public decs::EntityComponent
{
public:
	float X = 0;
	float Y = 0;
	
	Component1();
	Component1(const float& x, const float& y);
};

class Component2 : public decs::EntityComponent
{
public:
	float X = 0;
	float Y = 0;
	
	Component2();
	Component2(const float& x, const float& y);
};

class Tag1
{
};

class Tag2
{
};

int main()
{
	decs::Container container = {};
	decs::Entity entity1 = container.CreateEntity();

	// Components:
	Component1* c1 = entity1.AddComponent<Component1>(1.f,2.f);
	Component2* c2 = entity1.AddComponent<Component2>(3.f,4.f);
	entity1.RemoveComponent<Component1>();
	entity1.RemoveComponent<Component2>();

	// Tags:
	entity.AddTag<decs::Tag<Tag1>>();
	entity.AddTag<decs::Tag<Tag2>>();
	entity.RemoveTag<decs::Tag<Tag1>>();
	entity.RemoveTag<decs::Tag<Tag2>>();

	entity1.Destroy()
	
	return 0;
}
```

Like in most of ecs systems, in **decs** entity can have only one component of given type. If component of the same type will be added twice AddComponent function will return pointer to component created earlier.<br/>

### Spawning Entities
Entites can also be spawned which is a little faster than creating them by regular method. To spawn entity you need to use one of **Spawn()** methods from decs::Container class.
```cpp
Entity decs::Container::Spawn(const Entity& prefab, bool isActive = true);

bool decs::Container::Spawn(
	const Entity& prefab, 
	std::vector<Entity*>& spawnedEntities, 
	uint64_t spawnCount, 
	bool areActive = true
	);
	
bool decs::Container::Spawn(const Entity& prefab, uint64_t spawnCount, bool areActive = true);
```
As prefab parameter can be used entity from any decs::Container.

Entities can also be activated or deactivated. Deactivated entities will not be iterated. Activating and deactivating is performed by functions:
```cpp
decs::Container container = {};
decs::Entity entity = container.CreateEntity();
entity.SetActive(true);
bool isActive = entity.IsActive();
```

### Iterating over entites
**decs::Query<typename... Components>** object serves to iterating over entities. It has 3 methods for iteration:
```cpp
// Iterates over entities in archetypes from first to last
template<typename Callable
decs::Query::ForEach(Callable&& func);

// Iterates over entities in archetypes form last to first
template<typename Callable
decs::Query::ForEachBackward(Callable&& func);

// Iterates over entities in archetypes form last to first
template<typename Callable
decs::Query::ForEachSafe(Callable&& func);

```

```cpp
decs::Container container = {}; 

// this query can iterate over all entities which contains components passed as template parameters
decs::Query<Component1, Component2> query = { container }; 

query.ForEach([](Component1& c1, Component2& c2)
{
	// doing stuff with components
});

// if during iteration entity reference is needed, decs::Entity can be placed as first parameter of passed callable to ForEach function 
query.ForEach([](decs::Entity& e, Component1& c1, Component2& c2)
{
	// doing stuff with components and entity
});
```
It is also possible to query for more complex queries by using the member methods of the **decs::Query** class:
```cpp
template<typename... ComponentsTypes>
Query& Without(); // Entities in query will not have all components from ComponetsTypes parameters list
```
```cpp
template<typename... ComponentsTypes>
decs::Query& WithAnyFrom(); // Entities in query will have at least one of component from ComponentTypes parameters list
```
```cpp
template<typename... ComponentsTypes>
decs::Query& With(); // Entities in query will have all components from ComponentTypes parameters list
```

Creating Query with this methods can look like:
```cpp
decs::Query<Component1, Component2, Component3> query = { container };
query.Without<decs::tag<Tag1>, Component5>().WithAnyFrom<decs::tag<Tag2>, Component7>().With<decs::tag<Tag3>, Component9>();

query.ForEach([](Component1& c1, Component2& c2, Component3& c3)
{
	// doing stuff with components
});

query.ForEach([](decs::Entity& e, Component1& c1, Component2& c2, Component3& c3)
{
	// doing stuff with components and entity
});
```

**Query** can be used to iterate from multiple threads. To be able to iterate from multiple threads, first we need create batch iterators from **Query** with method:
```cpp
void decs::Query::CreateBatchIterators(std::vector<BatchIterator>& iterators, uint64_t desiredBatchesCount, uint64_t minBatchSize);
```
* **desiredBatchesCount** - number of maximum batch iterators which can be created for this query
* **minBatchSize** is minimal number of enttites in each iterator. 

If number of entities in query is less than **desiredBatchesCount** * **minBatchSize** then function will generate smaller number of iterators witch will contain max **minBatchSize** count of entities.

```cpp
using QueryType = decs::Query<Component1>;
QueryType query = { container };
std::vector<QueryType::BatchIterator> iterators;
query.CreateBatchIterators(iterators, 1000, 8);
for (auto& it : iterators)
{
	it.ForEach(lambda); // can be called from diffrent threads
}
```
During iteration over **Query::BatchIterator** with **ForEach** method, the same rules applay as when iterating with **ForEachForward** method of **Query** class.

**MutliQuery** offers same functionality as **Query**, but it can iterate over multiple **decs::Containers**. Each **decs::Container** can be added, removed and enabled/disabled with methods:
```cpp
bool decs::MutliQuery::AddContainer(decs::Container* container, bool bIsEnabled);
bool decs::MutliQuery::RemoveContainer(decs::Container* container);
void decs::MutliQuery::SetContainerEnabled(decs::Container* container, bool bIsEnabled);
```
Disabled containers will not be itareated.


