// QuickSort.h - non-recursive quicksort over raw memory blocks (qsort style comparator).
#ifndef QUICKSORT_H
#define QUICKSORT_H

class QuickSort {
public:
    /** Comparator: > 0 when the first element must come after the second. */
    typedef int (*CompareFunc)(const void* left, const void* right);

    /** Swap two elements of `size` bytes (word by word, then the remaining bytes). */
    static void SwapValues(void* left, void* right, int size);
    /** Sort `count` elements of `size` bytes starting at `base`. */
    static void Run(void* base, int count, int size, CompareFunc compare);
};

#endif
