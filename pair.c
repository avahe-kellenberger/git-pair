#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#define NO_FORMAT "\033[0m"
#define BOLD "\033[1m"
#define GREEN "\033[38;5;10m"
#define RED "\033[38;5;203m"
#define YELLOW "\033[38;5;226m"

const char *authors_file_name = ".gitauthors";
const char *commit_template_path = ".git/commit-template";

const char *title =
"           _ _                 _\n"
"      __ _(_) |_   _ __   __ _(_)_ __\n"
"     / _` | | __| | '_ \\ / _` | | '__|\n"
"    | (_| | | |_  | |_) | (_| | | |\n"
"     \\__, |_|\\__| | .__/ \\__,_|_|_|\n"
"     |___/        |_|\n"
"   -------------------------------------\n\n"
;

int init(void);
int prompt_add_author(void);
void prompt(char *response, char *prompt, ...);
int add_author(void);
int select_authors(void);
int select_author_index(int author_count, char *prompt);
void display_available_authors(char **authors, int author_count);
void free_authors(char **authors, int author_count);
char **read_authors(int *length);

int set_author(char *name, char *email);
int set_author_name(char *name);
int set_author_email(char *email);
int set_co_author(char *name, char *email);
int set_commit_template(void);

void print_help(void);
void print_title(void);

int main(int argc, char *argv[]) {
    if (argc < 2) {
        return select_authors();
    } 

    const char *param = argv[1];
    if (strcmp("add", param) == 0) {
        int added = prompt_add_author();
        printf("%sAuthors added: %d%s\n", GREEN, added, NO_FORMAT);
    } else if (strcmp("init", param) == 0) {
        init();
    } else if (strcmp("help", param) == 0) {
        print_help();
    } else {
        printf("%sInvalid input ", RED);
        printf("- run the `help` command to see parameter options.%s\n", NO_FORMAT);
    }
    return 0;
}

int init(void) {
    print_title();
    if (prompt_add_author() > 0) {
        return select_authors();
    }
    return -1;
}

/**
 * Asks the user if they want to add an author, until they explicitly exit.
 * @return The number of authors added. 
 */
int prompt_add_author(void) {
    int count = 1;
    while (add_author() == 0) {
        printf("%s\nPress enter to add an author, or q to exit:%s ", RED, NO_FORMAT);
        // Exit if the user only enters q.
        if (getchar() == 'q') {
            break;
        }
        printf("\n");
        count++;
    }
    return count;
}

/**
 * Adds an author entry to the authors file.
 * @return 0 if an author was added successfully.
 */
int add_author(void) {
    // Prompt for author name.
    char author_name[BUFSIZ];
    prompt(author_name, "%sEnter author's full name:%s ", GREEN, NO_FORMAT);
    if (strlen(author_name) == 0) {
        printf("%sNo author added.%s\n", RED, NO_FORMAT);
        return -1;
    }

    // Prompt for author email.
    char author_email[BUFSIZ];
    prompt(author_email, "%sEnter author's email:%s ", GREEN, NO_FORMAT);
    if (strlen(author_email) == 0) {
        printf("%sNo author added.%s\n", RED, NO_FORMAT);
        return -1;
    }

    // Open authors file for appending.
    FILE *authors_file = fopen(authors_file_name, "a+");

    // Create an author entry.
    // Double BUFSIZ (two string inputs) + 3 format chars. 
    int max_size = BUFSIZ * 2 + 3;
    char entry[max_size];
    snprintf(entry, max_size, "%s:<%s>\n", author_name, author_email);

    // Write entry to the file.
    fputs(entry, authors_file);

    // Close file on exit.
    return fclose(authors_file);
}

void prompt(char *response, char *prompt, ...) {
    va_list format_args;
    va_start(format_args, prompt);
    vprintf(prompt, format_args);
    va_end(format_args);
    fgets(response, BUFSIZ, stdin);
    response[strcspn(response, "\n")] = '\0';
}

/**
 * Selects the author and (options) co-author of future commits.
 * @return 0 if an author was selected successfully.
 */
