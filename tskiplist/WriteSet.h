#pragma once

#include "Utils.h"
#include "Node.h"
#include "SafeLock.h"

namespace tdsl {

template<typename K, typename V>
class Operation
{
public:
    Operation(Node<K,V> * next, bool deleted) : next(next), deleted(deleted) {}

    Node<K,V> * next;
    bool deleted;
};

template<typename K, typename V>
class WriteSet
{
public:
    void addItem(Node<K,V> * node, Node<K,V> * next, bool deleted){
        const auto it = items.find(node);
        if (it == items.end()) {
            items.insert(std::pair<Node<K,V> *, Operation<K,V>>(node, Operation<K,V>(next, deleted)));
        } else {
            if (next) {
                it->second.next = next;
            }
            if (deleted) {
                it->second.deleted = deleted;
            }
        }
    }

    bool getValue(Node<K,V> * node, Node<K,V> *& next, bool * deleted = NULL){
        auto it = items.find(node);
        if (it == items.end()) {
            return false;
        }

        if (deleted) {
            *deleted = it->second.deleted;
        }

        if (it->second.next != NULL) {
            next = it->second.next;
        } else {
            next = node->next;
        }

        return true;
    }

    // TODO: Make sure this doesn't lock/unlock due to copy construction
    bool tryLock(SafeLockList & locks){
        for (auto & it : items) {
            Node<K,V> * node = it.first;
            if (node->lock.tryLock()) {
                locks.add(node->lock);
            } else {
                return false;
            }
        }
        return true;
    }

    void update(unsigned int newVersion){
        for (auto & it : items) {
            Node<K,V> * n = it.first;
            Operation<K,V> & op = it.second;

            if (op.deleted) {
                n->deleted = op.deleted;
            }

            if (op.next) {
                n->next = op.next;
            }

            n->version = newVersion;
        }
    }

private:
    std::unordered_map<Node<K,V> *, Operation<K,V>> items;
};

} // namespace tdsl