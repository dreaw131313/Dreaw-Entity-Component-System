namespace decs
{
	template<typename T>
	struct stable
	{
	};

	template<typename T>
	struct component_type
	{
	public:
		using Type = T;
	};

	template<typename T>
	struct component_type<stable<T>>
	{
	public:
		using Type = T;
	};

	template<typename T>
	struct is_stable
	{
	public:
		static constexpr bool value = false;
	};

	template<typename T>
	struct is_stable<stable<T>>
	{
	public:
		static constexpr bool value = true;
	};

	template<typename T>
	struct tag
	{
	};

	template<typename T>
	struct is_tag
	{
	public:
		static constexpr bool value = false;
	};

	template<typename T>
	struct is_tag<tag<T>>
	{
	public:
		static constexpr bool value = true;
	};

}
