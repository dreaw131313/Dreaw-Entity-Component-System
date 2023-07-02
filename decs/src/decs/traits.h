namespace decs
{
	template<typename T>
	struct stable
	{
	};

	template<typename T>
	struct component_type
	{
		using Type = T;
	};

	template<typename T>
	struct component_type<stable<T>>
	{
		using Type = T;
	};

	template<typename T>
	struct is_stable
	{
		static constexpr bool value = false;
	};

	template<typename T>
	struct is_stable<stable<T>>
	{
		static constexpr bool value = true;
	};
}
