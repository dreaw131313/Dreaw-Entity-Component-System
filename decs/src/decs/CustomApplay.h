#pragma once
#include "decspch.h"
#include "Core.h"

namespace decs
{
	template <class F, class Tuple, std::size_t... I>
	constexpr decltype(auto) ApplayTupleWithPointersAsRefrencesImpl(F&& f, Tuple&& t, std::index_sequence<I...>)
	{
		return std::invoke(std::forward<F>(f), *std::get<I>(std::forward<Tuple>(t))...);
	}

	template <class F, class Tuple>
	constexpr decltype(auto) ApplayTupleWithPointersAsRefrences(F&& f, Tuple&& t)
	{
		return ApplayTupleWithPointersAsRefrencesImpl(
			std::forward<F>(f), std::forward<Tuple>(t),
			std::make_index_sequence<std::tuple_size_v<std::remove_reference_t<Tuple>>>{});
	}
}