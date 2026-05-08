#pragma once

#include <type_traits>

#include "Type.h"

namespace decs
{

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
	concept filter_concept = is_filter_v<T>;

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
	struct pure_type
	{
	public:
		using Type = T;
	};

	template<typename T>
	struct pure_type<T&>
	{
	public:
		using Type = T;
	};

	template<typename T>
	struct pure_type<const T&>
	{
	public:
		using Type = T;
	};

	template<typename T>
	struct pure_type<const T>
	{
	public:
		using Type = T;
	};

	template<typename T>
	using pure_type_t = pure_type<T>::Type;

	class Archetype;
	class Entity;
	namespace light
	{
		class Entity;
	}

	template<typename TCallable, typename... TComponentTypes>
	concept query_callable = std::is_invocable_v<TCallable, TComponentTypes...>
		|| std::is_invocable_v<TCallable, const Entity&, TComponentTypes...>
		|| std::is_invocable_v<TCallable, TComponentTypes&...>
		|| std::is_invocable_v<TCallable, const Entity&, TComponentTypes&...>;

	template<typename TCallable, typename... TComponentTypes>
	concept light_query_callable = std::is_invocable_v<TCallable, TComponentTypes...>
		|| std::is_invocable_v<TCallable, const light::Entity&, TComponentTypes...>
		|| std::is_invocable_v<TCallable, TComponentTypes&...>
		|| std::is_invocable_v<TCallable, const light::Entity&, TComponentTypes&...>;

	template<typename TCallable, typename... TComponentTypes>
	concept	light_query_iterate_container_callable = std::is_invocable_v<TCallable, std::span<TComponentTypes>...>
		|| std::is_invocable_v<TCallable, std::span<const TComponentTypes>...>
		|| std::is_invocable_v<TCallable, const std::span<TComponentTypes>...>
		|| std::is_invocable_v<TCallable, const std::span<TComponentTypes>&...>
		|| std::is_invocable_v<TCallable, const std::span<const TComponentTypes>...>
		|| std::is_invocable_v<TCallable, const std::span<const TComponentTypes>&...>
		;

	template<typename TCallable, typename... TComponentTypes>
	constexpr bool is_invocable_with_entity_v = std::is_invocable_v<TCallable, const Entity&, TComponentTypes...>
		|| std::is_invocable_v<TCallable, const Entity&, TComponentTypes&...>;

	template<typename TCallable, typename... TComponentTypes>
	constexpr bool is_invocable_with_light_entity_v = std::is_invocable_v<TCallable, const light::Entity&, TComponentTypes...>
		|| std::is_invocable_v<TCallable, const light::Entity&, TComponentTypes&...>;

	template<typename Func>
	concept container_iterator_entity_func = std::is_invocable_v<Func, const Entity&>;

	template<typename Func>
	concept container_iterator_archetype_func = std::is_invocable_v<Func, const Archetype*>;

	template<typename T>
	concept TLightComponentConcept = !std::is_same_v<T, bool> && !is_tag_v<T>;

	template<typename T>
	concept TLightComponentOrTagConcept = TLightComponentConcept<T> || tag_concept<T>;

	template<TLightComponentConcept... Types>
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

}