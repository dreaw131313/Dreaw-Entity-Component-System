#pragma once
#include "decs/Core.h"
#include "decs/trait.h"

#include "decs/Component/ChunkAllocator.h"
#include "decs/Type.h"

namespace decs
{

	/// <summary>
	/// Two last bits are taken. Bits from 0 to 29 are available (30 in total);
	/// Bits 30 and 31 must not be used!
	/// TODO: Add asserts to setting this bits!
	/// </summary>
	struct EntityComponentFlags
	{
	private:
		inline static constexpr const uint8_t s_IsCreatedBitIndex = 30;
		inline static constexpr const uint8_t s_IsEnabledBitIndex = 31;
		inline static constexpr const uint32_t s_IsCreatedBit = 1u << s_IsCreatedBitIndex;
		inline static constexpr const uint32_t s_IsEnabledBit = 1u << s_IsEnabledBitIndex;

	public:
		inline bool IsCreated() const noexcept
		{
			return m_Data & s_IsCreatedBit;
		}

		inline bool IsEnabled() const noexcept
		{
			return m_Data & s_IsEnabledBit;
		}

		inline void SetCreated(bool bIsCreated)
		{
			if (bIsCreated)
			{
				m_Data = m_Data | s_IsCreatedBit;
			}
			else
			{
				m_Data = m_Data & ~s_IsCreatedBit;
			}
		}

		inline void SetEnabled(bool bIsEnabled)
		{
			if (bIsEnabled)
			{
				m_Data = m_Data | s_IsEnabledBit;
			}
			else
			{
				m_Data = m_Data & ~s_IsEnabledBit;
			}
		}

		inline bool GetBit(uint8_t bitIndex) const noexcept
		{
			return m_Data & (1u << bitIndex);
		}

		inline void SetBit(uint8_t bitIndex, bool bValue)
		{
			// we assert in debug builds because internal bits can only be changed by this library
			DECS_ASSERT(bitIndex != s_IsCreatedBitIndex && bitIndex != s_IsEnabledBitIndex, "Bit indices used internaly must not be used!");

			if (bValue)
			{
				m_Data = m_Data | (1u << bitIndex);
			}
			else
			{
				m_Data = m_Data & ~(1u << bitIndex);
			}
		}

	private:
		uint32_t m_Data = 0;
	};


	class EntityData;
	class Entity;
	class IStableComponentContainer;
	template<typename TComponentType>
	class StableComponentContainer;

	class EntityComponent : public ChunkAllocatorResource
	{
		friend class Container;
		template<typename>
		friend class ComponentContext;
		template<typename>
		friend class PackedContainer;
		template<typename>
		friend class PackedStableComponentContainer;

		friend class IStableComponentContainer;
		template<typename TComponentType>
		friend class StableComponentContainer;

	public:
		EntityComponent() = default;

		EntityComponent(const EntityComponent& other):
			ChunkAllocatorResource(other)
		{

		}

		EntityComponent(EntityComponent&& other) noexcept:
			ChunkAllocatorResource(std::move(other))
		{

		}

		EntityComponent& operator =(const EntityComponent& other)
		{
			ChunkAllocatorResource::operator=(other);
			return *this;
		}

		EntityComponent& operator=(EntityComponent&& other) noexcept
		{
			if (this != &other)
			{
				ChunkAllocatorResource::operator=(std::move(other));
			}

			return *this;
		}

		virtual ~EntityComponent() = default;

		Entity GetEntity() const noexcept;

		inline bool IsCreatedByECS() const noexcept
		{
			return m_Flags.IsCreated();
		}

		inline bool IsEnabledByECS() const noexcept
		{
			return m_Flags.IsEnabled();
		}

		inline uint16_t GetDependecyCount() const
		{
			return m_DependencyCount;
		}

		inline void AddDependency(uint16_t dependecyCount = 1)
		{
			m_DependencyCount += dependecyCount;
		}

		inline void RemoveDependecy(uint16_t dependecyCount = 1)
		{
			if (dependecyCount > m_DependencyCount)
			{
				m_DependencyCount = 0;
			}
			else
			{
				m_DependencyCount -= dependecyCount;
			}
		}

	protected:
		inline void SetInternalFlag(uint8_t flagIndex, bool bValue)
		{
			m_Flags.SetBit(flagIndex, bValue);
		}

		inline bool GetInternalFlag(uint8_t flagIndex) const noexcept
		{
			return m_Flags.GetBit(flagIndex);
		}

	private:
		EntityData* m_EntityData = nullptr;
		EntityComponentFlags m_Flags{};
		uint16_t m_DependencyCount = 0;

	private:
		inline void OnPreCreate(EntityData* entitydata)
		{
			m_EntityData = entitydata;
		}

		inline void SetFlags(bool bIsCreated, bool bIsEnabled)
		{
			m_Flags.SetCreated(bIsCreated);
			m_Flags.SetEnabled(bIsEnabled);
		}

		inline void SetCreated(bool bIsCreated)
		{
			m_Flags.SetCreated(bIsCreated);
		}

		inline void SetEnabled(bool bIsEnabled)
		{
			m_Flags.SetEnabled(bIsEnabled);
		}


	};

	template<typename TComponentType>
	concept TComponentConcept = std::derived_from<TComponentType, EntityComponent>;

	template<typename T>
	concept TComponentOrTagConcept = TComponentConcept<T> || TTagConcept<T>;

	template<TComponentConcept... Types>
	class ComponentTypeGroup
	{
	public:
		constexpr TypeID operator[](const uint64_t index) const
		{
			return m_Group[index];
		}

		constexpr uint64_t Size() const
		{
			return m_Group.Size();
		}

	private:
		TypeGroup<Types...> m_Group{};
	};
}