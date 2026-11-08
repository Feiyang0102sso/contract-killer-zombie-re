// QuickSort.cpp - non-recursive quicksort with an explicit range stack.
#include "QuickSort.h"

// Depth of the explicit range stack (the smaller half is always sorted first).
#define QUICKSORT_STACK_SIZE 32

void QuickSort::SwapValues(void* left, void* right, int size)
{
    char* first = (char*)left;
    char* second = (char*)right;
    int done;

    // Whole words first.
    for (done = sizeof(int); done <= size; done += sizeof(int)) {
        int word = *(int*)first;
        *(int*)first = *(int*)second;
        *(int*)second = word;
        first += sizeof(int);
        second += sizeof(int);
    }

    // Then the remaining bytes.
    for (done = done - sizeof(int) + 1; done <= size; done++) {
        char byte = *first;
        *first++ = *second;
        *second++ = byte;
    }
}

void QuickSort::Run(void* base, int count, int size, CompareFunc compare)
{
    char* lowStack[QUICKSORT_STACK_SIZE];
    char* highStack[QUICKSORT_STACK_SIZE];

    if (count < 1) {
        return;
    }

    char* low = (char*)base;
    char* high = (char*)base + (count - 1) * size;
    lowStack[0] = low;
    highStack[0] = high;
    int stackTop = 0;

    for (;;) {
        while (low < high) {
            // Middle element becomes the pivot at `low`.
            int half = (high - low) >> 1;
            char* middle = low + (half - half % (unsigned int)size);
            SwapValues(low, middle, size);

            char* left = low;
            char* right = high;
            for (;;) {
                do {
                    left += size;
                } while (left < right && compare(low, left) > 0);

                while (right >= left && compare(right, low) > 0) {
                    right -= size;
                }

                if (left >= right) {
                    break;
                }
                SwapValues(left, right, size);
                right -= size;
            }
            SwapValues(low, right, size);

            // Push the larger part, keep sorting the smaller one.
            if (right - low <= high - right) {
                if (right + size < high) {
                    lowStack[stackTop] = right + size;
                    highStack[stackTop] = high;
                    stackTop++;
                }
                high = right - size;
            } else {
                if (right - size > low) {
                    lowStack[stackTop] = low;
                    highStack[stackTop] = right - size;
                    stackTop++;
                }
                low = right + size;
            }
        }

        // Pop the next pending range.
        if (stackTop < 1) {
            break;
        }
        stackTop--;
        low = lowStack[stackTop];
        high = highStack[stackTop];
    }
}
