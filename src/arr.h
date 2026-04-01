// COR24 APL Interpreter -- Array Data Structure
// Bump-allocated heap with rank/shape headers.
// Supports rank 0 (scalar), rank 1 (vector), rank 2 (matrix).
//
// Heap layout per array:
//   [idx+0] = rank  (0, 1, or 2)
//   [idx+1] = dim0  (length for vectors, rows for matrices)
//   [idx+2] = dim1  (cols for matrices, 0 otherwise)
//   [idx+3] = type  (0=numeric, 1=character, 2=boxed)
//   [idx+4..] = data elements (boxed: heap indices)

#pragma once

#define HEAP_SIZE 4096
#define ARR_HDR   4

#define ARR_NUM   0
#define ARR_CHAR  1
#define ARR_BOXED 2

int heap[HEAP_SIZE];
int heap_top;

// Reset heap (reclaim all arrays)
void arr_reset() {
    heap_top = 0;
}

// Allocate array. Returns heap index, or -1 if full.
// For rank 0: dim0=1, dim1=0 (one scalar element)
// For rank 1: dim0=length, dim1=0
// For rank 2: dim0=rows, dim1=cols
int arr_new(int rank, int dim0, int dim1) {
    int size;
    if (rank <= 1) {
        size = dim0;
    } else {
        size = dim0 * dim1;
    }
    int need = ARR_HDR + size;
    if (heap_top + need > HEAP_SIZE) return -1;

    int idx = heap_top;
    heap[idx]     = rank;
    heap[idx + 1] = dim0;
    heap[idx + 2] = dim1;
    heap[idx + 3] = ARR_NUM;
    heap_top = heap_top + need;

    // Zero-fill data
    int i = 0;
    while (i < size) {
        heap[idx + ARR_HDR + i] = 0;
        i++;
    }
    return idx;
}

// Accessors
int arr_rank(int idx) { return heap[idx]; }
int arr_dim0(int idx) { return heap[idx + 1]; }
int arr_dim1(int idx) { return heap[idx + 2]; }
int arr_type(int idx) { return heap[idx + 3]; }
void arr_set_type(int idx, int t) { heap[idx + 3] = t; }

// Total element count
int arr_size(int idx) {
    if (heap[idx] <= 1) return heap[idx + 1];
    return heap[idx + 1] * heap[idx + 2];
}

// Get/set data element by flat index
int arr_get(int idx, int i) {
    return heap[idx + ARR_HDR + i];
}

void arr_set(int idx, int i, int val) {
    heap[idx + ARR_HDR + i] = val;
}

// Create a scalar array holding one value
int arr_scalar(int val) {
    int idx = arr_new(0, 1, 0);
    if (idx < 0) return -1;
    arr_set(idx, 0, val);
    return idx;
}

// Create a vector from inline values (up to count elements)
int arr_vector(int count) {
    return arr_new(1, count, 0);
}
