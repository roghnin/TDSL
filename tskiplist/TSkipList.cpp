#include "TSkipList.h"

#include "SafeLock.h"

void SkipList::TXBegin(SkipListTransaction & transaction)
{
    transaction.readVersion = gvc.read();
}

bool SkipList::contains(const ItemType & k, SkipListTransaction & transaction)
{
    Node * pred = NULL, *succ = NULL;
    traverseTo(k, transaction, pred, succ);

    return (succ != NULL && succ->key == k);
}

std::optional<ValueType> SkipList::get(const ItemType & k, SkipListTransaction & transaction)
{
    Node * pred = NULL, *succ = NULL;
    traverseTo(k, transaction, pred, succ);

    if (succ != NULL && succ->key == k) {
        return succ->val;
    } else {
        return {};
    }
}

bool SkipList::insert(const ItemType & k, const ValueType & v, SkipListTransaction & transaction)
{
    Node * pred = NULL, *succ = NULL;
    traverseTo(k, transaction, pred, succ);

    if (succ != NULL && succ->key == k) {
        return false;
    }

    Node * newNode = new Node(k, v, transaction.readVersion);
    newNode->next = succ;

    transaction.writeSet.addItem(pred, newNode, false);
    transaction.writeSet.addItem(newNode, NULL, false);
    transaction.indexTodo.push_back(IndexOperation(newNode, OperationType::INSERT));
    return true;
}

std::optional<ValueType> SkipList::put(const ItemType & k, const ValueType & v, SkipListTransaction & transaction)
{
    std::optional<ValueType> res = {};
    Node * pred = NULL, *succ = NULL;
    traverseTo(k, transaction, pred, succ);

    Node * newNode = new Node(k, v, transaction.readVersion);
    
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

bool SkipList::remove(const ItemType & k, SkipListTransaction & transaction)
{
    Node * pred = NULL, *succ = NULL;
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

Node * SkipList::getValidatedValue(SkipListTransaction & transaction,
                                   Node * node, bool * outDeleted)
{
    Node * res = NULL;
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

bool SkipList::validateReadSet(SkipListTransaction & transaction)
{
    for (auto n : transaction.readSet) {
        if (!getValidatedValue(transaction, n)) {
            return false;
        }
    }
    return true;
}

void SkipList::TXCommit(SkipListTransaction & transaction)
{
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

void SkipList::traverseTo(const ItemType & k, SkipListTransaction & transaction,
                          Node *& pred, Node *& succ)
{
    Node * startNode = index.getPrev(k);
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