#pragma once

#include <vector>
#include <unordered_map>
#include <atomic>
#include <mutex>

#ifdef STRING_KV

typedef std::string ItemType;
typedef std::string ValueType;

#else // STRING_KV

typedef int ItemType;
typedef int ValueType;

#endif // STRING_KV

class AbortTransactionException : public std::exception
{
};