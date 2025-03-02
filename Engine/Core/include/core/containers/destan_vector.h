#pragma once
#include <vector>
#include <core/defines.h>

namespace destan::core
{
	template<typename T>
	class Vector
	{
	public:
		using Iterator = typename std::vector<T>::iterator;
		using ConstIterator = typename std::vector<T>::const_iterator;

		Vector() = default;
		Vector(std::initializer_list<T> list) : m_vector(list) {}

		inline void Push_Back(const T& value) { m_vector.push_back(value); }
		inline void Pop_Back() { m_vector.pop_back(); }
		inline i64 Size() const { return m_vector.size(); }
		inline bool Empty() const { return m_vector.empty(); }

		inline Iterator Begin() { return m_vector.begin(); }
		inline ConstIterator Begin() const { return m_vector.begin(); }
		inline Iterator End() { return m_vector.end(); }
		inline ConstIterator End() const { return m_vector.end(); }

		inline T& operator[](i64 index) { return m_vector[index]; }
		inline const T& operator[](i64 index) const { return m_vector[index]; }
	private:
		std::vector<T> m_vector;
	};
}