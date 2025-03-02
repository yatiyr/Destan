#pragma once
#include <unordered_map>
#include <core/defines.h>


namespace destan::core
{
	template<typename Key, typename Value>
	class Unordered_Map
	{
	public:
		Unordered_Map() = default;
		Unordered_Map(std::initializer_list<std::pair<Key, Value>> list) : m_map(list) {}

		using Iterator = typename std::unordered_map<Key, Value>::iterator;
		using ConstIterator = typename std::unordered_map<Key, Value>::const_iterator;

		inline Value& operator[](const Key& key) { return m_map[key]; }
		inline void Insert(const Key& key, const Value& value) { m_map[key] = value; }
		inline void Erase(const Key& key) { m_map.erase(key); }
		inline bool Contains(const Key& key) const { return m_map.find(key) != m_map.end(); }
		inline i64 Size() const { return m_map.size(); }
		inline bool Empty() const { return m_map.empty(); }

		inline Iterator Begin() { return m_map.begin(); }
		inline ConstIterator Begin() const { return m_map.begin(); }
		inline Iterator End() { return m_map.end(); }
		inline ConstIterator End() const { return m_map.end(); }

	private:
		std::unordered_map<Key, Value> m_map;
	};
}