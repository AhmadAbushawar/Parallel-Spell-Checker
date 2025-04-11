#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <omp.h>
#include "common.h"

void spellCheck(void);
void suggestCorrections(const char *word);

int main()
{
    omp_set_num_threads(NUM_THREADS);

    /* Load the dictionary,
       then sort the dictionary for binary search lookups. */
    loadDictionary("words.txt");
    qsort(dictionary, dictSize, sizeof(dictionary[0]), cmpDictionary);
    printf("Loaded %d words from the dictionary.\n", dictSize);

    /* Load and tokenize the input document. */
    loadAndTokenizeDocument("input.txt");
    printf("Loaded %d words from the input document.\n", wordCount);

    /* Perform parallel spell checking on the tokenized input words. */
    spellCheck();

    /* Display results. */
    printf("\n=========================================\n");
    printf("         SPELL CHECKER RESULTS (OpenMP)\n");
    printf("=========================================\n");
    printf("Dictionary Words Loaded       : %d\n", dictSize);
    printf("Input Document Words Loaded   : %d\n", wordCount);
    printf("Total Misspelled Words        : %d\n", misspelledCount);
    printf("=========================================\n\n");

    for (int i = 0; i < misspelledCount; i++)
    {
        printf("Misspelled Word         : %s\n", misspelledWords[i]);
        printf("Correction Suggestions  : ");
        suggestCorrections(misspelledWords[i]);
        printf("\n\n");
    }

    for (int i = 0; i < wordCount; i++)
        free(inputWords[i]);

    return 0;
}

/*
   Iterates over all input words in parallel.
   For each word that is not found in the dictionary,
   it adds the word to the shared misspelledWords array.
*/
void spellCheck(void)
{
    #pragma omp parallel for
    for (int i = 0; i < wordCount; i++)
    {
        if (!isWordInDictionary(inputWords[i]))
        {
            #pragma omp critical
            {
                if (misspelledCount < MAX_MISSPELLED)
                    misspelledWords[misspelledCount++] = inputWords[i];
            }
        }
    }
}

/*
   For a given misspelled word, this function scans the dictionary
   in parallel (using length filtering and edit distance checks) to
   collect candidate suggestions. The candidates are then sorted,
   and only the top suggestions are printed.
*/
void suggestCorrections(const char *word)
{
    int wordLen = strlen(word);

    /* Allocate an array to hold candidate suggestions.
       Worst case, every dictionary word qualifies. */
    struct Suggestion *candidates = malloc(dictSize * sizeof(struct Suggestion));
    if (!candidates)
    {
        perror("Memory allocation failed for suggestions");
        exit(1);
    }
    int candidateCount = 0;

/* Parallel loop to scan dictionary words. */
    #pragma omp parallel for
    for (int i = 0; i < dictSize; i++)
    {
        int dictWordLen = strlen(dictionary[i]);
        /* Apply length filtering to reduce expensive edit distance calls. */
        if (abs(dictWordLen - wordLen) > SUGGESTION_THRESHOLD)
            continue;
        int dist = editDistance(word, dictionary[i]);
        if (dist <= SUGGESTION_THRESHOLD)
        {
            int index;
            #pragma omp critical
            {
                index = candidateCount++;
                candidates[index].distance = dist;
                strcpy(candidates[index].word, dictionary[i]);
            }
        }
    }

    if (candidateCount == 0)
    {
        printf("No suggestions found.");
        free(candidates);
        return;
    }

    /* Sort the candidate suggestions by distance, then lexicographically. */
    qsort(candidates, candidateCount, sizeof(struct Suggestion), cmpSuggestion);

    /* Print only the top MAX_PRINT_SUGGESTIONS suggestions. */
    int toPrint = candidateCount < MAX_PRINT_SUGGESTIONS ? candidateCount : MAX_PRINT_SUGGESTIONS;
    for (int i = 0; i < toPrint; i++)
    {
        printf("%s", candidates[i].word);
        if (i < toPrint - 1)
            printf(", ");
    }
    free(candidates);
}
