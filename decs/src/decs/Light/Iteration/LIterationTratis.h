#pragma once

#include "decs/Core/trait.h"
#include "decs/Light/Filter/LFilter.h"
#include "decs/Light/Component/LPackedComponentContainer.h"

namespace decs::light::iteration::trait
{
	template<typename T>
	struct query_data_container final
	{
	public:
		using data_type = pure_type_t<T>;
		using container_type = PackedLightComponentContainer<data_type>;
		inline static constexpr bool is_filter = false;
	};

	template<typename T>
	struct query_data_container<filter<T>> final
	{
	public:
		using container_type = FilterContainer<filter<T>>;
		inline static constexpr bool is_filter = true;
	};

	template<typename T>
	using query_data_container_t = typename query_data_container<T>::container_type;


	template<typename T>
	struct is_filter_container : public std::false_type {};
	template<typename T>
	struct is_filter_container<FilterContainer<T>> : public std::true_type {};
	template<typename T>
	inline constexpr bool is_filter_container_v = is_filter_container<T>::value;
	template< typename T>
	using tuple_if_filter_container = std::conditional_t<is_filter_container_v<T>, std::tuple<T*>, std::tuple<>>;

	template<typename T>
	struct is_packed_container : public std::false_type {};
	template<typename T>
	struct is_packed_container<PackedLightComponentContainer<T>> : public std::true_type {};
	template<typename T>
	inline constexpr bool is_packed_container_v = is_packed_container<T>::value;
	template< typename T>
	using tuple_if_packed_container = std::conditional_t<is_packed_container_v<T>, std::tuple<T*>, std::tuple<>>;

	template<typename... Ts>
	using create_filter_container_only_tuple = decltype(std::tuple_cat(std::declval<tuple_if_filter_container<Ts>>()...));
	template<typename... Ts>
	using create_packed_container_only_tuple = decltype(std::tuple_cat(std::declval<tuple_if_packed_container<Ts>>()...));

	template<typename Func, typename AditionalParam, typename... FiltersContainers>
	struct is_invocable_with_only_filters : public std::false_type
	{

	};

	template<typename Func, typename AditionalParam, typename... FiltersContainers>
	struct is_invocable_with_only_filters<Func, AditionalParam, std::tuple<FiltersContainers*...>> : public std::bool_constant<std::is_invocable_v<Func, const typename FiltersContainers::filter_data_type&..., AditionalParam>>
	{
	};

	template<typename Func, typename AditionalParam, typename... FiltersContainers>
	inline constexpr bool is_invocable_with_only_filters_v = is_invocable_with_only_filters<Func, AditionalParam, FiltersContainers...>::value;

	template<typename TCallable, typename... ComponentTypes>
	concept	light_query_iterate_container_callable = std::is_invocable_v<TCallable, typename query_data_container_t<ComponentTypes>::get_span_result...>;

	template<typename TCallable, typename... ComponentTypes>
	concept light_query_callable = std::is_invocable_v<TCallable, ligth_component_or_filter_t<ComponentTypes>...>
		|| std::is_invocable_v<TCallable, const light::Entity&, ligth_component_or_filter_t<ComponentTypes>...>
		|| std::is_invocable_v<TCallable, ligth_component_or_filter_t<ComponentTypes>&...>
		|| std::is_invocable_v<TCallable, const light::Entity&, ligth_component_or_filter_t<ComponentTypes>&...>;

	template<typename TCallable, typename... ComponentTypes>
	constexpr bool is_invocable_with_light_entity_v = std::is_invocable_v<TCallable, const light::Entity&, ligth_component_or_filter_t<ComponentTypes>...>
		|| std::is_invocable_v<TCallable, const light::Entity&, ligth_component_or_filter_t<ComponentTypes>&...>;

}