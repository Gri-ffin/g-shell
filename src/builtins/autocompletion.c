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

/**
 *
 * @param text the command provided by the user to match against
 * @param state 0 on the first call, different on subsequent calls
 * @return a dynamically allocated string match, or NULL if no more matches
 */
char *builtin_generator(const char *text, const int state) {
    static int list_index, len;
    static char **executables = NULL;
    static int count = 0;
    static int exec_index = 0;
    static bool initialized = false;

    // only initialize the array once
    if (!initialized) {
        executables = resolve_executables_in_path(&count);
        initialized = true;
    }

    if (!state) {
        list_index = 0;
        len = strlen(text);
        exec_index = 0;
    }

    if (executables == NULL) return NULL;

    // First: yield matching builtins
    char *name;
    // iterate through the builtins array
    while ((name = builtins[list_index])) {
        list_index++;
        if (strncmp(text, name, len) == 0) {
            return strdup(name); // readline will free this memory
        }
    }

    // Then: yield matching executables, but SKIP ones already covered by builtins
    while (exec_index < count) {
        const char *ext = executables[exec_index++];
        if (strncmp(ext, text, len) != 0)
            continue;

        // Check if this name is already in builtins
        bool is_builtin = false;
        for (int i = 0; builtins[i] != NULL; i++) {
            if (strcmp(ext, builtins[i]) == 0) {
                is_builtin = true;
                break;
            }
        }
        if (!is_builtin)
            return strdup(ext);
    }

    return NULL;
}

/**
 * @brief Handles tab-completion for shell commands.
 * Only triggers at the start of a line (start == 0) to match builtins.
 * If completion fails, it prints a random insult and restores the prompt.
 * * @param text  The string prefix being autocompleted.
 * @param start The buffer index where the word begins.
 * @param end   The buffer index where the word ends.
 * @return Array of string matches, or NULL to fall back to filename completion.
 */
char **shell_completion(const char *text, const int start, int end) {
    char **matches = NULL;

    // We only want to autocomplete builtins if the text is at the start of the line (start == 0)
    if (start == 0) {
        matches = rl_completion_matches(text, builtin_generator);
        if (matches == NULL) {
            const int num_insults = sizeof(insults) / sizeof(insults[0]);
            const int random_index = rand() % num_insults;

            // print the insult on a new line, then restore the prompt seamlessly
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
