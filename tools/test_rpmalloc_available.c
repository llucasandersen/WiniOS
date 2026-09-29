/* Exercise the existing upstream allocator's list transitions directly.
 * This test does not modify the dependency's source or supply repaired logic. */
#include <stddef.h>
#include <stdio.h>
#include <assert.h>
#include <limits.h>

/* The dependency's Windows diagnostic is unused on valid transitions, but
 * must link for the macOS host test. Record any unexpected diagnostic. */
static unsigned diagnostics;
static void *GetStdHandle(unsigned long handle) { (void)handle; return NULL; }
static int WriteFile(void *handle, const void *buffer, unsigned long length,
                     unsigned long *written, void *overlapped)
{
    (void)handle; (void)buffer; (void)overlapped;
    ++diagnostics; *written = length; return 1;
}
#include "../FEX/External/rpmalloc/rpmalloc/rpmalloc.c"

int main(void)
{
    heap_t *heap = calloc(1, sizeof(*heap));
    page_t *a = calloc(1, sizeof(*a)), *b = calloc(1, sizeof(*b));
    assert(heap && a && b);
    a->heap = b->heap = heap;
    a->size_class = b->size_class = 0;
    a->page_type = b->page_type = 0;
    global_page_free_overflow[0] = UINT_MAX;

    /* Reusing a full page must erase its old prev link before publishing it. */
    a->is_full = 1; a->prev = b;
    page_full_to_available(a);
    assert(heap->page_available[0] == a && !a->prev);
    assert(!rpm_avail_check(heap, 0, a, 1, "head-read"));
    b->is_full = 1; b->prev = a;
    page_full_to_available(b);
    assert(heap->page_available[0] == b && !b->prev && a->prev == b);
    page_available_to_full(b);
    assert(heap->page_available[0] == a && !a->prev && !b->prev && !b->next);
    assert(!rpm_avail_check(heap, 0, a, 1, "head-read"));

    /* A full page can become locally empty after adopting deferred frees.
     * It is already off the available list and must not unlink stale links. */
    b->prev = a; b->next = a;
    page_available_to_free(b);
    assert(heap->page_available[0] == a && !a->prev);
    assert(heap->page_free[0] == b && !b->prev && !b->is_full);
    page_available_to_free(a);
    assert(!heap->page_available[0] && heap->page_free[0] == a);
    assert(a->next == b && !a->prev && !diagnostics);
    free(a); free(b); free(heap);
    puts("PASS: upstream allocator head/prev transitions and full-page freeing");
    return 0;
}
