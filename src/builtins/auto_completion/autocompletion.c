#include "autocompletion.h"
#include <stdlib.h>
#include "../builtins.h"
#include <string.h>
#include <readline/readline.h>
// ci fail without this
#include "../../path.h"
#include "../../utils.h"
#include <sys/stat.h>
#include <limits.h>
#include "../complete.h"

const char *insults[] = {
    "(⌐■_■) El Psy Kongroo... The Organization erased that command.",
    "(ಠ_ಠ) Get in the robot, because that command doesn't exist.",
    "( ‾ʖ̫‾) You thought it was a valid command, but it was me, DIO!",
    "(╬ Ò ‸ Ó) Omae wa mou shindeiru. (That command is already dead.)",
    "¯\\_(ツ)_/¯ Equivalent exchange failed. Command not found.",
    "(ノಠ益ಠ)ノ Its syntax error level is OVER 9000!",
    "Present day... Present time... Command not found in the Wired."
};

// Static declarations
static bool is_command_completion = true;
static int list_index;
static char **executables = NULL;
static DynamicArray *scripts_completions = NULL;
static int executables_count = 0;
static char **filenames = NULL;
static int files_count = 0;
static int exec_index = 0;
static int file_index = 0;
static bool initialized = false;

static DynamicArray *script_output = NULL;
static int script_output_index = 0;

/**
 * @brief little helper to help with binary search
 */
static int builtin_cmp(const void *a, const void *b) {
    return strcmp(*(const char **) a, *(const char **) b);
}

/**
 *
 * @param text the command name
 */
static void init_completion_scripts(const char *text) {
    list_index = 0;
    exec_index = 0;
    file_index = 0;
    script_output_index = 0;
    scripts_completions = get_completion_scripts();

    // clean up previous files
    if (filenames != NULL) {
        for (int i = 0; i < files_count; i++)
            free(filenames[i]);
        free(filenames);
        filenames = NULL;
    }
    files_count = 0;

    // clean up previous script output
    if (script_output != NULL) {
        array_free(script_output);
        script_output = NULL;
    }

    if (is_command_completion) {
        // Context: Completing a command
        const char *last_slash = strrchr(text, '/');
        char scan_dir[PATH_MAX];
        if (last_slash) {
            snprintf(scan_dir, sizeof(scan_dir), "%.*s", (int)(last_slash - text), text);
        } else {
            strncpy(scan_dir, ".", sizeof(scan_dir));
        }
        filenames = resolve_files_or_dirs_in_dir(&files_count, scan_dir);
    } else {
        // Context: Completing an argument -> check for scripts
        const char *space = strchr(rl_line_buffer, ' ');
        if (space && scripts_completions && scripts_completions->count > 0) {
            const int cmd_len = (int) (space - rl_line_buffer);
            CompleteCommand **cmds = (CompleteCommand **) scripts_completions->items;

            for (int i = 0; i < scripts_completions->count; i++) {
                if (strncmp(rl_line_buffer, cmds[i]->program, cmd_len) == 0
                    && cmds[i]->program[cmd_len] == '\0') {
                    char invocation[PATH_MAX + 512];
                    snprintf(invocation, sizeof(invocation), "%s \"%s\" \"%s\"",
                             cmds[i]->path, text, rl_line_buffer);

                    FILE *fp = popen(invocation, "r");
                    if (!fp) {
                        perror(invocation);
                        return;
                    }

                    script_output = create_dynamic_array();
                    if (!script_output) {
                        pclose(fp);
                        break;
                    }

                    char line[MAX_INPUT];
                    while (fgets(line, sizeof(line), fp)) {
                        line[strcspn(line, "\n")] = '\0';
                        if (!*line) continue;
                        array_push(strdup(line), script_output);
                    }
                    pclose(fp);
                    break;
                }
            }
        }
    }
}

/**
 *
 * @param text the command name
 * @param len length for the command name
 * @return the completion argument
 */
static char *generate_command_completion(const char *text, const size_t len) {
    // First: yield matching builtins
    char *name;
    while ((name = builtins[list_index])) {
        list_index++;
        if (strncmp(text, name, len) == 0) {
            rl_completion_append_character = ' ';
            return strdup(name);
        }
    }

    // Then: yield matching executables
    if (executables != NULL) {
        while (exec_index < executables_count) {
            const char *executable = executables[exec_index++];
            if (strncmp(text, executable, len) != 0)
                continue;
            if (!bsearch(&executable, builtins, builtins_count, sizeof(char *), builtin_cmp)) {
                rl_completion_append_character = ' ';
                return strdup(executable);
            }
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

            rl_completion_append_character = is_dir ? '\0' : ' ';
            return result;
        }
    }
    return NULL;
}

/**
 *
 * @param text the command name
 * @param len length fo the command name
 * @return the argument to complete
 */
static char *generate_argument_completion(const char *text, const size_t len) {
    // Context: Argument yield completions from script output ONLY
    if (script_output != NULL) {
        while (script_output_index < script_output->count) {
            const char *candidate = (char *) script_output->items[script_output_index++];
            if (strncmp(text, candidate, len) == 0) {
                rl_completion_append_character = ' ';
                return strdup(candidate);
            }
        }
    }

    return NULL;
}

/**
 *
 * @param text the command provided by user to match against
 * @param state 0 on the first call, different on subsequent calls
 * @return a dynamically allocated string match, or NULL if no more matches
 */
char *builtin_generator(const char *text, const int state) {
    if (!initialized) {
        executables = resolve_executables_in_path(&executables_count);
        initialized = true;
    }

    if (!state) {
        init_completion_scripts(text);
    }

    // YIELD LOGIC
    if (is_command_completion) {
        return generate_command_completion(text, strlen(text));
    }
    return generate_argument_completion(text, strlen(text));
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

    // Track whether we are completing the first word or an argument
    is_command_completion = (start == 0);

    // always invoke our custom generator
    matches = rl_completion_matches(text, builtin_generator);

    // Only print insults if a root command completion fails
    if (matches == NULL && start == 0) {
        const int num_insults = sizeof(insults) / sizeof(insults[0]);
        const uint32_t random_index = arc4random_uniform(num_insults);

        // print the insult on a new line, then restore the prompt seamlessly
        printf("\n%s\n", insults[random_index]);
        rl_on_new_line();
        rl_redisplay();

        rl_attempted_completion_over = 1;
    }

    // Note: If matches is NULL, Readline will automatically fall back
    // to its default behavior (standard filename completion).
    return matches;
}
