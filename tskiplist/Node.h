#pragma once

#include "Utils.h"
#include "Mutex.h"
#include "skiplist/skiplist.h"

class Node
{
public:
    Node(const ItemType & k, const ValueType & v, unsigned int version) :
        key(k), val(v), deleted(false), version(version), next(NULL)
    {
        skiplist_init_node(&snode);
    }

    Node(const ItemType & k, unsigned int version) :
        key(k), deleted(false), version(version), next(NULL)
    {
        skiplist_init_node(&snode);
    }

    virtual ~Node() = default;

    bool isLocked()
    {
        return lock.isLocked();
    }

    skiplist_node snode;
    ItemType key;
    ValueType val;
    Node * next;
    bool deleted;
    Mutex lock;
    unsigned int version;
};