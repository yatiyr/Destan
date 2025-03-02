#pragma once
#include <list>
#include <core/defines.h>

namespace destan::core
{
	template<typename T>
	class List
	{
	public:
		using Iterator = typename std::list<T>::iterator;
		using ConstIterator = typename std::list<T>::const_iterator;

		List() = default;
		List(std::initializer_list<T> list) : m_list(list) {}

		inline void Push_Back(const T& value) { m_list.push_back(value); }
		inline void Push_Front(const T& value) { m_list.push_front(value); }
		inline void Pop_Back() { m_list.pop_back(); }
		inline void Pop_Front() { m_list.pop_front(); }
		inline i64 Size() const { return m_list.size(); }
		inline bool Empty() const { return m_list.empty(); }

		inline Iterator Begin() { return m_list.begin(); }
		inline ConstIterator Begin() const { return m_list.begin(); }
		inline Iterator End() { return m_list.end(); }
		inline ConstIterator End() const { return m_list.end(); }
		inline T& operator[](i64 index) { return m_list[index]; }
		inline const T& operator[](i64 index) const { return m_list[index]; }
	private:
		std::list<T> m_list;
	};
}