#pragma once

namespace decs
{
	class Entity;

	class ComponentBase
	{
		friend class Container;
		template<typename>
		friend class ComponentContext;
		template<typename>
		friend class PackedContainer;
		template<typename>
		friend class StablePackedContainer;

	public:
		ComponentBase() = default;

		ComponentBase(const ComponentBase& other)
		{

		}

		ComponentBase(ComponentBase&& other) noexcept
		{

		}

		ComponentBase& operator =(const ComponentBase& other)
		{
			return *this;
		}

		ComponentBase& operator =(ComponentBase&& other) noexcept
		{
			return *this;
		}

		virtual ~ComponentBase() = default;

		inline bool IsCreatedByECS() const
		{
			return m_bIsCreatedByContainer;
		}

		inline bool IsEnabledByECS() const
		{
			return m_bIsEnabledByECS;
		}
		
	protected:
		/// <summary>
		/// This function is called always when adding component to entity (and when entity is spawned). Removing any other component or removing this component is forbidden because it cause undefined behavior.
		/// </summary>
		/// <param name="entity"></param>
		virtual void OnPreCreate(const Entity& entity);

	private:
		bool m_bIsCreatedByContainer = false;
		bool m_bIsEnabledByECS = false;

	private:
		inline void SetFlags(bool bIsCreated, bool bIsEnabled)
		{
			m_bIsCreatedByContainer = bIsCreated;
			m_bIsEnabledByECS = bIsEnabled;
		}
	};

}