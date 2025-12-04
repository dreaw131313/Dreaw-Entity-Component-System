#pragma once

#include <type_traits>

namespace decs
{
	template<typename T>
	struct tag final
	{
	public:
		using UnderlyingType = T;
	};

	template<typename T>
	struct is_tag final
	{
	public:
		inline static constexpr bool value = false;
	};

	template<typename T>
	struct is_tag<tag<T>> final
	{
	public:
		inline static constexpr bool value = true;
	};

	template<typename T>
	inline constexpr bool is_tag_v = is_tag<T>::value;

	template<typename... Ts>
	inline constexpr bool contain_tags_v = (is_tag_v<Ts> || ...);

	template<typename TTag>
	concept TTagConcept = is_tag_v<TTag>;


	template<typename T>
	struct drop_const
	{
	public:
		using Type = T;
	};

	template<typename T>
	struct drop_const<const T>
	{
	public:
		using Type = T;
	};

	template<typename T>
	using drop_const_t = drop_const<T>::Type;

	template<typename TCallable, typename... TComponentTypes>
	concept query_callable = std::is_invocable_v<TCallable, TComponentTypes...> 
		|| std::is_invocable_v<TCallable, Entity&, TComponentTypes...>
		|| std::is_invocable_v<TCallable, TComponentTypes&...>
		|| std::is_invocable_v<TCallable, Entity&, TComponentTypes&...>;


	template<typename TCallable, typename... TComponentTypes>
	constexpr bool is_invocable_with_entity_v = std::is_invocable_v<TCallable, Entity&, TComponentTypes...> 
		|| std::is_invocable_v<TCallable, Entity&, TComponentTypes&...>;

}