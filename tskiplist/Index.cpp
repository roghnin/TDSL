#include "Index.h"
#include "SafeLock.h"

#ifdef STRING_KV

constexpr char MIN_VAL[] = "";

#else // STRING_KV

constexpr ItemType MIN_VAL = -2147483647;

#endif // STRING_KV




// long Index::sum()
// {
//     long sum = 0;
//     Node * n = head.next;
//     while (n != NULL) {
//         sum += n->key;
//         n = n->next;
//     }

//     return sum;
// }
