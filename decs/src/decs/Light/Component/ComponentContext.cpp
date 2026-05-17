#include "ComponentContext.h"



namespace decs::light
{
	ComponentContextManager::~ComponentContextManager()
	{
		for (auto& [typeID, componentCtx] : m_Contexts)
		{
			delete componentCtx;
		}
	}

	IComponentContext* ComponentContextManager::GetOrCreateContext(TypeID componentTypeID, const IComponentContext* referenceContext)
	{
		auto& ctx = m_Contexts[componentTypeID];
		if (ctx == nullptr)
		{
			ctx = referenceContext->CreateMatchingContext();
		}

		return ctx;
	}
}