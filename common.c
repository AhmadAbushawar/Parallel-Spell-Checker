#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* Global Variables */

/* Dictionary storage. */
char dictionary[MAX_DICT_WORDS][MAX_WORD_LENGTH];
int dictSize = 0;

/* Tokenized input words. */
char *inputWords[MAX_INPUT_WORDS];
int wordCount = 0;

/* Misspelled words list. */
char *misspelledWords[MAX_MISSPELLED];
int misspelledCount = 0;

/* Utility Functions */

/* Converts a string to lowercase. */
void toLowerCase(char *str)
{
    for (int i = 0; str[i]; i++)
        str[i] = tolower(str[i]);
}

/* Loads dictionary words from a file and converts each to lowercase. */
void loadDictionary(const char *filename)
{
    FILE *file = fopen(filename, "r");
    if (!file)
    {
        perror("Error opening dictionary file");
        exit(1);
    }

    char line[MAX_WORD_LENGTH];
    while (fgets(line, sizeof(line), file) && dictSize < MAX_DICT_WORDS)
    {
        line[strcspn(line, "\n")] = '\0'; // Strip newline
        toLowerCase(line);
        strcpy(dictionary[dictSize++], line);
    }

    fclose(file);
}

/* Comparator used by qsort() to sort dictionary words alphabetically. */
int cmpDictionary(const void *a, const void *b)
{
    const char *wordA = (const char *)a;
    const char *wordB = (const char *)b;
    return strcmp(wordA, wordB);
}

/* Loads and tokenizes input document into individual lowercase words. */
void loadAndTokenizeDocument(const char *filename)
{
    FILE *file = fopen(filename, "r");
    if (!file)
    {
        perror("Error opening input document");
        exit(1);
    }

    char line[1000];
    while (fgets(line, sizeof(line), file))
    {
        char *token = strtok(line, " ,.;:!?\"’\n"); // Include curly apostrophe
        while (token != NULL && wordCount < MAX_INPUT_WORDS)
        {
            toLowerCase(token);
            inputWords[wordCount++] = strdup(token);
            token = strtok(NULL, " ,.;:!?\"’\n");
        }
    }

    fclose(file);
}

/* Performs binary search to check whether a word exists in the dictionary. */
int isWordInDictionary(const char *word)
{
    int low = 0, high = dictSize - 1;
    while (low <= high)
    {
        int mid = (low + high) / 2;
        int cmp = strcmp(word, dictionary[mid]);
        if (cmp == 0)
            return 1;
        else if (cmp < 0)
            high = mid - 1;
        else
            low = mid + 1;
    }
    return 0;
}

/* Computes Levenshtein edit distance between two strings. */
int editDistance(const char *s1, const char *s2)
{
    int len1 = strlen(s1);
    int len2 = strlen(s2);

    int *prev = malloc((len2 + 1) * sizeof(int));
    int *curr = malloc((len2 + 1) * sizeof(int));
    if (!prev || !curr)
    {
        perror("Memory allocation failed in editDistance");
        exit(1);
    }

    for (int j = 0; j <= len2; j++)
        prev[j] = j;

    for (int i = 1; i <= len1; i++)
    {
        curr[0] = i;
        for (int j = 1; j <= len2; j++)
        {
            int cost = (s1[i - 1] == s2[j - 1]) ? 0 : 1;
            int deletion = prev[j] + 1;
            int insertion = curr[j - 1] + 1;
            int substitution = prev[j - 1] + cost;

            int min = deletion;
            if (insertion < min)
                min = insertion;
            if (substitution < min)
                min = substitution;

            curr[j] = min;
        }

        for (int j = 0; j <= len2; j++)
            prev[j] = curr[j];
    }

    int result = prev[len2];
    free(prev);
    free(curr);
    return result;
}

/* Comparator to sort suggestions: first by distance, then alphabetically. */
int cmpSuggestion(const void *a, const void *b)
{
    const struct Suggestion *sa = (const struct Suggestion *)a;
    const struct Suggestion *sb = (const struct Suggestion *)b;

    if (sa->distance != sb->distance)
        return sa->distance - sb->distance;

    return strcmp(sa->word, sb->word);
}
