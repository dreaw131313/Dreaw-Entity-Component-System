#pragma once

#include "decs/Entity.h"

namespace decs
{
	class SpawnEntityCallback
	{
	public:
		/// <summary>
		/// This function is called after entity is created befor invoking any observer in Container.
		/// </summary>
		/// <param name="entity"></param>
		virtual void OnSpawnEntityCallback(const decs::Entity& entity) = 0;
	};
}