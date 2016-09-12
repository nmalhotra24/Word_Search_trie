#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define ALPHABET_SIZE (26)

// Converts key current character into index
// use only 'A' through 'Z' and upper case
#define CHAR_TO_INDEX(c) ((int)c - (int)'A')

#define WORD_SIZE 1024

/*
 * Trie node.
 */
typedef struct trie_node
{
    struct trie_node *next[ALPHABET_SIZE];

    // is_end is true if the node represents end of a word
    bool is_end;
} trie_node;

/*
 *  Datastructure to store Honeycomb.
 */
typedef struct honeycomb {
    int number_columns;
    char **columns;
} honeycomb;

/*
 * Datastructure to store words found in honeycomb.
 */
typedef struct word_store {
    int size;
    char **words;
} word_store;

/*
 * get_trienode
 *
 * Returns a new trie node and initialize all next pointers to NULL
 */
trie_node *
get_trienode(void)
{
    trie_node *node = NULL;

    node = (trie_node *)malloc(sizeof(trie_node));
    if (node == NULL) {
        printf("Error: Failed to allocate memory for Trie Node.\n");
        exit(1);
    }

    node->is_end = false;

    int i;
    for (i = 0; i < ALPHABET_SIZE; i++) {
       node->next[i] = NULL;
    }

    return node;
}

/*
 * insert_trie
 *
 * Insert a string(key) if not present, into the trie.
 * If the key is prefix of trie node, just marks leaf node.
 */
void
insert_trie(trie_node *root, const char *key)
{
    int level;
    int length = strlen(key);
    int index;

    trie_node *parent = root;

    for (level = 0; level < length; level++) {
        index = CHAR_TO_INDEX(key[level]);
        if (parent->next[index] == NULL) {
            parent->next[index] = get_trienode();
        }

        parent = parent->next[index];
    }

    /* Mark the last node as leaf */
    parent->is_end = true;
}

/*
 * delete_trie
 *
 * Free the entire trie.
 */
void
delete_trie(trie_node *root)
{
    if (root == NULL) return;

    int i;
    for (i = 0; i < ALPHABET_SIZE; i++) {
        delete_trie(root->next[i]);
    }

    free(root);
}

/*
 * fill_trie
 *
 * Read all words from dictionary file line by line and
 * add the words to trie.
 */
void
fill_trie(trie_node *root, FILE *fp)
{
    char word[WORD_SIZE];

    while (fgets(word, sizeof(word), fp) != NULL) {
        /* fgets might add a newline at the end of the string read. */
        word[strlen(word) - 1] = '\0';
        insert_trie(root, word);
    }
}

/*
 * hcomb_store
 *
 * Helper function to help store the characters in
 * the honeycomb array columns.
 */
void
hcomb_store(honeycomb *hc, char **layers, int num_layers, bool right)
{
    int i, j;

    for (i = num_layers - 1; i >= 0; i--) {
        int column_len = 2 * num_layers - i;
        char *column = (char *) malloc(column_len + 1); // + 1 for '\0'
        column[column_len] = '\0';
        /* copy contiguous segment of layer i in ith column from center */
        strncpy(column + num_layers - i - 1, layers[i] + i, i + 2);

        /* add characters to column that are part of adjacent layers */
        for (j = num_layers - 1; j > i; j--) {
            /* character on lower end of ith column from center */
            strncpy(column + num_layers - j - 1, layers[j] + i, 1);
            /* character on upper end of ith column from center */
            strncpy(column + num_layers - i + j, layers[j] - i + 3 * j + 1, 1);
        }

        /* 'half' multiplier places column in the correct half of
            hc's internal columns array */
        if (right) {
            hc->columns[num_layers + i + 1] = column;
        } else {
            hc->columns[num_layers - i - 1] = column;
        }
    }
}

/*
 * fill_honeycomb
 *
 * Fill the honeycomb to store word search characters.
 */
void
fill_honeycomb(honeycomb *hc, FILE *fp, int layers)
{
    /* Allocate space for '\0' */
    char *center = (char *) malloc(2 * layers);
    fscanf(fp, " %c", center + layers - 1);
    center[2 * layers - 1] = '\0';

    if (layers > 1) {
        char *rightlayers[layers - 1];
        char *leftlayers[layers - 1];

        int i, j, k;
        for (i = 1; i < layers; i++) {
            int halflayerlen = 2 + (i - 1) * 3;
            char *right = (char *) malloc(halflayerlen + 1);
            char *left = (char *) malloc(halflayerlen + 1);

            /* ead first char in layer string to upper center column */
            fscanf(fp, " %c", center + layers - 1 + i);

            /* read right side of layer and add to right layer
               string in REVERSE order */
            for (j = halflayerlen - 1; j >= 0; j--) {
                fscanf(fp, " %c", &(right[j]));
            }

            right[halflayerlen] = '\0';
            rightlayers[i - 1] = right;
            fscanf(fp, " %c", center + layers - 1 - i);

            /* read left side of layer and add to left layer
               string in SAME order */
            for (k = 0; k < halflayerlen; k++) {
                fscanf(fp, " %c", left + k);
            }

            left[halflayerlen] = '\0';
            leftlayers[i - 1] = left;
        }

        /* pass layers - 1 because center layer is not stored;
           only layers 1 through layers - 1 */
        hcomb_store(hc, leftlayers, layers - 1, false);
        hcomb_store(hc, rightlayers, layers - 1, true);

        for (i = 0; i < layers - 1; i++) {
            free(rightlayers[i]);
            free(leftlayers[i]);
        }
    }

    hc->columns[layers - 1] = center;
}

/*
 * create_honeycomb
 *
 * Create the Honeycomb datastructure to store word search characters.
 * Allocate space for arrays representing honeycomb's columns.
 */
