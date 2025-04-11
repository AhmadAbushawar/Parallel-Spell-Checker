# Parallel Spell Checker

A high-performance spell checker implementation with parallel processing capabilities using both OpenMP and Pthreads.

## Overview

This project implements a spell checker that can process large documents efficiently by leveraging parallel processing. The spell checker identifies misspelled words and suggests corrections based on the Levenshtein edit distance algorithm. It includes two parallel implementations:

- OpenMP-based implementation (`spellchecker_omp.c`)
- Pthreads-based implementation (`spellchecker_pthread.c`)

Both implementations share common functionality defined in `common.c` and `common.h`.

## Features

- Fast dictionary loading with binary search for word lookup
- Parallel processing of input documents
- Intelligent correction suggestions using edit distance algorithm
- Configurable suggestion threshold and result limits
- Thread count customization for optimal performance

## Workflow

1. **Dictionary Loading**: The program loads a dictionary file (`words.txt`) containing valid words.
2. **Dictionary Sorting**: The dictionary is sorted alphabetically to enable efficient binary search.
3. **Document Loading**: The input document (`input.txt`) is loaded and tokenized into individual words.
4. **Parallel Spell Checking**:
   - The input words are distributed among multiple threads
   - Each thread checks if its assigned words exist in the dictionary
   - Words not found in the dictionary are marked as misspelled
5. **Parallel Suggestion Generation**:
   - For each misspelled word, the program searches the dictionary for similar words
   - The dictionary search is distributed among multiple threads
   - Each thread applies length filtering and edit distance calculations
   - Candidate words within the edit distance threshold are collected
6. **Results Collection and Display**:
   - Suggestions from all threads are merged and sorted
   - The top suggestions (sorted by edit distance, then alphabetically) are displayed

## Algorithm: Levenshtein Edit Distance

The program uses the Levenshtein edit distance algorithm to find similar words for correction suggestions. This algorithm measures the minimum number of single-character operations (insertions, deletions, or substitutions) required to change one word into another.

### How it works:

1. Create a matrix of size (len1+1) × (len2+1), where len1 and len2 are the lengths of the two strings
2. Initialize the first row and column with incremental values (0, 1, 2, ...)
3. For each cell (i,j) in the matrix, calculate:
   - If characters match, cost = 0; otherwise, cost = 1
   - Calculate the minimum of:
     - Deletion: matrix[i-1][j] + 1
     - Insertion: matrix[i][j-1] + 1
     - Substitution: matrix[i-1][j-1] + cost
4. The value in the bottom-right cell represents the edit distance

## Performance Analysis

### Methodology

Performance tests were conducted with a dictionary of approximately 466,000 words and input documents of varying sizes. The tests measured execution time while varying the number of threads from 1 to 32.

### Results

#### OpenMP Execution Time (seconds)

| Threads | Real Time | User Time | System Time | Speedup (Real) |
|---------|-----------|-----------|-------------|----------------|
| 1       | 3.62      | 3.47      | 0.02        | 1.00×          |
| 2       | 2.05      | 3.66      | 0.01        | 1.77×          |
| 4       | 1.22      | 3.87      | 0.02        | 2.97×          |
| 8       | 0.95      | 5.22      | 0.01        | 3.81×          |
| 16      | 0.72      | 7.58      | 0.03        | 5.03×          |
| 32      | 0.68      | 5.78      | 0.04        | 5.32×          |

#### Pthreads Execution Time (seconds)

| Threads | Real Time | User Time | System Time | Speedup (Real) |
|---------|-----------|-----------|-------------|----------------|
| 1       | 3.68      | 3.45      | 0.01        | 1.00×          |
| 2       | 2.15      | 3.43      | 0.03        | 1.71×          |
| 4       | 1.28      | 3.40      | 0.01        | 2.88×          |
| 8       | 0.94      | 3.38      | 0.03        | 3.91×          |
| 16      | 0.76      | 3.92      | 0.04        | 4.84×          |
| 32      | 0.72      | 2.97      | 0.06        | 5.11×          |

#### Analysis

- Both implementations show good scaling up to 16 threads, with diminishing returns beyond that point
- OpenMP consistently outperforms Pthreads by a small margin (approximately 2-5%)
- Notable observations:
  - Despite real time decreasing with more threads, user time increases in OpenMP implementation, indicating higher CPU utilization across all cores
  - Pthreads maintains more consistent user time across different thread counts
  - System time remains minimal across all configurations, suggesting efficient system resource usage
  - Peak speedup of 5.32× with OpenMP and 5.11× with Pthreads using 32 threads

 ## Build and Usage

### Prerequisites

- GCC compiler with OpenMP support
- POSIX threads library
- Standard C libraries

```bash
# Compile OpenMP version
gcc -fopenmp -o spellchecker_omp spellchecker_omp.c common.c

# Compile Pthreads version
gcc -pthread -o spellchecker_pthread spellchecker_pthread.c common.c
```

### Usage

```bash
# Run OpenMP version
./spellchecker_omp

# Run Pthreads version
./spellchecker_pthread
```

### Configuration

You can modify the following parameters in `common.h`:

- `MAX_DICT_WORDS`: Maximum number of dictionary words
- `MAX_WORD_LENGTH`: Maximum length of a single word
- `MAX_INPUT_WORDS`: Maximum number of input words
- `MAX_MISSPELLED`: Maximum number of misspelled words
- `SUGGESTION_THRESHOLD`: Maximum edit distance for valid suggestions
- `MAX_PRINT_SUGGESTIONS`: Maximum suggestions to display per misspelled word
- `NUM_THREADS`: Default thread count for both implementations
