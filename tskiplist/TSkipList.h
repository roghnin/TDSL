#pragma once

#include <optional>
#include "Utils.h"
#include "WriteSet.h"
#include "Node.h"
#include "GVC.h"
#include "Index.h"

template<typename K, typename V>
class SkipListTransaction
{
public:
    SkipListTransaction() {}

    virtual ~SkipListTransaction() {}

    unsigned int readVersion;
    unsigned int writeVersion;
    std::vector<Node<K,V> *> readSet;
    WriteSet<K,V> writeSet;
    std::vector<IndexOperation<K,V>> indexTodo;
};

template<typename K, typename V>
class SkipList
{
public:
    SkipList() : index(gvc.read()) {}

    virtual ~SkipList() {}

    void TXBegin(SkipListTransaction<K,V> & transaction){
        transaction.readVersion = gvc.read();
    }

    bool contains(const K & k, SkipListTransaction<K,V> & transaction){
        Node<K,V> * pred = NULL, *succ = NULL;
        traverseTo(k, transaction, pred, succ);
        return (succ != NULL && succ->key == k);
    }

    std::optional<V> get(const K & k, SkipListTransaction<K,V> & transaction){
        Node<K,V> * pred = NULL, *succ = NULL;
        traverseTo(k, transaction, pred, succ);

        if (succ != NULL && succ->key == k) {
            return succ->val;
        } else {
            return {};
        }
    }

    bool insert(const K & k, const K & v, SkipListTransaction<K,V> & transaction){
        Node<K,V> * pred = NULL, *succ = NULL;
        traverseTo(k, transaction, pred, succ);

        if (succ != NULL && succ->key == k) {
            return false;
        }

        Node<K,V> * newNode = new Node<K,V>(k, v, transaction.readVersion);
        newNode->next = succ;

        transaction.writeSet.addItem(pred, newNode, false);
        transaction.writeSet.addItem(newNode, NULL, false);
        transaction.indexTodo.push_back(IndexOperation(newNode, OperationType::INSERT));
        return true;
    }
    
    std::optional<V> put(const K & k, const V & v, SkipListTransaction<K,V> & transaction){
        std::optional<ValueType> res = {};
        Node<K,V> * pred = NULL, *succ = NULL;
        traverseTo(k, transaction, pred, succ);

        Node<K,V> * newNode = new Node<K,V>(k, v, transaction.readVersion);
        
        if (succ != NULL && succ->key == k) {
            res = succ->val;
            transaction.readSet.push_back(succ);
            newNode->next = getValidatedValue(transaction, succ);
            transaction.writeSet.addItem(succ, NULL, true);
            transaction.writeSet.addItem(pred, newNode, false);
            transaction.indexTodo.push_back(IndexOperation(succ, OperationType::REMOVE));
        } else {
            newNode->next = succ;
            transaction.writeSet.addItem(pred, newNode, false);
            transaction.writeSet.addItem(newNode, NULL, false);
        }
        transaction.indexTodo.push_back(IndexOperation(newNode, OperationType::INSERT));
        return res;
    }

    bool remove(const K & k, SkipListTransaction<K,V> & transaction){
        Node<K,V> * pred = NULL, *succ = NULL;
        traverseTo(k, transaction, pred, succ);

        if (succ == NULL || succ->key != k) {
            return false;
        }

        transaction.readSet.push_back(succ);

        transaction.writeSet.addItem(pred, getValidatedValue(transaction, succ), false);
        transaction.writeSet.addItem(succ, NULL, true);
        transaction.indexTodo.push_back(IndexOperation(succ, OperationType::REMOVE));
        return true;
    }

    Node<K,V> * getValidatedValue(SkipListTransaction<K,V> & transaction, Node<K,V> * node,
                             bool * outDeleted = NULL){
        Node<K,V> * res = NULL;
        if (node->isLocked()) {
            throw AbortTransactionException();
        }

        if (!transaction.writeSet.getValue(node, res, outDeleted)) {
            res = node->next;

            if (outDeleted) {
                *outDeleted = node->deleted;
            }
        }

        if (node->version > transaction.readVersion) {
            throw AbortTransactionException();
        }

        if (node->isLocked()) {
            throw AbortTransactionException();
        }

        return res;
    }

    bool validateReadSet(SkipListTransaction<K,V> & transaction){
        for (auto n : transaction.readSet) {
            if (!getValidatedValue(transaction, n)) {
                return false;
            }
        }
        return true;
    }

    void TXCommit(SkipListTransaction<K,V> & transaction){
        {
            SafeLockList locks;
            if (!transaction.writeSet.tryLock(locks)) {
                throw AbortTransactionException();
            }

            if (!validateReadSet(transaction)) {
                throw AbortTransactionException();
            }

            transaction.writeVersion = gvc.addAndFetch();
            transaction.writeSet.update(transaction.writeVersion);
        }
        index.update(transaction.indexTodo);
    }

    void traverseTo(const K & k, SkipListTransaction<K,V> & transaction,
                    Node<K,V> *& pred, Node<K,V> *& succ){
        Node<K,V> * startNode = index.getPrev(k);
        bool deleted = false;
        succ = getValidatedValue(transaction, startNode, &deleted);
        while (startNode->isLocked() || deleted) {
            startNode = index.getPrev(startNode->key);
            succ = getValidatedValue(transaction, startNode, &deleted);
        }

        pred = startNode;
        deleted = false;
        while (succ != NULL && (succ->key < k || deleted)) {
            pred = succ;
            succ = getValidatedValue(transaction, pred, &deleted);
        }
        transaction.readSet.push_back(pred);
    }

    GVC gvc;
    Index<K,V> index;
};
