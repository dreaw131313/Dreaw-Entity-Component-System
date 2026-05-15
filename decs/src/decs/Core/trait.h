#pragma once

#include <type_traits>

#include "Type.h"

namespace decs
{

#pragma region TUPLE TRAITS:
	template<typename T, typename... Ts>
	constexpr std::size_t type_index_v = []{
		std::size_t i = 0;
		((std::is_same_v<T, Ts> ? false : (++i, true)) && ...);
		return i;
	}();

	template<typename T, typename TupleType>
	struct tuple_has_type : public std::false_type
	{

	};

	template<typename T, typename... Ts>
	struct tuple_has_type<T, std::tuple<Ts...>> : public std::bool_constant<(std::is_same_v<T, Ts> || ...)>
	{

	};

	template<typename T, typename Ts>
	inline static constexpr bool tuple_has_type_v = tuple_has_type<T, Ts>::value;

#pragma endregion

#pragma region TAG

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
	concept tag_concept = is_tag_v<TTag>;

#pragma endregion

#pragma region FILTER

	template<typename T>
	struct filter
	{
	public:
		using UnderlyingType = T;
	};

	template<typename T>
	struct is_filter final
	{
	public:
		inline static constexpr bool value = false;
	};

	template<typename T>
	struct is_filter<filter<T>> final
	{
	public:
		inline static constexpr bool value = true;
	};

	template<typename T>
	inline constexpr bool is_filter_v = is_filter<T>::value;

	template<typename T>
	concept filter_concept = true;

#pragma endregion

	template<typename T>
	struct is_const
	{
	public:
		inline static constexpr bool value = false;
	};

	template<typename T>
	struct is_const<const T>
	{
	public:
		inline static constexpr bool value = true;
	};

	template<typename T>
	constexpr bool is_const_v = is_const<T>::value;

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

	template<typename T>
	using pure_type_t = std::remove_cvref_t<T>;

	class Archetype;
	class Entity;
	namespace light
	{
		class Entity;
	}

	template<typename Func>
	concept container_iterator_entity_func = std::is_invocable_v<Func, const Entity&>;

	template<typename Func>
	concept container_iterator_archetype_func = std::is_invocable_v<Func, const Archetype*>;

	template<typename T>
	concept light_component_concept = !std::is_same_v<T, bool> && !is_tag_v<T>;

	template<typename T>
	concept light_component_or_filter_concept = light_component_concept<T> || filter_concept<T>;

	template<typename T>
	concept light_component_or_tag_or_filter_concept = light_component_or_filter_concept<T> || tag_concept<T>;

	template<typename T>
	struct ligth_component_or_filter
	{
	public:
		using Type = T;
	};

	template<typename T>
	struct ligth_component_or_filter<filter<T>>
	{
	public:
		using Type = T;
	};

	template<typename T>
	using ligth_component_or_filter_t = ligth_component_or_filter<T>::Type;

	template<typename T>
	struct ligth_component_or_tag_or_filter
	{
	public:
		using Type = T;
	};

	template<typename T>
	struct ligth_component_or_tag_or_filter<filter<T>>
	{
	public:
		using Type = T;
	};

	template<typename T>
	using ligth_component_or_tag_or_filter_t = ligth_component_or_tag_or_filter<T>::Type;


	template<light_component_or_filter_concept... Types>
	class LightComponentTypeGroup
	{
	public:
		constexpr TypeID operator[](const uint64_t index) const
		{
			return m_Group[index];
		}

		constexpr uint64_t Size() const
		{
			return m_Group.Size();
		}

	private:
		TypeGroup<Types...> m_Group{};
	};

	template<tag_concept... Types>
	class TagTypeGroup
	{
	public:
		constexpr TypeID operator[](const uint64_t index) const
		{
			return m_Group[index];
		}

		constexpr uint64_t Size() const
		{
			return m_Group.Size();
		}

	private:
		TypeGroup<Types...> m_Group{};
	};

	template<typename TCallable, typename... TComponentTypes>
	concept query_callable = std::is_invocable_v<TCallable, TComponentTypes...>
		|| std::is_invocable_v<TCallable, const Entity&, TComponentTypes...>
		|| std::is_invocable_v<TCallable, TComponentTypes&...>
		|| std::is_invocable_v<TCallable, const Entity&, TComponentTypes&...>;

	template<typename TCallable, typename... TComponentTypes>
	constexpr bool is_invocable_with_entity_v = std::is_invocable_v<TCallable, const Entity&, TComponentTypes...>
		|| std::is_invocable_v<TCallable, const Entity&, TComponentTypes&...>;

	template<typename TCallable, typename... ComponentTypes>
	concept light_query_callable = std::is_invocable_v<TCallable, ligth_component_or_filter_t<ComponentTypes>...>
		|| std::is_invocable_v<TCallable, const light::Entity&, ligth_component_or_filter_t<ComponentTypes>...>
		|| std::is_invocable_v<TCallable, ligth_component_or_filter_t<ComponentTypes>&...>
		|| std::is_invocable_v<TCallable, const light::Entity&, ligth_component_or_filter_t<ComponentTypes>&...>;

	template<typename TCallable, typename... ComponentTypes>
	concept	light_query_iterate_container_callable = std::is_invocable_v<TCallable, std::span<ligth_component_or_filter_t<ComponentTypes>>...>
		|| std::is_invocable_v<TCallable, std::span<const ligth_component_or_filter_t<ComponentTypes>>...>
		|| std::is_invocable_v<TCallable, const std::span<ligth_component_or_filter_t<ComponentTypes>>...>
		|| std::is_invocable_v<TCallable, const std::span<ligth_component_or_filter_t<ComponentTypes>>&...>
		|| std::is_invocable_v<TCallable, const std::span<const ligth_component_or_filter_t<ComponentTypes>>...>
		|| std::is_invocable_v<TCallable, const std::span<const ligth_component_or_filter_t<ComponentTypes>>&...>
		;

	template<typename TCallable, typename... ComponentTypes>
	constexpr bool is_invocable_with_light_entity_v = std::is_invocable_v<TCallable, const light::Entity&, ligth_component_or_filter_t<ComponentTypes>...>
		|| std::is_invocable_v<TCallable, const light::Entity&, ligth_component_or_filter_t<ComponentTypes>&...>;

}