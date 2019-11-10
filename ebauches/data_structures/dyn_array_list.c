#include <stdlib.h>

typedef struct DynamicArray {
	unsigned int count;
	unsigned int max_item_index;
	unsigned int last_item_index;
	void **items;
} DynamicArray;

typedef struct ListBase {
	void *first, *last;
} ListBase;

typedef struct DynamicList {
	struct DynamicArray dyn_arr;
	struct ListBase lb;
} DynamicList;

DynamicList *BLI_dlist_from_listbase(ListBase *lb);

ListBase *BLI_listbase_from_dlist(DynamicList *dlist, ListBase *lb);

void *BLI_dlist_find_link(DynamicList *dlist, unsigned int index)
{
	if (dlist->dyn_arr.items[index] != NULL) {
		return dlist->dyn_arr.items[index];
	}

	return NULL;
}

void BLI_dlist_free_item(DynamicList *dlist, unsigned int index);

void BLI_dlist_rem_item(DynamicList *dlist, unsigned int index);

void BLI_dlist_add_item_index(DynamicList *dlist, void *item, unsigned int index);

void BLI_dlist_destroy(DynamicList *dlist);

void BLI_dlist_init(DynamicList *dlist);

void BLI_dlist_reinit(DynamicList *dlist);
