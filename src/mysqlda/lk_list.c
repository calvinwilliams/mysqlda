#include "lk_list.h"

void INIT_LK_LIST_HEAD(struct lk_list_head *list)
{
	WRITE_ONCE(list->next, list);
	list->prev = list;
}

/*
 * Insert a new entry between two known consecutive entries.
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
static void __lk_list_add(struct lk_list_head *new,
			      struct lk_list_head *prev,
			      struct lk_list_head *next)
{
	next->prev = new;
	new->next = next;
	new->prev = prev;
	WRITE_ONCE(prev->next, new);
}

/**
 * lk_list_add - add a new entry
 * @new: new entry to be added
 * @head: list head to add it after
 *
 * Insert a new entry after the specified head.
 * This is good for implementing stacks.
 */
void lk_list_add(struct lk_list_head *new, struct lk_list_head *head)
{
	__lk_list_add(new, head, head->next);
}


/**
 * lk_list_add_tail - add a new entry
 * @new: new entry to be added
 * @head: list head to add it before
 *
 * Insert a new entry before the specified head.
 * This is useful for implementing queues.
 */
void lk_list_add_tail(struct lk_list_head *new, struct lk_list_head *head)
{
	__lk_list_add(new, head->prev, head);
}

/*
 * Delete a list entry by making the prev/next entries
 * point to each other.
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
static void __lk_list_del(struct lk_list_head * prev, struct lk_list_head * next)
{
	next->prev = prev;
	WRITE_ONCE(prev->next, next);
}

/**
 * lk_list_del - deletes entry from list.
 * @entry: the element to delete from the list.
 * Note: lk_list_empty() on entry does not return true after this, the entry is
 * in an undefined state.
 */
static void __lk_list_del_entry(struct lk_list_head *entry)
{
	__lk_list_del(entry->prev, entry->next);
}

void lk_list_del(struct lk_list_head *entry)
{
	__lk_list_del(entry->prev, entry->next);
	entry->next = LK_LIST_POISON1;
	entry->prev = LK_LIST_POISON2;
}

/**
 * lk_list_replace - replace old entry by new one
 * @old : the element to be replaced
 * @new : the new element to insert
 *
 * If @old was empty, it will be overwritten.
 */
void lk_list_replace(struct lk_list_head *old, struct lk_list_head *new)
{
	new->next = old->next;
	new->next->prev = new;
	new->prev = old->prev;
	new->prev->next = new;
}

void lk_list_replace_init(struct lk_list_head *old, struct lk_list_head *new)
{
	lk_list_replace(old, new);
	INIT_LK_LIST_HEAD(old);
}

/**
 * lk_list_del_init - deletes entry from list and reinitialize it.
 * @entry: the element to delete from the list.
 */
void lk_list_del_init(struct lk_list_head *entry)
{
	__lk_list_del_entry(entry);
	INIT_LK_LIST_HEAD(entry);
}

/**
 * lk_list_move - delete from one list and add as another's head
 * @list: the entry to move
 * @head: the head that will precede our entry
 */
void lk_list_move(struct lk_list_head *list, struct lk_list_head *head)
{
	__lk_list_del_entry(list);
	lk_list_add(list, head);
}

/**
 * lk_list_move_tail - delete from one list and add as another's tail
 * @list: the entry to move
 * @head: the head that will follow our entry
 */
void lk_list_move_tail(struct lk_list_head *list, struct lk_list_head *head)
{
	__lk_list_del_entry(list);
	lk_list_add_tail(list, head);
}

/**
 * lk_list_is_last - tests whether @list is the last entry in list @head
 * @list: the entry to test
 * @head: the head of the list
 */
int lk_list_is_last(const struct lk_list_head *list, const struct lk_list_head *head)
{
	return list->next == head;
}

/**
 * lk_list_empty - tests whether a list is empty
 * @head: the list to test.
 */
int lk_list_empty(const struct lk_list_head *head)
{
	return READ_ONCE(head->next) == head;
}

/**
 * lk_list_empty_careful - tests whether a list is empty and not being modified
 * @head: the list to test
 *
 * Description:
 * tests whether a list is empty _and_ checks that no other CPU might be
 * in the process of modifying either member (next or prev)
 *
 * NOTE: using lk_list_empty_careful() without synchronization
 * can only be safe if the only activity that can happen
 * to the list entry is lk_list_del_init(). Eg. it cannot be used
 * if another CPU could re-lk_list_add() it.
 */
int lk_list_empty_careful(const struct lk_list_head *head)
{
	struct lk_list_head *next = head->next;
	return (next == head) && (next == head->prev);
}

/**
 * lk_list_rotate_left - rotate the list to the left
 * @head: the head of the list
 */
void lk_list_rotate_left(struct lk_list_head *head)
{
	struct lk_list_head *first;

	if (!lk_list_empty(head)) {
		first = head->next;
		lk_list_move_tail(first, head);
	}
}

/**
 * lk_list_is_singular - tests whether a list has just one entry.
 * @head: the list to test.
 */
int lk_list_is_singular(const struct lk_list_head *head)
{
	return !lk_list_empty(head) && (head->next == head->prev);
}

static void __lk_list_cut_position(struct lk_list_head *list,
		struct lk_list_head *head, struct lk_list_head *entry)
{
	struct lk_list_head *new_first = entry->next;
	list->next = head->next;
	list->next->prev = list;
	list->prev = entry;
	entry->next = list;
	head->next = new_first;
	new_first->prev = head;
}

