#include <stddef.h>
#include <stdint.h>

#if GLIBC_VERSION < 212
// Not sure if we need to support older versions
#error "glibc version not supported"
#endif

#define INTERNAL_SIZE_T size_t
#define SIZE_SZ (sizeof (INTERNAL_SIZE_T))

#if defined(i386) && GLIBC_VERSION >= 226
// https://elixir.bootlin.com/glibc/glibc-2.26/source/sysdeps/i386/malloc-alignment.h#L22
#define MALLOC_ALIGNMENT       16
#else
#define MALLOC_ALIGNMENT (2 * SIZE_SZ < __alignof__ (long double) \
			  ? __alignof__ (long double) : 2 * SIZE_SZ)
#endif
#define MALLOC_ALIGN_MASK (MALLOC_ALIGNMENT - 1)
#define MIN_CHUNK_SIZE        (offsetof(struct malloc_chunk, fd_nextsize))
#define MINSIZE  \
  (unsigned long)(((MIN_CHUNK_SIZE+MALLOC_ALIGN_MASK) & ~MALLOC_ALIGN_MASK))

#define request2size(req)                                         \
  (((req) + SIZE_SZ + MALLOC_ALIGN_MASK < MINSIZE)  ?             \
   MINSIZE :                                                      \
   ((req) + SIZE_SZ + MALLOC_ALIGN_MASK) & ~MALLOC_ALIGN_MASK)
#define fastbin_index(sz) \
  ((((unsigned int) (sz)) >> (SIZE_SZ == 8 ? 4 : 3)) - 2)

#define MAX_FAST_SIZE     (80 * SIZE_SZ / 4)
#define NFASTBINS  (fastbin_index (request2size (MAX_FAST_SIZE)) + 1)
#define NBINS             128
#define BINMAPSHIFT      5
#define BITSPERMAP       (1U << BINMAPSHIFT)
#define BINMAPSIZE       (NBINS / BITSPERMAP)

# define TCACHE_MAX_BINS		64

struct malloc_chunk {

  INTERNAL_SIZE_T      mchunk_prev_size;  /* Size of previous chunk (if free).  */
  INTERNAL_SIZE_T      mchunk_size;       /* Size in bytes, including overhead. */

  struct malloc_chunk* fd;         /* double links -- used only if free. */
  struct malloc_chunk* bk;

  /* Only used for large blocks: pointer to next larger size.  */
  struct malloc_chunk* fd_nextsize; /* double links -- used only if free. */
  struct malloc_chunk* bk_nextsize;
};

typedef struct malloc_chunk* mchunkptr;
typedef struct malloc_chunk *mfastbinptr;

#if GLIBC_VERSION >= 227
// https://github.com/bminor/glibc/blob/glibc-2.27/malloc/malloc.c#L1674-L1716
struct malloc_state
{
  /* Serialize access.  */
  int mutext;

  /* Flags (formerly in max_fast).  */
  int flags;

  /* Set if the fastbin chunks contain recently inserted free blocks.  */
  /* Note this is a bool but not all targets support atomics on booleans.  */
  int have_fastchunks;

  /* Fastbins */
  mfastbinptr fastbinsY[NFASTBINS];

  /* Base of the topmost chunk -- not otherwise kept in a bin */
  mchunkptr top;

  /* The remainder from the most recent split of a small request */
  mchunkptr last_remainder;

  /* Normal bins packed as described above */
  mchunkptr bins[NBINS * 2 - 2];

  /* Bitmap of bins */
  unsigned int binmap[BINMAPSIZE];

  /* Linked list */
  struct malloc_state *next;

  /* Linked list for free arenas.  Access to this field is serialized
      by free_list_lock in arena.c.  */
  struct malloc_state *next_free;

  /* Number of threads attached to this arena.  0 if the arena is on
      the free list.  Access to this field is serialized by
      free_list_lock in arena.c.  */
  INTERNAL_SIZE_T attached_threads;

