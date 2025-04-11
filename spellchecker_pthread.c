#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "common.h"

/* Thread data structure for spell checking. */
typedef struct
{
    int start; // Starting index into inputWords array.
    int end;   // Ending index (inclusive).
} SpellThreadData;

/* Thread data structure for suggestion generation. */
typedef struct
{
    int start;                     // Starting index in dictionary for this thread.
    int end;                       // Ending index (inclusive).
    const char *word;              // The misspelled word (already in lowercase).
    int wordLen;                   // Precomputed length of the misspelled word.
    struct Suggestion *candidates; // Local candidate array.
    int candidateCount;            // Number of candidates found by this thread.
} SuggestThreadData;

/* Mutex to protect the shared misspelledWords array. */
pthread_mutex_t misspelledMutex;

void *spellCheckThread(void *arg);
void spellCheck(void);
void *suggestionThread(void *arg);
void suggestCorrections(const char *word);

int main(void)
{
    pthread_t threads[NUM_THREADS];
    SpellThreadData spellThreadData[NUM_THREADS];

    if (pthread_mutex_init(&misspelledMutex, NULL) != 0)
    {
        perror("Failed to initialize mutex");
        exit(1);
    }

    /* Load the dictionary and tokenize the input document. */
    loadDictionary("words.txt");
    qsort(dictionary, dictSize, sizeof(dictionary[0]), cmpDictionary);
    printf("Loaded %d words from the dictionary.\n", dictSize);

    loadAndTokenizeDocument("input.txt");
    printf("Loaded %d words from the input document.\n", wordCount);

    /* Partition the work among spell-checking threads. */
    int chunk = wordCount / NUM_THREADS;
    int remainder = wordCount % NUM_THREADS;
    int start = 0;
    for (int i = 0; i < NUM_THREADS; i++)
    {
        spellThreadData[i].start = start;
        spellThreadData[i].end = start + chunk - 1;
        if (remainder > 0)
        {
            spellThreadData[i].end++;
            remainder--;
        }
        start = spellThreadData[i].end + 1;
        if (pthread_create(&threads[i], NULL, spellCheckThread, (void *)&spellThreadData[i]))
        {
            perror("Error creating spell-check thread");
            exit(1);
        }
    }
    for (int i = 0; i < NUM_THREADS; i++)
    {
        if (pthread_join(threads[i], NULL))
        {
            perror("Error joining spell-check thread");
            exit(1);
        }
    }

    /* Display results. */
    printf("\n=========================================\n");
    printf("         SPELL CHECKER RESULTS (Pthreads)\n");
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
    pthread_mutex_destroy(&misspelledMutex);

    return 0;
}

/* Spell check thread: processes a portion of inputWords to identify misspelled words. */
void *spellCheckThread(void *arg)
{
    SpellThreadData *data = (SpellThreadData *)arg;
    for (int i = data->start; i <= data->end; i++)
    {
        if (!isWordInDictionary(inputWords[i]))
        {
            pthread_mutex_lock(&misspelledMutex);
            if (misspelledCount < MAX_MISSPELLED)
                misspelledWords[misspelledCount++] = inputWords[i];
            pthread_mutex_unlock(&misspelledMutex);
        }
    }
    pthread_exit(NULL);
    return NULL;
}

/* Suggestion thread: scans its assigned portion of the dictionary
   to compute candidate suggestions for a misspelled word. */
void *suggestionThread(void *arg)
{
    SuggestThreadData *data = (SuggestThreadData *)arg;
    int range = data->end - data->start + 1;
    data->candidates = malloc(range * sizeof(struct Suggestion));
    if (!data->candidates)
    {
        perror("Memory allocation failed in suggestionThread");
        pthread_exit(NULL);
    }
    data->candidateCount = 0;
    for (int i = data->start; i <= data->end; i++)
    {
        int dictWordLen = strlen(dictionary[i]);
        if (abs(dictWordLen - data->wordLen) > SUGGESTION_THRESHOLD)
            continue;
        int dist = editDistance(data->word, dictionary[i]);
        if (dist <= SUGGESTION_THRESHOLD)
        {
            strcpy(data->candidates[data->candidateCount].word, dictionary[i]);
            data->candidates[data->candidateCount].distance = dist;
            data->candidateCount++;
        }
    }
    pthread_exit(NULL);
    return NULL;
}

/* Divides the dictionary among NUM_THREADS suggestion threads,
   collects candidate suggestions from each thread, merges, sorts, and prints
   the top MAX_PRINT_SUGGESTIONS suggestions. */
void suggestCorrections(const char *word)
{
    int wordLen = strlen(word);
    int totalCandidates = 0;
    pthread_t threads[NUM_THREADS];
    SuggestThreadData threadData[NUM_THREADS];

    /* Partition the dictionary indices among suggestion threads. */
    int chunk = dictSize / NUM_THREADS;
    int remainder = dictSize % NUM_THREADS;
    int startIndex = 0;
    for (int i = 0; i < NUM_THREADS; i++)
    {
        threadData[i].start = startIndex;
        threadData[i].end = startIndex + chunk - 1;
        if (remainder > 0)
        {
            threadData[i].end++;
            remainder--;
        }
        startIndex = threadData[i].end + 1;
        threadData[i].word = word;
        threadData[i].wordLen = wordLen;
        threadData[i].candidateCount = 0;
        threadData[i].candidates = NULL;
        if (pthread_create(&threads[i], NULL, suggestionThread, &threadData[i]))
        {
            perror("Error creating suggestion thread");
            exit(1);
        }
    }

    /* Wait for suggestion threads to finish and sum candidate counts. */
    for (int i = 0; i < NUM_THREADS; i++)
    {
        pthread_join(threads[i], NULL);
        totalCandidates += threadData[i].candidateCount;
    }

    if (totalCandidates == 0)
    {
        printf("   No suggestions found.");
        return;
    }

    /* Allocate merged array for all candidate suggestions. */
    struct Suggestion *merged = malloc(totalCandidates * sizeof(struct Suggestion));
    if (!merged)
    {
        perror("Memory allocation failed for merged suggestions");
        exit(1);
    }
    int pos = 0;
    for (int i = 0; i < NUM_THREADS; i++)
    {
        for (int j = 0; j < threadData[i].candidateCount; j++)
        {
            merged[pos++] = threadData[i].candidates[j];
        }
        free(threadData[i].candidates);
    }

    /* Sort merged suggestions (first by distance, then lexicographically). */
    qsort(merged, totalCandidates, sizeof(struct Suggestion), cmpSuggestion);

    /* Print only the top MAX_PRINT_SUGGESTIONS suggestions. */
    int toPrint = totalCandidates < MAX_PRINT_SUGGESTIONS ? totalCandidates : MAX_PRINT_SUGGESTIONS;
    for (int i = 0; i < toPrint; i++)
    {
        printf("%s", merged[i].word);
        if (i < toPrint - 1)
            printf(", ");
    }
    free(merged);
}
