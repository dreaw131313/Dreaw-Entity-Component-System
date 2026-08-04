#pragma once

#include "decs/Core/RefCounterHandle.h"

namespace decs
{
	class Container;

	/// <summary>
	/// Helper class which allows simple flag change to set all entities as dead
	/// </summary>
	struct ContainerLifetimeData :
		public RefCountedObject,
		private NonCopyableNonMoveable
	{
		friend class Container;
		friend struct Entity;
		friend struct ConstEntity;

	public:
		ContainerLifetimeData(Container* container) :
			m_Container(container)
		{

		}

		~ContainerLifetimeData() = default;

		inline bool IsAlive() const noexcept
		{
			return m_bIsContainerAlive.load();
		}

	private:
		Container* m_Container = nullptr;
		std::atomic<bool> m_bIsContainerAlive = true;

	private:
		void MarkDead()
		{
			m_bIsContainerAlive = false;
			m_Container = nullptr;
		}

		inline Container* GetContainer() const noexcept
		{
			return m_Container;
		}
	};

	using ContainerLifetimeDataHandle = TRefCountHandle<ContainerLifetimeData>;

}