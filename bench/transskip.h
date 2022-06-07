#ifndef __TRANSSKIP_H__
#define __TRANSSKIP_H__


#include <cstdint>
#include <iostream>
#include "common/assert.h"
#include "common/allocator.h"
#include "../tskiplist/TSkipList.h"

#define STRING_KV

#ifdef STRING_KV

#ifndef K_SZ
#define K_SZ 64
#endif

#ifndef V_SZ
#define V_SZ 1024
#endif

enum class sl_key_type {
    k_valid,
    k_invalid,
    k_min,
    k_max
};

template <uint64_t cap>
class sl_key_t {
    size_t size = 0;
    char s[cap];
    sl_key_type type = sl_key_type::k_valid;
public:
    sl_key_t<cap>(const std::string& str) : size(str.size()){
        assert(size<=cap);
        memcpy(s, str.c_str(), str.size()+1);
        assert(s[size] == '\0');
    }
    sl_key_t<cap>(const sl_key_t<cap>& oth){
        if (oth.type != sl_key_type::k_valid) {
            type = oth.type;
        } else {
            size = oth.size;
            assert(size<=cap);
            memcpy(s, oth.s, size+1);
            assert(s[size] == '\0');
        }
    }
    sl_key_t<cap>(const uint64_t n) {
        size = std::sprintf(s, "$d", n);
    }

    sl_key_t<cap>& operator = (const uint64_t n) {
        size = std::sprintf(s, "$d", n);
        return *this;
    }
    sl_key_t<cap>& operator = (const std::string& str) {
        size = str.size();
        assert(size<=cap);
        memcpy(s, str.c_str(), str.size()+1);
        assert(s[size] == '\0');
        return *this;
    }
    sl_key_t<cap>& operator = (const sl_key_type& t) {
        assert(t != sl_key_type::k_valid);
        type = t;
        return *this;
    }

    friend bool operator == (const sl_key_t<cap>& l, const sl_key_t<cap>& r) {
        assert(l.type != sl_key_type::k_invalid);
        assert(r.type != sl_key_type::k_invalid);
        if (l.type != r.type) {
            return false;
        } else if (l.type == sl_key_type::k_valid && 
            r.type == sl_key_type::k_valid) {
            return (strcmp(l.s, r.s) == 0);
        } else {
            return false;
        }
    }

    friend bool operator != (const sl_key_t<cap>& l, const sl_key_t<cap>& r) {
        return (!l == r);
    }
    
    friend bool operator<(const sl_key_t<cap>& l, const sl_key_t<cap>& r) {
        assert(l.type != sl_key_type::k_invalid);
        assert(r.type != sl_key_type::k_invalid);
        if (l.type == sl_key_type::k_valid &&
            r.type == sl_key_type::k_valid) {
            return (strcmp(l.s, r.s) < 0);
        } else if (l.type == r.type) {
            return false;
        } else if (l.type == sl_key_type::k_min ||
            r.type == sl_key_type::k_max) {
            return true;
        } else {
            return false;
        }
    }
};

template <uint64_t cap>
class sl_val_t{
    size_t size;
    char s[cap];
public:
    sl_val_t<cap>(const std::string& str) : size(str.size()){
        assert(size<=cap);
        memcpy(s, str.c_str(), str.size()+1);
        assert(s[size] == '\0');
    }
    sl_val_t<cap>(const sl_val_t<cap>& oth){
        size = oth.size;
        assert(size<=cap);
        memcpy(s, oth.s, size+1);
        assert(s[size] == '\0');
    }
    sl_val_t<cap>(const uint64_t n) {
        size = std::sprintf(s, "$d", n);
    }
    sl_val_t<cap>& operator = (const uint64_t n) {
        size = std::sprintf(s, "$d", n);
        return *this;
    }
    sl_val_t<cap>& operator = (const std::string& str) {
        size = str.size();
        assert(size<=cap);
        memcpy(s, str.c_str(), str.size()+1);
        assert(s[size] == '\0');
    }
};

typedef sl_key_t<K_SZ> setkey_t;
typedef sl_val_t<V_SZ> setval_t;

