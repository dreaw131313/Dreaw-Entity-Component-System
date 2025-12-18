#include "Component.h"

#include "decs/Normal/Entity.h"

namespace decs
{
	Entity EntityComponent::GetEntity() const noexcept
	{
		return Entity(m_EntityData);
	}

}