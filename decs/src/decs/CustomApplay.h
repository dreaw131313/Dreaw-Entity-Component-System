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

	/*template <class _Callable, class _Tuple, size_t... _Indices>
	constexpr decltype(auto) _Apply_impl(_Callable&& _Obj, _Tuple&& _Tpl, index_sequence<_Indices...>) noexcept(
		noexcept(_STD invoke(_STD forward<_Callable>(_Obj), _STD get<_Indices>(_STD forward<_Tuple>(_Tpl))...)))
	{
		return _STD invoke(_STD forward<_Callable>(_Obj), _STD get<_Indices>(_STD forward<_Tuple>(_Tpl))...);
	}

	template <class _Callable, class _Tuple>
	constexpr decltype(auto) apply(_Callable&& _Obj, _Tuple&& _Tpl) noexcept(
		noexcept(_Apply_impl(_STD forward<_Callable>(_Obj), _STD forward<_Tuple>(_Tpl),
							 make_index_sequence<tuple_size_v<remove_reference_t<_Tuple>>>{})))
	{
		return _Apply_impl(_STD forward<_Callable>(_Obj), _STD forward<_Tuple>(_Tpl),
						   make_index_sequence<tuple_size_v<remove_reference_t<_Tuple>>>{});
	}*/
}