#include <ncurses.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>

#define MAX_FILES 1024
#define MAX_PATH  4096

#define KEY_ENTER '\n'

// Global state
char current_path[MAX_PATH];
char *files[MAX_FILES];
int file_count = 0;
int selected = 0;
FILE *debug_log;

void init_debug() {
    debug_log = fopen("debug.log", "w");
    if (!debug_log) {
        // Print to stderr before ncurses or use printw after initscr
        fprintf(stderr, "Failed to open debug.log\n");
    } else {
        fprintf(debug_log, "Debug log initialized\n");
        fflush(debug_log); // Ensure initial message is written
    }
}

void deinit_debug() {
    if (debug_log) {
        fprintf(debug_log, "Debug log closing\n");
        fflush(debug_log); // Flush before closing
        fclose(debug_log);
        debug_log = NULL; // Prevent reuse
    }
}

void debug(const char *fmt, ...) {
    if (!debug_log) {
        // Fallback to stderr (outside ncurses or use printw)
        va_list args;
        va_start(args, fmt);
        vfprintf(stderr, fmt, args);
        fprintf(stderr, "\n");
        va_end(args);
        return;
    }
    va_list args;
    va_start(args, fmt);
    vfprintf(debug_log, fmt, args);
    fprintf(debug_log, "\n");
    fflush(debug_log); // Ensure immediate write
    va_end(args);
}

void get_file_info(const char *path, char *buf, size_t buflen) {
    struct stat st;
    int err = stat(path, &st);
    int index = 0;
    if (err != 0) {
        snprintf(buf, buflen, "Error retrieving info");
        return;
    }

    index = index + snprintf(buf + index, buflen, "Size: %lld bytes\n", (long long)st.st_size);
    index = index + snprintf(buf + index, buflen, "Permissions: %o\n", st.st_mode & 0777);
    index = index + snprintf(buf + index, buflen, "Last modified: %s", ctime(&st.st_mtime));
    index = index + snprintf(buf + index, buflen, "Owner UID: %d\n", st.st_uid);
    index = index + snprintf(buf + index, buflen, "Owner GID: %d\n", st.st_gid);
    index = index + snprintf(buf + index, buflen, "Is Directory: %s\n", S_ISDIR(st.st_mode) ? "Yes" : "No");
}

void scan_directory(const char *path) {
    DIR *dir;
    struct dirent *entry;

    // cleanup old list
    for (int i = 0; i < file_count; i++) {
        free(files[i]);
    }
    file_count = 0;

    dir = opendir(path);
    if (!dir) return;

    while ((entry = readdir(dir)) != NULL && file_count < MAX_FILES) {
        files[file_count] = strdup(entry->d_name);
        if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            free(files[file_count]);
            continue; // skip . and ..
        }
        file_count++;
    }
    closedir(dir);
}

void draw_ui(WINDOW *left, WINDOW *right, int height, int width) {
    debug("Drawing UI: selected=%d", selected);
    wclear(left);
    wclear(right);
    box(left, 0, 0);
    box(right, 0, 0);

    // Left panel = file list
    for (int i = 0; i < file_count && i < height - 2; i++) {
        if (i == selected) wattron(left, A_REVERSE);
        mvwprintw(left, i+1, 1, "%s", files[i]);
        if (i == selected) wattroff(left, A_REVERSE);
    }

    // Right panel = file preview / metadata
    char path[MAX_PATH];
    snprintf(path, sizeof(path), "%s/%s", current_path, files[selected]);
    char info[256];
    get_file_info(path, info, sizeof(info));
    debug("file info: %s", info); 
    mvwprintw(right, 1, 1, "Selected: %s", files[selected]);

    int row = 2;
    char *line = strtok(info, "\n");
    while (line && row < height - 1) {
        mvwprintw(right, row++, 1, "%s", line);
        line = strtok(NULL, "\n");
    }

    wrefresh(left);
    wrefresh(right);
}

void navigate(int key) {
    switch (key) {
        case KEY_UP:
            if (selected > 0) selected--;
            break;
        case KEY_DOWN:
            if (selected < file_count - 1) selected++;
            break;
        case KEY_ENTER: { // Enter key
            char path[MAX_PATH];
            snprintf(path, sizeof(path), "%s/%s", current_path, files[selected]);

            struct stat st;
            if (stat(path, &st) == 0 && S_ISDIR(st.st_mode)) {
                debug("Changing directory to: %s", path);
                chdir(path);
                getcwd(current_path, sizeof(current_path));
                scan_directory(current_path);
                selected = 0;
            }
            break;
        }
        case KEY_BACKSPACE: { // Enter key
            char path[MAX_PATH];
            snprintf(path, sizeof(path), "%s/%s", current_path, "..");
            chdir(path);
            getcwd(current_path, sizeof(current_path));
            scan_directory(current_path);
            selected = 0;
            break;
        }
        case 'q':
            endwin();
            exit(0);
    }
}

int main() {

    init_debug();
    debug("Starting file manager");

    // Init ncurses
    initscr();
    nl(); //ensure "Enter" is \n
    noecho();
    cbreak();
    keypad(stdscr, TRUE);

    // Init state
    getcwd(current_path, sizeof(current_path));
    scan_directory(current_path);

    debug("Current path: %s", current_path);

    // Split screen
    int height, width;
    getmaxyx(stdscr, height, width);
    WINDOW *left = newwin(height, width/2, 0, 0);
    WINDOW *right = newwin(height, width/2, 0, width/2);

    refresh(); //first refresh

    // Main loop
    while (1) {
        draw_ui(left, right, height, width/2);
        int ch = getch();
        navigate(ch);
    }

    endwin();
    return 0;
}
