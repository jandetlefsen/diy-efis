#ifndef __list_h__
#define __list_h__

/*-----------------------------------------------------------------------------
 INCLUDE HEADER FILES
-----------------------------------------------------------------------------*/

#include "../../ion.h"




#if defined( DBG )
#define list_assert(c)     fsm_assert3(c)
#else
#define list_assert(c)
#endif




/*-----------------------------------------------------------------------------
 DEFINE STRUCTURES
-----------------------------------------------------------------------------*/

typedef struct list_head_s{
   struct list_head_s *prev,
                    *next;

} list_head_t;




/*-----------------------------------------------------------------------------
 DEFINE FUNCTIONS
-----------------------------------------------------------------------------*/

/*
 Name: list_init
 Desc: Initialize a specific list.
 Params:
   - head: Pointer to a specific list to be initialized.
 Returns: None.
 Caveats: None.
*/

#define list_init(head) do { \
	(head)->next = (head);\
	(head)->prev = (head);\
} while (0)




/*
 Name: __list_add
 Desc: add an element at the specific list.
 Params:
   - prev: Pointer to a previous element.
   - _new: Pointer to an element to be added.
   - next: Pointer to a next element.
 Returns: None.
 Caveats: None.
*/

#define __list_add( _prev, _new, _next ) do { \
	list_head_t *link_next_node_point = _next;\
   (_prev)->next = (_new);\
   (_new)->prev = (_prev);\
   (_new)->next = link_next_node_point;\
   link_next_node_point->prev = (_new);\
} while (0)




/*
 Name: list_add
 Desc: add an element at the specific list.
 Params:
   - head: Pointer of element to point the position of list to add.
   - _new: Pointer to an element to be added.
 Returns: None.
 Caveats: None.
*/

#define list_add( head, _new ) do{ \
   __list_add( (head), (_new ), (head)->next );\
} while (0)




/*
 Name: list_add_tail
 Desc: add an element at a tail of specific list.
 Params:
   - head: Pointer of element to point the position of list to add.
   - _new: Pointer to an element to be added.
 Returns: None.
 Caveats: None.
*/

#define list_add_tail( head, _new  ) do{ \
   __list_add( (head)->prev, (_new ), (head) );\
} while (0)




/*
 Name: __list_del
 Desc: delete an element at the specific list.
 Params:
   - prev: Pointer to a previous element.
   - next: Pointer to a next element.
 Returns: None.
 Caveats: None.
*/

#define __list_del( _prev, _next ) do{ \
   (_prev)->next = (_next);\
   (_next)->prev = (_prev);\
} while (0)




/*
 Name: list_del
 Desc: delete an element at the specific list.
 Params:
   - entry: Pointer to an element to be deleted.
 Returns: None.
 Caveats: None.
*/

#define list_del( entry ) do{ \
   list_assert( NULL != (entry) );\
   __list_del( (entry)->prev, (entry)->next );\
   (entry)->prev =  NULL;\
   (entry)->next =  NULL;\
} while (0)




/*
 Name: list_del
 Desc: delete an element at the specific list.
 Params:
   - entry: Pointer to an element to be deleted.
 Returns: None.
 Caveats: It's no initialize the deleted element to NULL.
*/

#define list_del_just( entry ) do{ \
   list_assert( NULL != (entry) );\
   __list_del( (entry)->prev, (entry)->next );\
} while (0)




/*
 Name: list_del_init
 Desc: delete an element at the specific list and initialize the element.
 Params:
   - entry: Pointer to an element to be deleted.
 Returns: None.
 Caveats: It's initialize the deleted element to the type of linked-list.
*/

#define list_del_init( entry ) do{ \
   list_assert( NULL != (entry) );\
   __list_del( (entry)->prev, (entry)->next );\
   list_init( (entry) );\
} while (0)




