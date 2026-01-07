#include "Component.h"

#include "decs/Normal/Entity.h"

namespace decs
{
	Entity EntityComponent::GetEntity() const noexcept
	{
		if (m_InternalData != nullptr)
		{
			return Entity(m_InternalData->m_EntityData);
		}
		return {};
	}

}