#pragma once
#include <queue>
#include <core/defines.h>

namespace destan::core
{
	template<typename T>
	class Queue
	{
	public:
		Queue() = default;
		Queue(std::initializer_list<T> list) : m_queue(list) {}

		inline void Push(const T& value) { m_queue.push(value); }
		inline void Pop() { m_queue.pop(); }
		inline T& Front() { return m_queue.front(); }
		inline const T& Front() const { return m_queue.front(); }
		inline i64 Size() const { return m_queue.size(); }
		inline bool Empty() const { return m_queue.empty(); }
	private:
		std::queue<T> m_queue;
	};
}