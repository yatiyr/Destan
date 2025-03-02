#pragma once
#include <string>
#include <core/defines.h>


namespace destan::core
{
	class String
	{
	public:
		String() = default;
		String(const char* str) : m_string(str) {}
		String(const std::string& str) : m_string(str) {}
		String(std::string&& str) noexcept : m_string(std::move(str)) {}

		using Iterator = typename std::string::iterator;
		using ConstIterator = typename std::string::const_iterator;

		inline i64 Size() const { return m_string.size(); }
		inline bool Empty() const { return m_string.empty(); }
		inline void Clear() { m_string.clear(); }

		inline const char* C_Str() const { return m_string.c_str(); }
		inline std::string& Std_String() { return m_string; }
		inline const std::string& Std_String() const { return m_string; }

		inline char& operator[](i64 index) { return m_string[index]; }
		inline const char& operator[](i64 index) const { return m_string[index]; }

		inline String operator+(const String& other) const { return m_string + other.m_string; }
		inline String& operator+=(const String& other) { m_string += other.m_string; return *this; }

		inline bool operator==(const String& other) const { return m_string == other.m_string; }
		inline bool operator!=(const String& other) const { return m_string != other.m_string; }

		inline Iterator Begin() { return m_string.begin(); }
		inline ConstIterator Begin() const { return m_string.begin(); }
		inline Iterator End() { return m_string.end(); }
		inline ConstIterator End() const { return m_string.end(); }

	private:
		std::string m_string;
	};
}