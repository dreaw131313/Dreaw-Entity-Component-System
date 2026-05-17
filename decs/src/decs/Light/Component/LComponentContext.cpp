#include "LComponentContext.h"

namespace decs::light
{
	ComponentContextManager::~ComponentContextManager()
	{
		for (auto& [typeID, componentCtx] : m_Contexts)
		{
			delete componentCtx;
		}
	}

	IComponentContext* ComponentContextManager::GetOrCreateContext(const IComponentContext* referenceContext)
	{
		TypeID typeID = referenceContext->GetComponentTypeID();
		auto& ctx = m_Contexts[typeID];
		if (ctx == nullptr)
		{
			ctx = referenceContext->CreateMatchingContext();
		}

		return ctx;
	}
}