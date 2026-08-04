#pragma once
#include "decs/Core/Core.h"
#include "decs/Core/trait.h"
#include "decs/Core/Type.h"


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
		inline static constexpr const uint8_t s_IsAllocatedBitIndex = 13;
		inline static constexpr const uint8_t s_IsCreatedBitIndex = 14;
		inline static constexpr const uint8_t s_IsEnabledBitIndex = 15;
		inline static constexpr const uint32_t s_IsAllocatedBit = 1u << s_IsAllocatedBitIndex;
		inline static constexpr const uint32_t s_IsCreatedBit = 1u << s_IsCreatedBitIndex;
		inline static constexpr const uint32_t s_IsEnabledBit = 1u << s_IsEnabledBitIndex;

	public:
		inline bool IsAllocated() const noexcept
		{
			return m_Data & s_IsAllocatedBitIndex;
		}

		inline bool IsCreated() const noexcept
		{
			return m_Data & s_IsCreatedBit;
		}

		inline bool IsEnabled() const noexcept
		{
			return m_Data & s_IsEnabledBit;
		}

		inline void SetAllocated(bool bAllocated)
		{
			if (bAllocated)
			{
				m_Data = m_Data | s_IsAllocatedBitIndex;
			}
			else
			{
				m_Data = m_Data & ~s_IsAllocatedBitIndex;
			}
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

		inline bool SetBit(uint8_t bitIndex, bool bValue)
		{
			if (bitIndex == s_IsCreatedBitIndex || bitIndex == s_IsEnabledBitIndex || bitIndex == s_IsAllocatedBitIndex)
			{
				return false;
			}

			if (bValue)
			{
				m_Data = m_Data | (1u << bitIndex);
			}
			else
			{
				m_Data = m_Data & ~(1u << bitIndex);
			}

			return true;
		}

	private:
		uint16_t m_Data = 0;
	};

	struct EntityData;
	struct Entity;
	class IStableComponentContainer;
	template<typename TComponentType>
	class StableComponentContainer;

	class InternalComponentData
	{
		template<typename T>
		friend class TComponentChunk;
		friend class EntityComponent;

	private:
		EntityData* m_EntityData = nullptr;
		uint32_t m_IndexInAllocator = std::numeric_limits<uint32_t>::max();
		EntityComponentFlags m_Flags = {};
		uint16_t m_DependencyCount = 0;

	private:
		void Reset()
		{
			m_IndexInAllocator = std::numeric_limits<uint32_t>::max();
			m_EntityData = nullptr;
			m_Flags = {};
			m_DependencyCount = 0;
		}

		bool IsAllocated() const noexcept
		{
			return m_Flags.IsAllocated();
		}

		void SetAllocated(bool bAllcoated)
		{
			m_Flags.SetAllocated(bAllcoated);
		}
	};

	class EntityComponent
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

		template<typename T>
		friend class TComponentChunk;
		template<typename T>
		friend class TComponentAllocator;

	public:
		EntityComponent() = default;
		~EntityComponent() = default;

		EntityComponent(const EntityComponent& other)
		{

		}

		EntityComponent(EntityComponent&& other) noexcept
		{

		}

		EntityComponent& operator =(const EntityComponent& other)
		{
			return *this;
		}

		EntityComponent& operator=(EntityComponent&& other) noexcept
		{
			return *this;
		}

		Entity GetEntity() const noexcept;

		inline bool IsCreatedByECS() const noexcept
		{
			if (m_InternalData == nullptr)
			{
				return false;
			}
			return m_InternalData->m_Flags.IsCreated();
		}

		inline bool IsEnabledByECS() const noexcept
		{
			if (m_InternalData == nullptr)
			{
				return false;
			}
			return m_InternalData->m_Flags.IsEnabled();
		}

		inline uint16_t GetDependecyCount() const
		{
			if (m_InternalData == nullptr)
			{
				return 0;
			}
			return m_InternalData->m_DependencyCount;
		}

		inline void AddDependency(uint16_t dependecyCount = 1)
		{
			if (m_InternalData != nullptr)
			{
				m_InternalData->m_DependencyCount++;
			}
		}

		inline void RemoveDependecy(uint16_t dependecyCount = 1)
		{
			if (m_InternalData != nullptr)
			{
				if (dependecyCount > m_InternalData->m_DependencyCount)
				{
					m_InternalData->m_DependencyCount = 0;
				}
				else
				{
					m_InternalData->m_DependencyCount -= dependecyCount;
				}
			}
		}

	protected:
		inline void SetInternalFlag(uint8_t flagIndex, bool bValue)
		{
			if (m_InternalData == nullptr)
			{
				return;
			}
			m_InternalData->m_Flags.SetBit(flagIndex, bValue);
		}

		inline bool GetInternalFlag(uint8_t flagIndex) const noexcept
		{
			if (m_InternalData == nullptr)
			{
				return false;
			}
			return m_InternalData->m_Flags.GetBit(flagIndex);
		}

	private:
		InternalComponentData* m_InternalData = nullptr;

	private:
		uint32_t GetIndexInAllocator() const
		{
			if (m_InternalData == nullptr)
			{
				return std::numeric_limits<uint32_t>::max();
			}
			return m_InternalData->m_IndexInAllocator;
		}

		bool SetIndexInAllocator(uint32_t index)
		{
			if (m_InternalData != nullptr)
			{
				m_InternalData->m_IndexInAllocator = index;
				return true;
			}
			return false;
		}

		bool SetEntityData(EntityData* entityData)
		{
			if (m_InternalData != nullptr)
			{
				m_InternalData->m_EntityData = entityData;
				return true;
			}
			return false;
		}

		inline void OnPreCreate(EntityData* entitydata)
		{
			if (m_InternalData != nullptr)
			{
				m_InternalData->m_EntityData = entitydata;
			}
		}

		inline void SetFlags(bool bIsCreated, bool bIsEnabled)
		{
			if (m_InternalData != nullptr)
			{
				m_InternalData->m_Flags.SetCreated(bIsCreated);
				m_InternalData->m_Flags.SetEnabled(bIsEnabled);
			}
		}

		inline void SetCreated(bool bIsCreated)
		{
			if (m_InternalData != nullptr)
			{
				m_InternalData->m_Flags.SetCreated(bIsCreated);
			}
		}

		inline void SetEnabled(bool bIsEnabled)
		{
			if (m_InternalData != nullptr)
			{
				m_InternalData->m_Flags.SetEnabled(bIsEnabled);
			}
		}

	};

	template<typename TComponentType>
	concept component_concept = std::derived_from<TComponentType, EntityComponent>;

	template<typename T>
	concept TComponentOrTagConcept = component_concept<T> || tag_concept<T>;

	template<component_concept... Types>
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