  /* Memory allocated from the system in this arena.  */
  INTERNAL_SIZE_T system_mem;
  INTERNAL_SIZE_T max_system_mem;
};
#elif GLIBC_VERSION >= 223
// https://github.com/bminor/glibc/blob/glibc-2.23/malloc/malloc.c#L1686-L1724
struct malloc_state
{
  /* Serialize access.  */
  int mutext;

  /* Flags (formerly in max_fast).  */
  int flags;

  /* Fastbins */
  mfastbinptr fastbinsY[NFASTBINS];

  /* Base of the topmost chunk -- not otherwise kept in a bin */
  mchunkptr top;

  /* The remainder from the most recent split of a small request */
  mchunkptr last_remainder;

  /* Normal bins packed as described above */
  mchunkptr bins[NBINS * 2 - 2];

  /* Bitmap of bins */
  unsigned int binmap[BINMAPSIZE];

  /* Linked list */
  struct malloc_state *next;

  /* Linked list for free arenas.  Access to this field is serialized
      by free_list_lock in arena.c.  */
  struct malloc_state *next_free;

  /* Number of threads attached to this arena.  0 if the arena is on
      the free list.  Access to this field is serialized by
      free_list_lock in arena.c.  */
  INTERNAL_SIZE_T attached_threads;

  /* Memory allocated from the system in this arena.  */
  INTERNAL_SIZE_T system_mem;
  INTERNAL_SIZE_T max_system_mem;
};
#else
// https://github.com/bminor/glibc/blob/glibc-2.12/malloc/malloc.c#L2362-L2400
struct malloc_state {
    /* Serialize access.  */
    int mutex;

    /* Flags (formerly in max_fast).  */
    int flags;

    /* Fastbins */
    mfastbinptr      fastbinsY[NFASTBINS];

    /* Base of the topmost chunk -- not otherwise kept in a bin */
    mchunkptr        top;

    /* The remainder from the most recent split of a small request */
    mchunkptr        last_remainder;

    /* Normal bins packed as described above */
    mchunkptr        bins[NBINS * 2 - 2];

    /* Bitmap of bins */
    unsigned int     binmap[BINMAPSIZE];

    /* Linked list */
    struct malloc_state *next;

    struct malloc_state *next_free;

    /* Memory allocated from the system in this arena.  */
    INTERNAL_SIZE_T system_mem;
    INTERNAL_SIZE_T max_system_mem;
};
#endif

typedef struct malloc_state *mstate;

#if GLIBC_VERSION >= 235
// https://github.com/bminor/glibc/blob/glibc-2.35/malloc/malloc.c#L1874-L1918
struct malloc_par
{
  /* Tunable parameters */
  unsigned long trim_threshold;
  INTERNAL_SIZE_T top_pad;
  INTERNAL_SIZE_T mmap_threshold;
  INTERNAL_SIZE_T arena_test;
  INTERNAL_SIZE_T arena_max;

  /* Transparent Large Page support.  */
  INTERNAL_SIZE_T thp_pagesize;
  /* A value different than 0 means to align mmap allocation to hp_pagesize
     add hp_flags on flags.  */
  INTERNAL_SIZE_T hp_pagesize;
  int hp_flags;

  /* Memory map support */
  int n_mmaps;
  int n_mmaps_max;
  int max_n_mmaps;
  /* the mmap_threshold is dynamic, until the user sets
     it manually, at which point we need to disable any
     dynamic behavior. */
  int no_dyn_threshold;

  /* Statistics */
  INTERNAL_SIZE_T mmapped_mem;
  INTERNAL_SIZE_T max_mmapped_mem;

  /* First address handed out by MORECORE/sbrk.  */
  char *sbrk_base;

