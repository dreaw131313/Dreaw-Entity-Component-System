#pragma once
#include "Type.h"
#include "ObserversManager.h"

namespace decs
{
	class Entity;
	class Container;

	class ComponentContextBase
	{
		friend class Container;
	public:
		ComponentContextBase()
		{

		}

		virtual ~ComponentContextBase()
		{

		}

		inline virtual uint64_t GetChunkCapacity() const = 0;

		virtual TypeID GetComponentTypeID() const = 0;
		virtual void InvokeOnCreateComponent_S(void* component, Entity& entity) = 0;
		virtual void InvokeOnDestroyComponent_S(void* component, Entity& entity) = 0;
		virtual void SetObserverManager(ObserversManager* observerManager) = 0;
		virtual ComponentContextBase* CreateOwnEmptyCopy(ObserversManager* observerManager) = 0;

		inline int GetObservatorOrder() const { return m_ObservatorOrder; }

	private:
		int m_ObservatorOrder = 0;
	};

	template<typename ComponentType>
	class ComponentContext : public ComponentContextBase
	{
		friend class Container;
	public:
		ComponentObserver<ComponentType>* m_Observer = nullptr;

	public:
		ComponentContext(
			const uint64_t& chunkCapacity,
			ComponentObserver<ComponentType>* observer
		) :
			m_ChunkCapacity(chunkCapacity),
			m_Observer(observer)
		{

		}

		~ComponentContext()
		{

		}

		inline virtual uint64_t GetChunkCapacity() const override
		{
			return m_ChunkCapacity;
		}

		virtual TypeID GetComponentTypeID() const { return Type<ComponentType>::ID(); }

		virtual void InvokeOnCreateComponent_S(void* component, Entity& entity)override
		{
			if (m_Observer != nullptr && m_Observer->m_CreateObserver != nullptr)
			{
				m_Observer->m_CreateObserver->OnCreateComponent(*reinterpret_cast<ComponentType*>(component), entity);
			}
		}

		virtual void InvokeOnDestroyComponent_S(void* component, Entity& entity)override
		{
			if (m_Observer != nullptr && m_Observer->m_DestroyObserver != nullptr)
			{
				m_Observer->m_DestroyObserver->OnDestroyComponent(*reinterpret_cast<ComponentType*>(component), entity);
			}
		}

		void InvokeOnCreateComponent(ComponentType& component, Entity& entity)
		{
			if (m_Observer != nullptr && m_Observer->m_CreateObserver != nullptr)
			{
				m_Observer->m_CreateObserver->OnCreateComponent(component, entity);
			}
		}

		void InvokeOnDestroyComponent(ComponentType& component, Entity& entity)
		{
			if (m_Observer != nullptr && m_Observer->m_DestroyObserver != nullptr)
			{
				m_Observer->m_DestroyObserver->OnDestroyComponent(component, entity);
			}
		}

		virtual void SetObserverManager(ObserversManager* observerManager) override
		{
			if (observerManager == nullptr)
			{
				m_Observer = nullptr;
			}
			else
			{
				m_Observer = observerManager->GetComponentObserver<ComponentType>();
			}
		}

		ComponentContextBase* CreateOwnEmptyCopy(ObserversManager* observerManager) override
		{
			if (observerManager != nullptr)
			{
				return new ComponentContext<ComponentType>(
					m_ChunkCapacity,
					observerManager->GetComponentObserver<ComponentType>()
					);
			}

			return new ComponentContext<ComponentType>(
				m_ChunkCapacity,
				nullptr
				);
		}

	private:
		uint64_t m_ChunkCapacity = 1000;
	};
}