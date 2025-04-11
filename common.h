#ifndef COMMON_H
#define COMMON_H

/* Global Constants */
#define MAX_DICT_WORDS 466600   // Maximum number of dictionary words.
#define MAX_WORD_LENGTH 50      // Maximum length of a single word.
#define MAX_INPUT_WORDS 50000   // Maximum number of input words.
#define MAX_MISSPELLED 5000     // Maximum number of misspelled words.
#define SUGGESTION_THRESHOLD 2  // Maximum edit distance for valid suggestions.
#define MAX_PRINT_SUGGESTIONS 5 // Max suggestions to display per misspelled word.
#define NUM_THREADS 8           // Default thread count for both OpenMP and pthreads versions.

/* Global Arrays */

/* Dictionary storage. */
extern char dictionary[MAX_DICT_WORDS][MAX_WORD_LENGTH];
extern int dictSize;

/* Tokenized input words. */
extern char *inputWords[MAX_INPUT_WORDS];
extern int wordCount;

/* List of misspelled words. */
extern char *misspelledWords[MAX_MISSPELLED];
extern int misspelledCount;

/* Data Structures */

/* Candidate suggestion with edit distance. */
struct Suggestion
{
    char word[MAX_WORD_LENGTH]; // Suggested dictionary word.
    int distance;               // Edit distance to the misspelled word.
};

/* Function Prototypes */

/* Converts a string to lowercase. */
void toLowerCase(char *str);

/* Loads dictionary words from a file (one word per line). */
void loadDictionary(const char *filename);

/* Comparator for sorting dictionary words alphabetically. */
int cmpDictionary(const void *a, const void *b);

/* Tokenizes input document into lowercase words. */
void loadAndTokenizeDocument(const char *filename);

/* Checks if a word exists in the dictionary using binary search. */
int isWordInDictionary(const char *word);

/* Computes the Levenshtein edit distance between two strings. */
int editDistance(const char *s1, const char *s2);

/* Comparator for sorting suggestions by distance and alphabetically. */
int cmpSuggestion(const void *a, const void *b);

#endif // COMMON_H
