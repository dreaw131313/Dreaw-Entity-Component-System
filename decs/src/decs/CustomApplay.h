#pragma once
#include "Core.h"

namespace decs
{
	template <class Callable, class Tuple, std::size_t... Indices>
	constexpr decltype(auto) ApplayTupleWithPointersAsRefrencesImpl(Callable&& callable, Tuple&& tuple, std::index_sequence<Indices...>)noexcept(noexcept(std::invoke(std::forward<Callable>(callable), *std::get<Indices>(std::forward<Tuple>(tuple))...)))
	{
		return std::invoke(std::forward<Callable>(callable), *std::get<Indices>(std::forward<Tuple>(tuple))...);
	}

	template <class Callable, class Tuple>
	constexpr decltype(auto) ApplayTupleWithPointersAsRefrences(Callable&& callable, Tuple&& tuple)noexcept(
		noexcept(ApplayTupleWithPointersAsRefrencesImpl(std::forward<Callable>(callable), std::forward<Tuple>(tuple), std::make_index_sequence<std::tuple_size_v<std::remove_reference_t<Tuple>>>{})))
	{
		return ApplayTupleWithPointersAsRefrencesImpl(
			std::forward<Callable>(callable), std::forward<Tuple>(tuple),
			std::make_index_sequence<std::tuple_size_v<std::remove_reference_t<Tuple>>>{});
	}
}