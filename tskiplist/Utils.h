#pragma once

#include <vector>
#include <unordered_map>
#include <atomic>
#include <mutex>

#ifdef STRING_KV

typedef std::string ItemType;

#else // STRING_KV

typedef int ItemType;

#endif // STRING_KV

class AbortTransactionException : public std::exception
{
};