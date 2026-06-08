#include "autocompletion.h"
#include <stdlib.h>
#include "builtins.h"
#include <string.h>
#include <readline/readline.h>
#include "../path.h"

const char *insults[] = {
    "(⌐■_■) El Psy Kongroo... The Organization erased that command.",
    "(ಠ_ಠ) Get in the robot, because that command doesn't exist.",
    "( ‾ʖ̫‾) You thought it was a valid command, but it was me, DIO!",
    "(╬ Ò ‸ Ó) Omae wa mou shindeiru. (That command is already dead.)",
    "¯\\_(ツ)_/¯ Equivalent exchange failed. Command not found.",
    "(ノಠ益ಠ)ノ Its syntax error level is OVER 9000!",
    "Present day... Present time... Command not found in the Wired."
};

// Readline calls this repeatedly. It must return a malloc'd string for each match,
// and NULL when there are no more matches left.
char *builtin_generator(const char *text, const int state) {
    static int list_index, len;
    static char **executables = NULL;
    static int count = 0;
    static int exec_index = 0;
    char *name;
    static bool initialized = false;

    if (!initialized) {
        executables = resolve_executables_in_path(&count);
        initialized = true;
    }

    // 'state' is 0 on the first call for this completion attempt.
    // we initialize our counters here.
    if (!state) {
        list_index = 0;
        len = strlen(text);
        exec_index = 0;
    }

    if (executables == NULL) return NULL;

    // iterate through the builtins array
    while ((name = builtins[list_index])) {
        list_index++;

        // check if the builtin starts with user input
        if (strncmp(text, name, len) == 0) {
            return strdup(name); // readline will free this memory
        }
    }

    while (exec_index < count) {
        const char *external_program = executables[exec_index++];
        if (strncmp(external_program, text, len) == 0) {
            return strdup(external_program);
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
        if (matches == NULL) {
            const int num_insults = sizeof(insults) / sizeof(insults[0]);
            const int random_index = rand() % num_insults;

            // print the insult on a new line, then restore your prompt seamlessly
            printf("\n%s\n", insults[random_index]);
            rl_on_new_line();
            rl_redisplay();

            rl_attempted_completion_over = 1;
        }
    }

    // Note: If matches is NULL, Readline will automatically fall back
    // to its default behavior (standard filename completion).
    return matches;
}
