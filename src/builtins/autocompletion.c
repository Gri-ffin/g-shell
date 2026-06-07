#include "autocompletion.h"
#include "builtins.h"
#include <string.h>
#include <readline/readline.h>

// Readline calls this repeatedly. It must return a malloc'd string for each match,
// and NULL when there are no more matches left.
char *builtin_generator(const char *text, const int state) {
    static int list_index, len;
    char *name;

    // 'state' is 0 on the first call for this completion attempt.
    // we initialize our counters here.
    if (!state) {
        list_index = 0;
        len = strlen(text);
    }

    // iterate through the builtins array
    while ((name = builtins[list_index])) {
        list_index++;

        // check if the builtin starts with user input
        if (strncmp(text, name, len) == 0) {
            return strdup(name); // readline will free this memory
        }
    }

    // hit end of list
    return NULL;
}

// The Attempted Completion Function
// This intercepts the default filename completion.
char **shell_completion(const char *text, const int start, int end) {
    char **matches = NULL;

    // We only want to autocomplete builtins if the text is at the start of the line (start == 0)
    if (start == 0) {
        matches = rl_completion_matches(text, builtin_generator);
    }

    // Note: If matches is NULL, Readline will automatically fall back
    // to its default behavior (standard filename completion).
    return matches;
}
