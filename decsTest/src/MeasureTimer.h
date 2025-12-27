#pragma once

#include <chrono>

struct MeasureTimer final
{
public:
	MeasureTimer()
	{
		Start();
	}

	MeasureTimer(bool start)
	{
		if (start) Start();
	}

	~MeasureTimer()
	{

	}

	inline void Start()
	{
		m_StartTime = std::chrono::high_resolution_clock::now();
	}

	inline double ElapsedAsNanoseconds() const
	{
		auto  now = std::chrono::high_resolution_clock::now();
		return std::chrono::duration<double, std::chrono::nanoseconds::period>(now - m_StartTime).count();
	}

	inline double ElapsedAsMicroseconds() const
	{
		auto  now = std::chrono::high_resolution_clock::now();
		return std::chrono::duration<double, std::chrono::microseconds::period>(now - m_StartTime).count();
	}

	inline double ElapsedAsMilisecond() const
	{
		auto  now = std::chrono::high_resolution_clock::now();
		return std::chrono::duration<double, std::chrono::milliseconds::period>(now - m_StartTime).count();
	}

	inline double ElapsedAsSecond() const
	{
		auto  now = std::chrono::high_resolution_clock::now();
		return std::chrono::duration<double, std::chrono::seconds::period>(now - m_StartTime).count();
	}

private:
	std::chrono::steady_clock::time_point m_StartTime;
};