/**
 * lk_list_cut_position - cut a list into two
 * @list: a new list to add all removed entries
 * @head: a list with entries
 * @entry: an entry within head, could be the head itself
 *	and if so we won't cut the list
 *
 * This helper moves the initial part of @head, up to and
 * including @entry, from @head to @list. You should
 * pass on @entry an element you know is on @head. @list
 * should be an empty list or a list you do not care about
 * losing its data.
 *
 */
void lk_list_cut_position(struct lk_list_head *list, struct lk_list_head *head, struct lk_list_head *entry)
{
	if (lk_list_empty(head))
		return;
	if (lk_list_is_singular(head) &&
		(head->next != entry && head != entry))
		return;
	if (entry == head)
		INIT_LK_LIST_HEAD(list);
	else
		__lk_list_cut_position(list, head, entry);
}

static void __lk_list_splice(const struct lk_list_head *list,
				 struct lk_list_head *prev,
				 struct lk_list_head *next)
{
	struct lk_list_head *first = list->next;
	struct lk_list_head *last = list->prev;

	first->prev = prev;
	prev->next = first;

	last->next = next;
	next->prev = last;
}

/**
 * lk_list_splice - join two lists, this is designed for stacks
 * @list: the new list to add.
 * @head: the place to add it in the first list.
 */
void lk_list_splice(const struct lk_list_head *list, struct lk_list_head *head)
{
	if (!lk_list_empty(list))
		__lk_list_splice(list, head, head->next);
}

/**
 * lk_list_splice_tail - join two lists, each list being a queue
 * @list: the new list to add.
 * @head: the place to add it in the first list.
 */
void lk_list_splice_tail(struct lk_list_head *list, struct lk_list_head *head)
{
	if (!lk_list_empty(list))
		__lk_list_splice(list, head->prev, head);
}

/**
 * lk_list_splice_init - join two lists and reinitialise the emptied list.
 * @list: the new list to add.
 * @head: the place to add it in the first list.
 *
 * The list at @list is reinitialised
 */
void lk_list_splice_init(struct lk_list_head *list, struct lk_list_head *head)
{
	if (!lk_list_empty(list)) {
		__lk_list_splice(list, head, head->next);
		INIT_LK_LIST_HEAD(list);
	}
}

/**
 * lk_list_splice_tail_init - join two lists and reinitialise the emptied list
 * @list: the new list to add.
 * @head: the place to add it in the first list.
 *
 * Each of the lists is a queue.
 * The list at @list is reinitialised
 */
void lk_list_splice_tail_init(struct lk_list_head *list, struct lk_list_head *head)
{
	if (!lk_list_empty(list)) {
		__lk_list_splice(list, head->prev, head);
		INIT_LK_LIST_HEAD(list);
	}
}

void INIT_LK_HLIST_NODE(struct lk_hlist_node *h)
{
	h->next = NULL;
	h->pprev = NULL;
}

int lk_hlist_unhashed(const struct lk_hlist_node *h)
{
	return !h->pprev;
}

int lk_hlist_empty(const struct lk_hlist_head *h)
{
	return !READ_ONCE(h->first);
}

static void __lk_hlist_del(struct lk_hlist_node *n)
{
	struct lk_hlist_node *next = n->next;
	struct lk_hlist_node **pprev = n->pprev;

	WRITE_ONCE(*pprev, next);
	if (next)
		next->pprev = pprev;
}

void lk_hlist_del(struct lk_hlist_node *n)
{
	__lk_hlist_del(n);
	n->next = LK_HLIST_POISON1;
	n->pprev = LK_HLIST_POISON2;
}

void lk_hlist_del_init(struct lk_hlist_node *n)
{
	if (!lk_hlist_unhashed(n)) {
		__lk_hlist_del(n);
		INIT_LK_HLIST_NODE(n);
	}
}

void lk_hlist_add_head(struct lk_hlist_node *n, struct lk_hlist_head *h)
{
	struct lk_hlist_node *first = h->first;
	n->next = first;
	if (first)
		first->pprev = &n->next;
	WRITE_ONCE(h->first, n);
	n->pprev = &h->first;
}

/* next must be != NULL */
void lk_hlist_add_before(struct lk_hlist_node *n, struct lk_hlist_node *next)
{
	n->pprev = next->pprev;
	n->next = next;
	next->pprev = &n->next;
	WRITE_ONCE(*(n->pprev), n);
}

void lk_hlist_add_behind(struct lk_hlist_node *n, struct lk_hlist_node *prev)
{
	n->next = prev->next;
	WRITE_ONCE(prev->next, n);
	n->pprev = &prev->next;

	if (n->next)
		n->next->pprev  = &n->next;
}

/* after that we'll appear to be on some hlist and lk_hlist_del will work */
void lk_hlist_add_fake(struct lk_hlist_node *n)
{
	n->pprev = &n->next;
}

int lk_hlist_fake(struct lk_hlist_node *h)
{
	return h->pprev == &h->next;
}

/*
 * Check whether the node is the only node of the head without
 * accessing head:
 */
int lk_hlist_is_singular_node(struct lk_hlist_node *n, struct lk_hlist_head *h)
{
	return !n->next && n->pprev == &h->first;
}

/*
 * Move a list from one list head to another. Fixup the pprev
 * reference of the first entry if it exists.
 */
void lk_hlist_move_list(struct lk_hlist_head *old, struct lk_hlist_head *new)
{
	new->first = old->first;
	if (new->first)
		new->first->pprev = &new->first;
	old->first = NULL;
}

