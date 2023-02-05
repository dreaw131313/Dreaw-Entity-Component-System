#pragma once
#include "Core.h"
#include "Type.h"
#include "Entity.h"
#include "Container.h"

namespace decs
{
	template<typename... ComponentsTypes>
	class Query
	{
	private:
		struct ArchetypeContext
		{
		public:
			Archetype* Arch = nullptr;
			uint64_t m_EntitiesCount = 0;
			PackedContainerBase* m_Containers[sizeof...(ComponentsTypes)] = { nullptr };

		public:
			inline void ValidateEntitiesCount() { m_EntitiesCount = Arch->EntitiesCount(); }
		};

		struct ContainerContext
		{
		public:
			Container* m_Container = nullptr;
			std::vector<ArchetypeContext> m_ArchetypesContexts;
			ecsSet<Archetype*> m_ContainedArchetypes;
			uint64_t m_ArchetypesCountDirty = 0;
		};

	public:
		template<typename... ComponentsTypes>
		Query& Without()
		{
			m_IsDirty = true;
			m_Without.clear();
			findIds<ComponentsTypes...>(m_Without);
			return *this;
		}

		template<typename... ComponentsTypes>
		Query& WithAnyFrom()
		{
			m_IsDirty = true;
			m_WithAnyOf.clear();
			findIds<ComponentsTypes...>(m_WithAnyOf);
			return *this;
		}

		template<typename... ComponentsTypes>
		Query& With()
		{
			m_IsDirty = true;
			m_WithAll.clear();
			findIds<ComponentsTypes...>(m_WithAll);
			return *this;
		}

		template<typename Callable>
		inline void ForEach(Callable&& func) noexcept
		{
			ForEachBackward(func);
		}

		template<typename Callable>
		void ForEachForward(Callable&& func) noexcept
		{
		}

		template<typename Callable>
		void ForEachBackward(Callable&& func) noexcept
		{
		}

	private:
		TypeGroup<ComponentsTypes...> m_Includes = {};
		std::vector<TypeID> m_Without;
		std::vector<TypeID> m_WithAnyOf;
		std::vector<TypeID> m_WithAll;

		bool m_IsDirty = true;

	private:
		template<typename T = void, typename... Args>
		void CreatePackedContainersTuple(
			std::tuple<PackedContainer<ComponentsTypes>*...>& containersTuple,
			const ArchetypeContext& context
		) const noexcept
		{
			constexpr uint64_t compIdx = sizeof...(ComponentsTypes) - sizeof...(Args) - 1;
			std::get<PackedContainer<T>*>(containersTuple) = (reinterpret_cast<PackedContainer<T>*>(context.m_Containers[compIdx]));

			if constexpr (sizeof...(Args) == 0) return;

			CreatePackedContainersTuple<Args...>(
				containersTuple,
				context
				);
		}

		template<>
		void CreatePackedContainersTuple<void>(
			std::tuple<PackedContainer<ComponentsTypes>*...>& containersTuple,
			const ArchetypeContext& context
			) const noexcept
		{

		}

	};
}