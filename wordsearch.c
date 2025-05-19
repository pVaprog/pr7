#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ctype.h>
#include <wordexp.h>
#include <strings.h>

#define MAX_PATH 4096

// Проверка текстового файла по расширению
int is_text_file(const char *filename) {
    const char *ext = strrchr(filename, '.');
    if (!ext) return 0;
    
    const char *text_exts[] = {"txt", "c", "h", "md", "cpp", "hpp", "py", "java", "sh", "csv", "log", NULL};
    
    for (int i = 0; text_exts[i]; i++) {
        if (strcasecmp(ext + 1, text_exts[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

// Улучшенная проверка целого слова
int is_whole_word(const char *line, const char *word, size_t pos, size_t line_len) {
    // Проверяем символ перед словом
    if (pos > 0 && (isalnum(line[pos - 1]) || line[pos - 1] == '_')) {
        return 0;
    }
    
    // Проверяем символ после слова
    size_t word_len = strlen(word);
    size_t after_pos = pos + word_len;
    if (after_pos < line_len && (isalnum(line[after_pos]) || line[after_pos] == '_')) {
        return 0;
    }
    
    return 1;
}

// Поиск слова в файле
void search_in_file(const char *path, const char *word) {
    FILE *file = fopen(path, "r");
    if (!file) {
        perror("Ошибка открытия файла");
        return;
    }

    char line[MAX_PATH];
    int line_num = 0;
    
    while (fgets(line, sizeof(line), file)) {
        line_num++;
        size_t line_len = strlen(line);
        const char *match = line;
        
        while ((match = strstr(match, word)) != NULL) {
            size_t pos = match - line;
            if (is_whole_word(line, word, pos, line_len)) {
                char *newline = strchr(line, '\n');
                if (newline) *newline = '\0';
                printf("%s:%d: %s\n", path, line_num, line);
                break;
            }
            match++;
        }
    }
    
    fclose(file);
}

// Рекурсивный поиск в директории
void search_in_dir(const char *path, const char *word) {
    DIR *dir = opendir(path);
    if (!dir) {
        perror("Ошибка открытия директории");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;

        char full_path[MAX_PATH];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

        struct stat statbuf;
        if (stat(full_path, &statbuf) == -1) {
            perror("Ошибка получения информации о файле");
            continue;
        }

        if (S_ISDIR(statbuf.st_mode)) {
            search_in_dir(full_path, word);
        } else if (is_text_file(entry->d_name)) {
            search_in_file(full_path, word);
        }
    }

    closedir(dir);
}

// Обработка пути с ~
void expand_path(char *path) {
    if (path[0] == '~') {
        char *home = getenv("HOME");
        if (home) {
            char temp[MAX_PATH];
            snprintf(temp, sizeof(temp), "%s%s", home, path + 1);
            strcpy(path, temp);
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Использование: %s <директория> <слово>\n", argv[0]);
        printf("Пример: %s ~/files слово\n", argv[0]);
        return 1;
    }

    char dir_path[MAX_PATH];
    strncpy(dir_path, argv[1], sizeof(dir_path));
    expand_path(dir_path);

    struct stat statbuf;
    if (stat(dir_path, &statbuf) == -1 || !S_ISDIR(statbuf.st_mode)) {
        fprintf(stderr, "Ошибка: директория '%s' недоступна\n", dir_path);
        return 1;
    }

    printf("Поиск слова '%s' в %s...\n", argv[2], dir_path);
    search_in_dir(dir_path, argv[2]);
    
    return 0;
}
