namespace decs
{
	template<typename T>
	class Stable
	{

	};

	template<typename T>
	struct is_stable_component
	{
		static constexpr bool value = false;
	};

	template<typename T>
	struct is_stable_component<Stable<T>>
	{
		static constexpr bool value = true;
	};
}