#else // STRING_KV

typedef uint64_t setkey_t;
typedef uint64_t setval_t;

#endif // STRING_KV


#ifdef __SET_IMPLEMENTATION__

/*************************************
 * INTERNAL DEFINITIONS
 */

/* Fine for 2^NUM_LEVELS nodes. */
#define NUM_LEVELS 20

#ifdef STRING_KV

/* Internal key values with special meanings. */
#define INVALID_FIELD    sl_key_type::k_invalid   /* Uninitialised field value.     */
#define SENTINEL_KEYMIN  sl_key_type::k_min       /* Key value of first dummy node. */
#define SENTINEL_KEYMAX  sl_key_type::k_max       /* Key value of last dummy node.  */


/*
 * Used internally be set access functions, so that callers can use
 * key values 0 and 1, without knowing these have special meanings.
 */
#define CALLER_TO_INTERNAL_KEY(_k) (_k)
#define INTERNAL_TO_CALLER_KEY(_k) (_k)

#else //STRING_KV

/* Internal key values with special meanings. */
#define INVALID_FIELD   (0)    /* Uninitialised field value.     */
#define SENTINEL_KEYMIN ( 1UL) /* Key value of first dummy node. */
#define SENTINEL_KEYMAX (~0UL) /* Key value of last dummy node.  */


/*
 * Used internally be set access functions, so that callers can use
 * key values 0 and 1, without knowing these have special meanings.
 */
#define CALLER_TO_INTERNAL_KEY(_k) ((_k) + 2)
#define INTERNAL_TO_CALLER_KEY(_k) ((_k) - 2)

#endif //STRING_KV

/*
 * SUPPORT FOR WEAK ORDERING OF MEMORY ACCESSES
 */

#ifdef WEAK_MEM_ORDER

/* Read field @_f into variable @_x. */
#define READ_FIELD(_x,_f)                                       \
do {                                                            \
    (_x) = (_f);                                                \
    if ( (_x) == INVALID_FIELD ) { RMB(); (_x) = (_f); }        \
    assert((_x) != INVALID_FIELD);                              \
} while ( 0 )

#else

/* Read field @_f into variable @_x. */
#define READ_FIELD(_x,_f) ((_x) = (_f))

#endif

#endif /* __SET_IMPLEMENTATION__ */

/*************************************
 * PUBLIC DEFINITIONS
 */

/*************************************
 * Transaction Definitions
 */

struct Operator
{
    uint8_t type;
    uint32_t key;
};

struct Desc
{
    static size_t SizeOf(uint8_t size)
    {
        return sizeof(uint8_t) + sizeof(uint8_t) + sizeof(Operator) * size;
    }

    volatile uint8_t status;
    uint8_t size;
    Operator ops[];
};

struct NodeDesc
{
    NodeDesc(Desc* _desc, uint8_t _opid)
        : desc(_desc), opid(_opid){}

    Desc* desc;
    uint8_t opid;
};


struct node_t
{
    int        level;
#define LEVEL_MASK     0x0ff
#define READY_FOR_FREE 0x100
    setkey_t k;
    setval_t v;

    NodeDesc* nodeDesc;

    node_t* next[1];
};

struct trans_skip
{
    Allocator<Desc>* descAllocator;
    Allocator<NodeDesc>* nodeDescAllocator;

    node_t* tail;
    node_t head;
};


/*
 * Key range accepted by set functions.
 * We lose three values (conveniently at top end of key space).
 *  - Known invalid value to which all fields are initialised.
 *  - Sentinel key values for up to two dummy nodes.
 */
#define KEY_MIN  ( 0U)
#define KEY_MAX  ((~0U) - 3)


void init_transskip_subsystem(void);
void destroy_transskip_subsystem(void);


bool execute_ops(SkipList &l, Desc* desc);

/*
 * Allocate an empty set.
 */
trans_skip *transskip_alloc(Allocator<Desc>* _descAllocator, Allocator<NodeDesc>* _nodeDescAllocator);

void  transskip_free(SkipList &l);

void ResetMetrics(SkipList &l);

#endif /* __SET_H__ */
