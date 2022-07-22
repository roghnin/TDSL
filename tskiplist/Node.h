#pragma once

#include "Utils.h"
#include "Mutex.h"
#include "skiplist/skiplist.h"

enum NodeType{MIN, NORMAL};

template<typename K, typename V>
class Node
{
public:
    Node(const K & k, const V & v, unsigned int version, NodeType type = NORMAL) :
        key(k), val(v), deleted(false), version(version), next(NULL), type(type)
    {
        skiplist_init_node(&snode);
    }

    Node(const K & k, unsigned int version, NodeType type = NORMAL) :
        key(k), deleted(false), version(version), next(NULL), type(type)
    {
        skiplist_init_node(&snode);
    }

    virtual ~Node() = default;

    bool isLocked()
    {
        return lock.isLocked();
    }

    skiplist_node snode;
    K key;
    V val;
    NodeType type;
    Node<K,V> * next;
    bool deleted;
    Mutex lock;
    unsigned int version;
};