honeycomb *
create_honeycomb(int layers)
{
    honeycomb *hc = (honeycomb *) malloc(sizeof(honeycomb));
    if (hc == NULL) {
        printf("Error: Failed to allocate memory for Honeycomb.\n");
        exit(1);
    }

    hc->number_columns = 2 * layers - 1;
    hc->columns = (char **) calloc(hc->number_columns, sizeof(char *));
    if (hc->columns == NULL) {
        printf("Error: Failed to allocate memory for Honeycomb.\n");
        free(hc);
        exit(1);
    }

    return hc;
}

/*
 * delete_honeycomb
 *
 * Delete the Honeycomb.
 */
void
delete_honeycomb(honeycomb *hc)
{
    int i;
    for (i = 0; i < hc->number_columns; i++) {
        free(hc->columns[i]);
    }

    free(hc->columns);
    free(hc);
}

/*
 * create_store
 *
 * Create the Word Store.
 *
 * This does not allocates any actual momory for
 * word storage. Actual memory will be allocated
 * when words needs to be added to the word store.
 */
word_store *
create_store()
{
    word_store* store = (word_store *) malloc(sizeof(word_store));
    if (store == NULL) {
        printf("Error: Failed to allocate memory for Word Store.\n");
        exit(1);
    }

    store->size = 0;
    store->words = NULL;

    return store;
}

/*
 * delete_store
 *
 * Delete the Word store.
 */
void
delete_store(word_store *store)
{
     int i;
    for (i = 0; i < store->size; i++) {
        free(store->words[i]);
    }

    free(store->words);
    free(store);
}

/*
 * find_words_trie
 *
 * Helper function to recursively find words with a prefix
 * in the trie.
 */
void
find_words_trie(honeycomb *hc, trie_node *node, word_store* store,
                char *word, int column, int label)
{
    /* Basic sanity */
    if ((column < 0 || label < 0 || column >= hc->number_columns ||
        label >= strlen(hc->columns[column]) || hc->columns[column][label] == '-')
        && strlen(word) != 0) {
        return;
    } else {
        /* Add the current character to the end of the prefix */
        strncat(word, hc->columns[column] + label, 1);
        int index = CHAR_TO_INDEX(hc->columns[column][label]);
        trie_node* next_node = node->next[index];
        if (next_node) {
            if (next_node->is_end) {
                store->words = realloc(store->words,
                                       ++(store->size) * sizeof(char *));
                store->words[store->size-1] = strdup(word);
                /* Remove the end marker for this key to avoid
                   duplicate detection */
                next_node->is_end = false;
            }

            /* avoid revisting */
            char save = hc->columns[column][label];
            hc->columns[column][label] = '-';
            int i, j;
            for (i = -1; i <= 1; i++) {
                for (j = -1; j <= 1; j++) {
                    if (!(column + i == column && label + j == label)) {
                        find_words_trie(hc, next_node, store, word,
                                        column + i, label + j);
                    }
                }
            }
            hc->columns[column][label] = save;
        }

        word[strlen(word) - 1] = '\0';
    }
}

/*
 * find_words
 *
 * Traverse honeycomb character by character and find
 * words starting with the character in the trie.
 * Incase a match is found in the trie, find all words
 * in the trie matching prefix with adjoining characters of
 * the original character in the honeycomb. */
void
find_words(honeycomb *hc, trie_node *root, word_store* store)
{
    char *word = (char *) malloc(WORD_SIZE);

    int i, j;
    for (i = 0; i < hc->number_columns; i++) {
        for (j = 0; j < strlen(hc->columns[i]); j++) {
            word[0] = '\0';
            find_words_trie(hc, root, store, word, i, j);
        }
    }

    free(word);
}

/*
 * comparator
 *
 * Compares two words for sort order when sorting found words.
 */
int
comparator(const void *word1, const void *word2)
{
    return strcmp(*(char **) word1, *(char **) word2);
}

int
main(int argc, char *argv[])
{
    if (argc != 3) {
        printf("Error: Insufficient arguments.\nNeed two files"
               " (honeycomb.txt and dictionary.txt) as input.\n");
        exit(1);
    }

    FILE *honeycomb_fp = fopen(argv[1], "r");
    if (honeycomb_fp == NULL) {
        printf("Error: honeycomb.txt file missing.\n");
        exit(1);
    }

    FILE *dictionary_fp = fopen(argv[2], "r");
    if (dictionary_fp == NULL) {
        printf("Error: dictionary.txt file missing.\n");
        exit(1);
    }

    /* Create a honeycomb from letters in the file. */
    int layers;
    fscanf(honeycomb_fp, "%d", &layers);
    honeycomb *hc = create_honeycomb(layers);
    fill_honeycomb(hc, honeycomb_fp, layers);
    fclose(honeycomb_fp);

    /* Create a Trie for all the words in the dictionary. */
    trie_node *root = get_trienode();
    fill_trie(root, dictionary_fp);
    fclose(dictionary_fp);

    /* Create a Word Store to store all the words found. */
    word_store* store = create_store();
    find_words(hc, root, store);

    if (store->size == 0) {
        printf("No words found.\n");
    } else {
        qsort(store->words, store->size, sizeof(char *), comparator);

        int i;
        printf("%s\n", store->words[0]);
        for (i = 1; i < store->size; i++) {
            /* avoid duplicates */
            if (strcmp(store->words[i], store->words[i-1]) != 0) {
                printf("%s\n", store->words[i]);
            }
        }
    }

    /* Free the honeycomb, trie and word store after done */
    delete_honeycomb(hc);
    delete_trie(root);
    delete_store(store);

    return 0;
}
