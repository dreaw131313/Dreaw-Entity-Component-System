#pragma once

#include "Observers.h"
#include "Container.h"

namespace decs
{
	class ObserversManager
	{
	public:
		inline void SetCreateEntityObserver(CreateEntityObserver* createEntityObserver)
		{
			m_CreateEntityObserver = createEntityObserver;
		}

		inline void SetDestroyEntityObserver(DestroyEntityObserver* destroyEntityObserver)
		{
			m_DestroyEntityObserver = destroyEntityObserver;
		}

		inline void SetEnableEntityObserver(EnableEntityObserver* enableEntityObserver)
		{
			m_EnableEntityObserver = enableEntityObserver;
		}

		inline void SetDisableEntityObserver(DisableEntityObserver* disableEntityObserver)
		{
			m_DisableEntityObserver = disableEntityObserver;
		}

		void FillContainerObservers(Container& container)
		{
			container.SetEntityObservers(
				m_CreateEntityObserver,
				m_DestroyEntityObserver,
				m_EnableEntityObserver,
				m_DisableEntityObserver
			);
		}

	private:
		CreateEntityObserver* m_CreateEntityObserver = nullptr;
		DestroyEntityObserver* m_DestroyEntityObserver = nullptr;
		EnableEntityObserver* m_EnableEntityObserver = nullptr;
		DisableEntityObserver* m_DisableEntityObserver = nullptr;

	};
}