  /* Maximum number of buckets to use.  */
  size_t tcache_bins;
  size_t tcache_max_bytes;
  /* Maximum number of chunks in each bucket.  */
  size_t tcache_count;
  /* Maximum number of chunks to remove from the unsorted list, which
     aren't used to prefill the cache.  */
  size_t tcache_unsorted_limit;
};
#elif GLIBC_VERSION >= 226
// https://github.com/bminor/glibc/blob/glibc-2.26/malloc/malloc.c#L1718-L1753
struct malloc_par
{
  /* Tunable parameters */
  unsigned long trim_threshold;
  INTERNAL_SIZE_T top_pad;
  INTERNAL_SIZE_T mmap_threshold;
  INTERNAL_SIZE_T arena_test;
  INTERNAL_SIZE_T arena_max;

  /* Memory map support */
  int n_mmaps;
  int n_mmaps_max;
  int max_n_mmaps;
  /* the mmap_threshold is dynamic, until the user sets
     it manually, at which point we need to disable any
     dynamic behavior. */
  int no_dyn_threshold;

  /* Statistics */
  INTERNAL_SIZE_T mmapped_mem;
  INTERNAL_SIZE_T max_mmapped_mem;

  /* First address handed out by MORECORE/sbrk.  */
  char *sbrk_base;

  /* Maximum number of buckets to use.  */
  size_t tcache_bins;
  size_t tcache_max_bytes;
  /* Maximum number of chunks in each bucket.  */
  size_t tcache_count;
  /* Maximum number of chunks to remove from the unsorted list, which
     aren't used to prefill the cache.  */
  size_t tcache_unsorted_limit;
};
#elif GLIBC_VERSION >= 224
// https://github.com/bminor/glibc/blob/glibc-2.24/malloc/malloc.c#L1719-L1743
struct malloc_par
{
  /* Tunable parameters */
  unsigned long trim_threshold;
  INTERNAL_SIZE_T top_pad;
  INTERNAL_SIZE_T mmap_threshold;
  INTERNAL_SIZE_T arena_test;
  INTERNAL_SIZE_T arena_max;

  /* Memory map support */
  int n_mmaps;
  int n_mmaps_max;
  int max_n_mmaps;
  /* the mmap_threshold is dynamic, until the user sets
     it manually, at which point we need to disable any
     dynamic behavior. */
  int no_dyn_threshold;

  /* Statistics */
  INTERNAL_SIZE_T mmapped_mem;
  INTERNAL_SIZE_T max_mmapped_mem;

  /* First address handed out by MORECORE/sbrk.  */
  char *sbrk_base;
};
#elif GLIBC_VERSION >= 215
// https://github.com/bminor/glibc/blob/glibc-2.15/malloc/malloc.c#L1829-L1857
struct malloc_par {
  /* Tunable parameters */
  unsigned long    trim_threshold;
  INTERNAL_SIZE_T  top_pad;
  INTERNAL_SIZE_T  mmap_threshold;
  INTERNAL_SIZE_T  arena_test;
  INTERNAL_SIZE_T  arena_max;

  /* Memory map support */
  int              n_mmaps;
  int              n_mmaps_max;
  int              max_n_mmaps;
  /* the mmap_threshold is dynamic, until the user sets
     it manually, at which point we need to disable any
     dynamic behavior. */
  int              no_dyn_threshold;

  /* Statistics */
  INTERNAL_SIZE_T  mmapped_mem;
  /*INTERNAL_SIZE_T  sbrked_mem;*/
  /*INTERNAL_SIZE_T  max_sbrked_mem;*/
  INTERNAL_SIZE_T  max_mmapped_mem;
  INTERNAL_SIZE_T  max_total_mem; /* only kept for NO_THREADS */

  /* First address handed out by MORECORE/sbrk.  */
  char*            sbrk_base;
};
#else
// https://github.com/bminor/glibc/blob/glibc-2.12/malloc/malloc.c#L2402-L2433
struct malloc_par {
  /* Tunable parameters */
  unsigned long    trim_threshold;
  INTERNAL_SIZE_T  top_pad;
  INTERNAL_SIZE_T  mmap_threshold;
  INTERNAL_SIZE_T  arena_test;
  INTERNAL_SIZE_T  arena_max;

