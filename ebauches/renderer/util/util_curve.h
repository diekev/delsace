#include <algorithm>
#include <memory>
#include <vector>

#define GLM_FORCE_RADIANS
#include <glm/gtc/quaternion.hpp>

#include "util_math.h"

template <typename T>
class Curve {
public:
	using Ptr = std::shared_ptr<Curve>;
	using CPtr = std::shared_ptr<const Curve>;
	using Sample = std::pair<float, T>;
	using SampleVec = std::vector<Sample>;

	Curve() = default;

	Curve(const T &init_value)
	{
		addSample(0.0f, init_value);
	}

	Curve(const size_t num_samples, const T &init_value);

	static Ptr create()
	{
		return Ptr(new Curve);
	}

	void addSample(const float t, const T &value);
	T interpolate(const float t) const;
	size_t numSamples() const;
	const SampleVec &samples() const;
	std::vector<float> samplePoints() const;
	std::vector<T> sampleValues() const;
	void removeDuplicates();
	static CPtr average(const std::vector<CPtr> &curves);

private:
	struct CheckTGreaterThan : public std::unary_function<std::pair<float, T>, bool> {
		CheckTGreaterThan(float match)
			: m_match(match)
		{}

		bool operator()(const std::pair<float, T> &test)
		{
			return test.first > m_match;
		}

		float m_match;
	};

	static T defaultReturnValue()
	{
		return T(0);
	}

	static T lerp(const Sample &lower, const Sample &upper, const float t)
	{
		return glm::lerp(lower.second, upper.second, t);
	}

	SampleVec m_samples;
};

using FloatCurve = Curve<float>;
using ColorCurve = Curve<glm::vec4>;
using VectorCurve = Curve<glm::vec3>;
using QuatCurve = Curve<glm::fquat>;
using MatrixCurve = Curve<glm::mat4>;

template <typename T>
Curve<T>::Curve(const size_t num_samples, const T &init_value)
{
	for (size_t i = 0; i < num_samples; ++i) {
		addSample(static_cast<float>(i), init_value);
	}
}

template <typename T>
void Curve<T>::addSample(const float t, const T &value)
{
	typename SampleVec::iterator i = std::find_if(m_samples.begin(), m_samples.end(), CheckTGreaterThan(t));

	if (i != m_samples.end()) {
		m_samples.insert(i, std::make_pair(t, value));
	}
	else {
		m_samples.push_back(std::make_pair(t, value));
	}
}

template <typename T>
T Curve<T>::interpolate(const float t) const
{
	if (m_samples.size() == 0) {
		return defaultReturnValue();
	}

	typename SampleVec::const_iterator i = std::find_if(m_samples.begin(), m_samples.end(), CheckTGreaterThan(t));

	if (i == m_samples.end()) {
		return m_samples.back().second;
	}
	else if (i == m_samples.begin()){
		return m_samples.front().second;
	}

	const Sample &upper = *i;
	const Sample &lower = *(--i);
	const float fac = glm::lerp(t, lower.first, upper.first);

	return lerp(lower, upper, fac);
}

template <typename T>
std::vector<float> Curve<T>::samplePoints() const
{
	std::vector<float> result;

	for (const auto &sample : m_samples) {
		result.push_back(sample.first);
	}

	return result;
}

template <typename T>
std::vector<T> Curve<T>::sampleValues() const
{
	std::vector<T> result;

	for (const auto &sample : m_samples) {
		result.push_back(sample.second);
	}

	return result;
}

template <typename T>
void Curve<T>::removeDuplicates()
{
	SampleVec new_samples;
	auto size = m_samples.size();

	if (size == 1) {
		return;
	}

#if 0
	auto last = std::unique(m_samples.begin(), m_samples.end(),
	                        [](const SampleVec &s1, const SampleVec &s2)
							{
								return s1.second != s2.second;
							});

	m_samples.erase(last, m_samples.end());
#endif

	if (m_samples[0] != m_samples[1]) {
		new_samples.push_back(m_samples[0]);
	}

	for (size_t i = 1; i < size - 1; ++i) {
		if (m_samples[i].second != m_samples[i - 1].second ||
			m_samples[i].second != m_samples[i + 1].second) {
			new_samples.push_back(m_samples[i]);
		}
	}

	if (m_samples[size - 1] != m_samples[size - 2]) {
		new_samples.push_back(m_samples[size - 1]);
	}

	m_samples.swap(new_samples);
}

template <typename T>
typename Curve<T>::CPtr Curve<T>::average(const std::vector<typename Curve<T>::CPtr> &curves)
{
	typename Curve<T>::Ptr result(new Curve<T>);

	if (curves.size() == 0) {
		return result;
	}

	if (curves.size() == 1) {
		return curves[0];
	}

	/* Find first and last sample in all curves */
	auto first = std::numeric_limits<float>::max();
	auto last = std::numeric_limits<float>::min();
	auto num_samples = 0;

	for (const auto curve : curves) {
		first = std::min(first, curve->samplePoints().front());
		last = std::max(last, curve->samplePoints().back());
		num_samples = std::max(num_samples, curves->samplePoints().size());
	}

	/* Average curve */
	for (auto i(0); i < num_samples; ++i) {
		auto value = defaultReturnValue();
		auto t = math::fit(static_cast<float>(i), 0.0f, static_cast<float>(num_samples - 1), first, last);

		for (const auto curve : curves) {
			value += curve->interpolate(t);
		}

		value *= 1.0f / curves.size();
		result->addSample(t, value);
	}

	return result;
}

template <typename T>
size_t Curve<T>::numSamples() const
{
	return m_samples.size();
}

template <typename T>
const typename Curve<T>::SampleVec &Curve<T>::samples() const
{
	return m_samples;
}

template <>
inline glm::mat4 Curve<glm::mat4>::defaultReturnValue()
{
	return glm::mat4();
}

template <>
inline glm::fquat Curve<glm::fquat>::defaultReturnValue()
{
	return glm::fquat();
}

template <>
inline glm::fquat Curve<glm::fquat>::lerp(const Curve<glm::fquat>::Sample &lower,
                                          const Curve<glm::fquat>::Sample &upper,
                                          const float t)
{
	/* bug in glm, should be slerp, but slerp is calling mix */
	return glm::mix(lower.second, upper.second, t);
}

