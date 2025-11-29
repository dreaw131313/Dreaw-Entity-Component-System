#include "Component.h"

#include "decs/Entity.h"

namespace decs
{
	Entity EntityComponent::GetEntity() const noexcept
	{
		return Entity(m_EntityData);
	}

}