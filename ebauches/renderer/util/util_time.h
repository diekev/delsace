#ifndef __TIME_H__
#define __TIME_H__

/**
 * General time class (in seconds)
 */
class Time {
	float m_value;

public:
	explicit Time(const float t)
		: m_value(t)
	{}

	operator float() const
	{
		return m_value;
	}

	float value() const
	{
		return m_value;
	}
};

/**
 * Time for shutter open/close
 */
class PTime {
	float m_value;

public:
	explicit PTime(const float t)
		: m_value(t)
	{}

	operator float() const
	{
		return m_value;
	}

	float value() const
	{
		return m_value;
	}
};

#endif /* __TIME_H__ */
