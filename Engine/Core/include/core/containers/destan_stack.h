#pragma once
#include <stack>
#include <core/defines.h>

namespace destan::core
{
	template<typename T>
	class Stack
	{
	public:
		Stack() = default;
		Stack(std::initializer_list<T> list) : m_stack(list) {}

		inline void Push(const T& value) { m_stack.push(value); }
		inline void Pop() { m_stack.pop(); }
		inline T& Top() { return m_stack.top(); }
		inline const T& Top() const { return m_stack.top(); }
		inline i64 Size() const { return m_stack.size(); }
		inline bool Empty() const { return m_stack.empty(); }
	private:
		std::stack<T> m_stack;
	};
}