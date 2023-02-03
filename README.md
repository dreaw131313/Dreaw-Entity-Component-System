# Dreaw-Entity-Component-System
**Dreaw-Entity-Component-System** in short **decs** it is simple ecs library written in **C++** which I am developing for use in my own game engine<br/>

## decs requirements
* Requires C++ 17
* Mainly standard library is used, but hash maps from https://github.com/skarupke/flat_hash_map repository are also used. Which map is used can be changed in Core.h file by changing line with: ``` using ecsMap = std::unordered_map<Key, Value>; ```

## How to use **decs**
To start using decs, copy the **decs** folder to your project and include the header file **decs.h**.

### Creating and storing entities and components
All entites and components are stored in class **decs::Container** which is giving access for method for creating and destroying entities.<br/>
Component classes do not need to inherit from any class. Base types like int, float etc. (except bool) can also be components.<br/>

By default components stored in **decs::Container** do not have **stable memory adress**, but it can be enforced by using template **decs::Stable< ComponentType >** instead of only **ComponentType**.<br/>

```cpp
class Component1
{
public:
	float X = 0;
	float Y = 0;
	
	Component1();
	Component1(const float& x, const float& y);
};

class Component2
{
public:
	float X = 0;
	float Y = 0;
	
	Component2();
	Component2(const float& x, const float& y);
};

int main()
{
	decs::Container container = {};
	decs::Entity entity1 = container.CreateEntity();
	// using entity member function :
	Component1* c1 = entity1.AddComponent<Component1>(1.f,2.f);
	Component2* c2 = entity1.AddComponent<decs::Stable<Component2>>(3.f,4.f);
	entity1.RemoveComponent<Component1>();
	entity1.RemoveComponent<decs::Stable<Component2>>();
	entity1.Destroy()
	
	return 0;
}
```

Like in most of ecs systems, in **decs** entity can have only one component of given type. If component of the same type will be added twice AddComponent function will return pointer to component created earlier.<br/>

### Entity class
decs::Entity class is represented by ID and reference to decs::Container in which was created.<br/>

Entites can also be spawned which is a little faster than creating them by regular method. To spawn entity you need to use one of **Spawn()** methods from decs::Container class.
```cpp
Entity Spawn(const Entity& prefab, const bool& isActive = true);
bool Spawn(
	const Entity& prefab, 
	std::vector<Entity*>& spawnedEntities, 
	const uint64_t& spawnCount, 
	const bool& areActive = true
	);
bool Spawn(const Entity& prefab, const uint64_t& spawnCount, const bool& areActive = true);
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
**decs::View<typename... Components>** object serves to iterating over entities
```cpp
decs::Container container = {}; 

// this view can iterate over all entities which contains components passed as template parameters
decs::View<Component1, decs::Stable<Component2>> view = { container }; 

view.ForEach([&](Component1& c1, Component2& c2)
{
	// doing stuff with components
});

 // in this function first paramter of lambda must be decs::Entity
view.ForEachWithEntity([&](decs::Entity& e, Component1& c1, Component2& c2)
{
	// doing stuff with components and entity
});
```
It is also possible to query more complex views using the member methods of the view class:
```cpp
template<typename... ComponentsTypes>
View& Without(); // Entities in view will not have all components from ComponetsTypes parameters list
```
```cpp
template<typename... ComponentsTypes>
View& WithAnyFrom(); // Entities in view will have at least one of component from ComponentTypes parameters list
```
```cpp
template<typename... ComponentsTypes>
View& With(); // Entities in view will have all components from ComponentTypes parameters list
```

Creating view with this methods can look like:
```cpp
decs::View<Component1, decs::Stable<Component2>, Component3> view = { container };
view.Without<Component4,Component5>().WithAnyFrom<Component6, Component7>().With<Component8, Component9>();

view.ForEach([](Component1& c1, Component2& c2, Component3& c3)
{
	// doing stuff with components
});

view.ForEach([](decs::Entity& e, Component1& c1, Component2& c2, Component3& c3)
{
	// doing stuff with components and entity
});
```

During iteration with methods **ForEach** and **ForEachWithEntity** is possible:
* create new entites. 
* adding and removing components from currently iterated entity
* destroying currently iterated entity

Things like:
* destroying other entites
* adding and removing component from entities diffrent than curently iterated entity

are undefined behavior.

