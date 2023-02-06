namespace decs
{
	template<typename T>
	struct Stable
	{
		using Type = T;
	};

	template<typename T>
	struct component_type
	{
		using Type = T;
	};

	template<typename T>
	struct component_type<Stable<T>>
	{
		using Type = T;
	};

	template<typename T>
	struct is_stable
	{
		static constexpr bool value = false;
	};

	template<typename T>
	struct is_stable<Stable<T>>
	{
		static constexpr bool value = true;
	};
}
