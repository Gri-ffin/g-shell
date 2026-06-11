#include "autocompletion.h"
#include <stdlib.h>
#include "builtins.h"
#include <string.h>
#include <readline/readline.h>
// ci fail without this
#include <stdint.h>
#include "../path.h"
#include "../utils.h"
#include <sys/stat.h>
#include <limits.h>

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
 * @brief little helper to help with binary search
 */
static int builtin_cmp(const void *a, const void *b) {
    return strcmp(*(const char **) a, *(const char **) b);
}

/**
 *
 * @param text the command provided by the user to match against
 * @param state 0 on the first call, different on subsequent calls
 * @return a dynamically allocated string match, or NULL if no more matches
 */
char *builtin_generator(const char *text, const int state) {
    static int list_index;
    static size_t len;
    static char **executables = NULL;
    static int executables_count = 0;
    static char **filenames = NULL;
    static int files_count = 0;
    static int exec_index = 0;
    static int file_index = 0;
    static bool initialized = false;

    // only initialize the array once
    if (!initialized) {
        executables = resolve_executables_in_path(&executables_count);
        initialized = true;
    }

    if (!state) {
        list_index = 0;
        len = strlen(text);
        exec_index = 0;
        file_index = 0;

        // clean up previous files, we changed states
        if (filenames != NULL) {
            for (int i = 0; i < files_count; i++)
                free(filenames[i]);
            free(filenames);
            filenames = NULL;
        }
        files_count = 0;

        const char *last_slash = strrchr(text, '/');
        char scan_dir[PATH_MAX];
        if (last_slash) {
            snprintf(scan_dir, sizeof(scan_dir), "%.*s", (int)(last_slash - text), text);
        } else {
            strncpy(scan_dir, ".", sizeof(scan_dir));
        }

        filenames = resolve_files_or_dirs_in_dir(&files_count, scan_dir);
    }

    if (executables == NULL) return NULL;

    // First: yield matching builtins
    char *name;
    while ((name = builtins[list_index])) {
        list_index++;
        if (strncmp(text, name, len) == 0) {
            rl_completion_append_character = ' ';
            return strdup(name); // readline will free this memory
        }
    }

    // Then: yield matching executables, but SKIP ones already covered by builtins
    // use binary search instead of linear scan
    while (exec_index < executables_count) {
        const char *executable = executables[exec_index++];
        if (strncmp(text, executable, len) != 0)
            continue;
        if (!bsearch(&executable, builtins, builtins_count, sizeof(char *), builtin_cmp)) {
            rl_completion_append_character = ' ';
            return strdup(executable);
        }
    }

    // get position of the last slash
    const char *last_slash = strrchr(text, '/');
    char expected_prefix[PATH_MAX];

    if (last_slash) {
        // If text is "src/pa", copy up to the slash -> "src/"
        snprintf(expected_prefix, sizeof(expected_prefix), "%.*s/", (int)(last_slash - text), text);
    } else {
        // if just "Mak", wel will look in "." and prepend "./"
        strcpy(expected_prefix, "./");
    }

    // The actual text we want to match against (e.g., "Mak" instead of "src/Mak")
    const char *base_text = last_slash ? last_slash + 1 : text;
    const size_t base_len = strlen(base_text);

    // Then yield matching file names
    while (file_index < files_count) {
        const char *full_file = filenames[file_index++];

        // Skip over the directory prefix that utils.c prepended
        const char *file_basename = full_file;
        // If full_file is "./Makefile" and expected_prefix is "./"
        if (strncmp(full_file, expected_prefix, strlen(expected_prefix)) == 0) {
            file_basename += strlen(expected_prefix); // file becomes just Makefile
        }

        // Compare just the base name
        if (strncmp(file_basename, base_text, base_len) == 0) {
            struct stat st;
            const int is_dir = (stat(full_file, &st) == 0 && S_ISDIR(st.st_mode));

            char *result;
            // Reconstruct the string to exactly match how the user typed the directory
            if (last_slash) {
                // If text starts at memory address 1000 ("src/pa")
                // last_slash is the address of the / character, which is at memory address 1003,
                // 1003 - 1000 = 3, plus 1 byte for "/" and the length of the filename
                // if dir we want to append "/" and "\0" otherwise just 1 byte for space.
                result = malloc((last_slash - text) + 1 + strlen(file_basename) + (is_dir ? 2 : 1));
                sprintf(result, "%.*s/%s%s", (int)(last_slash - text), text, file_basename, is_dir ? "/" : "");
            } else {
                result = malloc(strlen(file_basename) + (is_dir ? 2 : 1));
                sprintf(result, "%s%s", file_basename, is_dir ? "/" : "");
            }

            if (is_dir) {
                rl_completion_append_character = '\0';
            } else {
                rl_completion_append_character = ' ';
            }
            return result;
        }
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
            const uint32_t random_index = arc4random_uniform(num_insults);

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