  /* Memory map support */
  int              n_mmaps;
  int              n_mmaps_max;
  int              max_n_mmaps;
  /* the mmap_threshold is dynamic, until the user sets
     it manually, at which point we need to disable any
     dynamic behavior. */
  int              no_dyn_threshold;

  /* Cache malloc_getpagesize */
  unsigned int     pagesize;

  /* Statistics */
  INTERNAL_SIZE_T  mmapped_mem;
  /*INTERNAL_SIZE_T  sbrked_mem;*/
  /*INTERNAL_SIZE_T  max_sbrked_mem;*/
  INTERNAL_SIZE_T  max_mmapped_mem;
  INTERNAL_SIZE_T  max_total_mem; /* only kept for NO_THREADS */

  /* First address handed out by MORECORE/sbrk.  */
  char*            sbrk_base;
};
#endif

#if GLIBC_VERSION >= 235
// https://github.com/bminor/glibc/blob/glibc-2.35/malloc/arena.c#L75-L87
typedef struct _heap_info
{
  mstate ar_ptr; /* Arena for this heap. */
  struct _heap_info *prev; /* Previous heap. */
  size_t size;   /* Current size in bytes. */
  size_t mprotect_size; /* Size in bytes that has been mprotected
                           PROT_READ|PROT_WRITE.  */
  size_t pagesize; /* Page size used when allocating the arena.  */
  /* Make sure the following data is properly aligned, particularly
     that sizeof (heap_info) + 2 * SIZE_SZ is a multiple of
     MALLOC_ALIGNMENT. */
  char pad[-3 * SIZE_SZ & MALLOC_ALIGN_MASK];
} heap_info;
#else
// https://github.com/bminor/glibc/blob/glibc-2.12/malloc/arena.c#L59-L69
typedef struct _heap_info
{
    mstate ar_ptr; /* Arena for this heap. */
    struct _heap_info *prev; /* Previous heap. */
    size_t size;   /* Current size in bytes. */
    size_t mprotect_size; /* Size in bytes that has been mprotected
                            PROT_READ|PROT_WRITE.  */
    /* Make sure the following data is properly aligned, particularly
        that sizeof (heap_info) + 2 * SIZE_SZ is a multiple of
        MALLOC_ALIGNMENT. */
    char pad[-6 * SIZE_SZ & MALLOC_ALIGN_MASK];
} heap_info;
#endif

#if GLIBC_VERSION >= 234
// https://github.com/bminor/glibc/blob/glibc-2.34/malloc/malloc.c#L3013-L3018
typedef struct tcache_entry
{
  struct tcache_entry *next;
  /* This field exists to detect double frees.  */
  uintptr_t key;
} tcache_entry;
#elif GLIBC_VERSION >= 229
// https://github.com/bminor/glibc/blob/glibc-2.29/malloc/malloc.c#L2904-L2909
typedef struct tcache_entry
{
  struct tcache_entry *next;
  /* This field exists to detect double frees.  */
  struct tcache_perthread_struct *key;
} tcache_entry;
#elif GLIBC_VERSION >= 226
// https://github.com/bminor/glibc/blob/glibc-2.26/malloc/malloc.c#L2927-L2930
typedef struct tcache_entry
{
  struct tcache_entry *next;
} tcache_entry;
#endif

#if GLIBC_VERSION >= 230
// https://github.com/bminor/glibc/blob/glibc-2.30/malloc/malloc.c#L2906-L2910
typedef struct tcache_perthread_struct
{
  uint16_t counts[TCACHE_MAX_BINS];
  tcache_entry *entries[TCACHE_MAX_BINS];
} tcache_perthread_struct;
#elif GLIBC_VERSION >= 226
// https://github.com/bminor/glibc/blob/glibc-2.26/malloc/malloc.c#L2937-L2941
typedef struct tcache_perthread_struct
{
  char counts[TCACHE_MAX_BINS];
  tcache_entry *entries[TCACHE_MAX_BINS];
} tcache_perthread_struct;
#endif
