#pragma once
#include "decspch.h"

#include "Core.h"
#include "Container.h"

namespace decs
{
	namespace
	{
		struct EntityCommand
		{
		public:
			EntityID ID = std::numeric_limits<EntityID>::max();
			Container* ECSContainer = nullptr;
		public:
			EntityCommand() {}
			EntityCommand(const EntityID& entityID, Container* ecsContainer) :
				ID(entityID),
				ECSContainer(ecsContainer)
			{
			}

			virtual void PerformCommand() = 0;

			virtual bool IsValid() { return ECSContainer != nullptr; }
		};

		struct DestroyEntityCommand : public EntityCommand
		{
		public:
			DestroyEntityCommand() {}
			DestroyEntityCommand(const EntityID& entityID, Container* ecsContainer) :EntityCommand(entityID, ecsContainer)
			{

			}

			virtual void PerformCommand() override
			{
				if (IsValid())
				{
					ECSContainer->DestroyEntity(ID);
				}
			}
		};

		struct RemoveComponentCommand : public EntityCommand
		{
		public:
			RemoveComponentCommand() {}
			RemoveComponentCommand(
				const EntityID& entityID,
				Container* ecsContainer,
				const TypeID& componentTypeID
			) :
				EntityCommand(entityID, ecsContainer),
				m_CompoenentType(componentTypeID)
			{

			}

			virtual void PerformCommand() override
			{
				if (IsValid())
				{
					ECSContainer->RemoveComponent(ID, m_CompoenentType);
				}
			}

		private:
			TypeID m_CompoenentType = 0;
		};

	}

	struct EntityCommandBuffer final
	{
	public:
		EntityCommandBuffer()
		{
		}

		EntityCommandBuffer(
			const uint64_t& initialDestroyEntityCommandsCapacity,
			const uint64_t& initialRemoveComponentCommandsCapacity
		)
		{
			m_DestroyEntityCommands.reserve(initialDestroyEntityCommandsCapacity);
			m_RemoveComponentsCommands.reserve(initialDestroyEntityCommandsCapacity);
		}

		~EntityCommandBuffer()
		{

		}

		void PerformCommands()
		{
			for (auto& command : m_DestroyEntityCommands)
				command.PerformCommand();

			for (auto& command : m_RemoveComponentsCommands)
				command.PerformCommand();

			ClearCommands();
		}

		void ClearCommands()
		{
			m_DestroyEntityCommands.clear();
			m_RemoveComponentsCommands.clear();
		}

		void ShrinkCommandsContainers()
		{
			m_DestroyEntityCommands.shrink_to_fit();
			m_RemoveComponentsCommands.shrink_to_fit();
		}


		inline uint64_t GetCommandsCount() const { return m_DestroyEntityCommands.size() + m_RemoveComponentsCommands.size(); }
		inline uint64_t GetDestroyEntityCommandsCount() const { return m_DestroyEntityCommands.size(); }
		inline uint64_t GetRemoveComponentCommandsCount() const { return m_RemoveComponentsCommands.size(); }


		inline void AddDestroyEntityCommand(const EntityID& entityID, Container& ecsContainer)
		{
			m_DestroyEntityCommands.emplace_back(entityID, &ecsContainer);
		}


		inline void AddRemoveComponentCommand(const EntityID& entityID, Container& ecsContainer, const TypeID& typeID)
		{
			m_RemoveComponentsCommands.emplace_back(entityID, &ecsContainer, typeID);
		}

		template<typename ComponentType>
		inline void AddRemoveComponentCommand(const EntityID& entityID, Container& ecsContainer)
		{
			AddRemoveComponentCommand(entityID, ecsContainer, Type<ComponentType>::ID());
		}

	private:
		std::vector<DestroyEntityCommand> m_DestroyEntityCommands;
		std::vector<RemoveComponentCommand> m_RemoveComponentsCommands;

	};

}