int select_authors(void) {
    int author_count = 0;
    char **authors = read_authors(&author_count);

    // Show available authors.
    display_available_authors(authors, author_count);

    // Set the author.
    int index = select_author_index(author_count, "\n%sSelect the author:%s ");
    if (index < -1 || index > author_count - 1) {
        printf("%sIndex out of bounds - exiting.%s\n", RED, NO_FORMAT);
        exit(1);
    }

    char *entry, *name, *email;
    if (index == -1) {
        set_author("", "");
        printf("%sRemoved author.%s\n", RED, NO_FORMAT);
    } else {
        entry = index == -1 ? "" : strdup(authors[index]);
        name = index == -1 ? "" : strsep(&entry, ":");
        email = entry;
        if (set_author(name, email) != 0) {
            return -1;
        }
        printf("%sSet git user and email as %s %s%s\n\n", GREEN, name, email, NO_FORMAT);
    }

    // Set the co-author.
    index = select_author_index(author_count, "%sSelect the co-author:%s ");
    if (index < -1 || index > author_count - 1) {
        printf("%sIndex out of bounds - exiting.%s\n", RED, NO_FORMAT);
        exit(1);
    }

    if (index == -1) {
        set_co_author("", "");
        printf("%sRemoved co-author.%s\n", RED, NO_FORMAT);
    } else {
        entry = strdup(authors[index]);
        name = strsep(&entry, ":");
        email = entry;
        if (set_co_author(name, email) != 0) {
            return -1;
        }

        if (set_commit_template() != 0) {
            return -1;
        }
        printf("%sSet co-author as: %s %s%s\n", GREEN, name, email, NO_FORMAT);
    }

    free_authors(authors, author_count);
    return 0;
}

/**
 * Prompts the user for an index to select,
 * which is associated with a git author.
 */
int select_author_index(int author_count, char *prompt) {
    int index, item_count;
    do {
        printf(prompt, YELLOW, NO_FORMAT);
        item_count = scanf("%d", &index);
        if (item_count == EOF) {
            exit(1);
        }
    } while (item_count == 0);
    return index - 1;
}

/**
 * Displays all authors in the authors file.
 */
void display_available_authors(char **authors, int author_count) {
    // Display authors on each line to select for author, then co-author.
    printf("\t%s[%d]%s: %s%s%s\n", GREEN, 0, NO_FORMAT, RED, "Remove current author from role", NO_FORMAT);
    for (int i = 0; i < author_count; i++) {
        printf("\t%s[%d]%s: %s\n", GREEN, i + 1, NO_FORMAT, authors[i]);
    }
}

/**
 * Frees the authors array.
 */
void free_authors(char **authors, int author_count) {
    for (int i = 0; i < author_count; i++) {
        free(authors[i]);
    }
    free(authors);
}

/**
 * @return All author entries in the authors file.
 */
char **read_authors(int *length) {
    FILE *authors_file = fopen(authors_file_name, "r");
    // Check if authors file exists.
    if (authors_file == NULL) {
        printf("%sFile %s not in directory.%s\n", RED, authors_file_name, NO_FORMAT);
        printf("Run with the init parameter to create the file and add code authors.\n");
        exit(1);
    }

    // Store all authors in array.
    char **authors = malloc(255 * sizeof(*authors));
    char buff[255];
    int i;
    for (i = 0; fgets(buff, 255, authors_file); i++) {
        buff[strcspn(buff, "\n")] = '\0';
        authors[i] = malloc(strlen(buff) + 1);
        strcpy(authors[i], buff);
    }
    *length = i;
    fclose(authors_file);
    return authors;
}

/**
 * Sets the author via git config.
 * @see set_author_name
 * @see set_author_email
 */
int set_author(char *name, char *email) {
    return set_author_name(name) && set_author_email(email);
}

int set_author_name(char *name) {
    char git_cmd[BUFSIZ];
    snprintf(git_cmd, BUFSIZ, "git config user.name \"%s\"", name);
    return system(git_cmd);
}

int set_author_email(char *email) {
    char git_cmd[BUFSIZ];
    snprintf(git_cmd, BUFSIZ, "git config user.email \"%s\"", email);
    return system(git_cmd);
}

int set_commit_template(void) {
    char git_cmd[BUFSIZ];
    snprintf(git_cmd, BUFSIZ, "git config commit.template \"%s\"", commit_template_path);
    return system(git_cmd);
}

int set_co_author(char *name, char *email) {
    FILE *template = fopen(commit_template_path, "w");
    if (strlen(name) > 0 || strlen(email) > 0) {
        char entry[BUFSIZ];
        snprintf(entry, BUFSIZ, "\n\nCo-authored-by: %s %s", name, email);
        fputs(entry, template);
    }
    return fclose(template);
}

void print_help(void) {
    printf("%s%sCommands:%s\n\n", RED, BOLD, NO_FORMAT);
    printf("   %s<no command>%s - Select an author and optional co-author which exists in %s\n", GREEN, NO_FORMAT, authors_file_name);
    printf("   %sinit%s         - Initiate the setup for git pair\n", GREEN, NO_FORMAT);
    printf("   %sadd%s          - Add an author to your %s file for selection\n", GREEN, NO_FORMAT, authors_file_name);
    printf("   %shelp%s         - Display this message\n", GREEN, NO_FORMAT);
    printf("\n");
}

void print_title(void) {
    printf("%s%s%s", YELLOW, title, NO_FORMAT);
}