/*
 Name: list_move
 Desc: Move an element to a head at a specific list.
 Params:
   - list: Pointer to a specific list, which a head is moved.
   - head: Pointer to an element to be moved.
 Returns: None.
 Caveats: None.
*/

#define list_move( list, head ) do{ \
   list_assert( NULL != (list) && NULL != (head) );\
   __list_del( (head)->prev, (head)->next );\
   list_add( (list), (head) );\
} while (0)




/*
 Name: list_move_tail
 Desc: Move an element to a tail at a specific list.
 Params:
   - list: Pointer to a specific list, which a head is moved.
   - head: Pointer to an element to be moved.
 Returns: None.
 Caveats: None.
*/

#define list_move_tail( list, head ) do{ \
   list_assert( NULL != (head)->prev && NULL != (head)->next );\
   __list_del( (head)->prev, (head)->next );\
   list_add_tail( (list), (head) );\
} while (0)




/*
 Name: list_is_empty
 Desc: Check whether a specific list is empty or not.
 Params:
   - list: Pointer to a list to be checked.
 Returns: None.
 Caveats: None.
*/

#define list_is_empty( list ) do{ \
   return (list)->next == (list);\
} while (0)




/*
Name: list_for_each
Desc: Iterate over a list
Params:
   pos: the &struct list_head_t to use as a loop counter.
   head: the head for your list.
*/

#define list_for_each(pos, head) \
   for ( (pos) = (head)->next; (pos) != (head); (pos) = (pos)->next )




/*
Name: list_entry
Desc: Get the struct for this entry
Params:
   ptr: the &struct list_head_t pointer.
   type: the type of the struct this is embedded in.
   member: the name of the list_struct within the struct.
*/

#define list_entry(ptr, type, member) \
        ((type*)((uint8_t *)ptr - (uint8_t *)&(((type*)0)->member)))


/*
Name: list_for_each_entry
Desc: Iterate over list of given type
Params:
   - pos: the type * to use as a loop counter.
   - head: the head for your list.
   - member: the name of the list_struct within the struct.
*/

#define list_for_each_entry(type, pos, head, member)\
   for ( pos = list_entry((head)->next, type, member);\
         &pos->member != (head);\
         pos = list_entry(pos->member.next, type, member) )




/*
Name: list_for_eash_entry_rev
Desc: Iterate backwards over list of given type.
Params:
   - pos: the type * to use as a loop counter.
   - head: the head for your list.
   - member: the name of the list_struct within the struct.
*/

#define list_for_each_entry_rev(type, pos, head, member)\
   for ( pos = list_entry((head)->prev, type, member);\
         &pos->member != (head);\
         pos = list_entry(pos->member.prev, type, member) )




/*
 Name: list_for_each_entry_safe
 Desc: Iterate over list of given type safe against removal of list entry
 Params:
   - pos: the type * to use as a loop counter.
   - n: another type * to use as temporary storage.
   - head: the head for your list.
   - member: the name of the list_struct within the struct.
 Caveats: None
*/

#define list_for_each_entry_safe(type, pos, n, head, member)\
   for ( pos = list_entry((head)->next, type, member), \
         n = list_entry(pos->member.next, type, member);\
         &pos->member != (head);\
         pos = n, n = list_entry(n->member.next, type, member) )




/*
 Name: list_for_eash_entry_safe_rev
 Desc: Iterate backwards over list of given type safe against removal of list
       entry.
 Params:
   - pos: the type * to use as a loop counter.
   - n: another type * to use as temporary storage.
   - head: the head for your list.
   - member: the name of the list_struct within the struct.
 Caveats: None
*/

#define list_for_each_entry_safe_rev(type, pos, n, head, member)\
   for ( pos = list_entry((head)->prev, type, member), \
         n = list_entry(pos->member.prev, type, member);\
         &pos->member != (head);\
         pos = n, n = list_entry(n->member.prev, type, member) )

#endif

/*-----------------------------------------------------------------------------
 END OF FILE
-----------------------------------------------------------------------------*/

