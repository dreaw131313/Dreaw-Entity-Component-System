#pragma once

#include <type_traits>

namespace decs
{
	template<typename T>
	struct tag final
	{
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

}