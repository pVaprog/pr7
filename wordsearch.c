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
#define MAX_LINE 2048

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

// Проверка на границы слова
int is_word_boundary(char c) {
    return !isalnum(c) && c != '_';
}

// Поиск точного совпадения слова или фразы
int find_exact_match(const char *line, const char *phrase) {
    const char *match = line;
    size_t phrase_len = strlen(phrase);
    
    while ((match = strstr(match, phrase)) != NULL) {
        size_t pos = match - line;
        
        // Проверяем границы перед фразой
        int before_ok = (pos == 0) || is_word_boundary(line[pos - 1]);
        
        // Проверяем границы после фразы
        int after_ok = (pos + phrase_len == strlen(line)) || 
                      is_word_boundary(line[pos + phrase_len]);
        
        if (before_ok && after_ok) {
            return 1; // Нашли точное совпадение
        }
        
        match++; // Продолжаем поиск
    }
    
    return 0; // Точное совпадение не найдено
}

// Поиск фразы в файле
void search_in_file(const char *path, const char *phrase) {
    FILE *file = fopen(path, "r");
    if (!file) {
        perror("Ошибка открытия файла");
        return;
    }

    char line[MAX_LINE];
    int line_num = 0;
    
    while (fgets(line, sizeof(line), file)) {
        line_num++;
        size_t line_len = strlen(line);
        
        // Удаляем символ новой строки для вывода
        if (line_len > 0 && line[line_len-1] == '\n') {
            line[line_len-1] = '\0';
            line_len--;
        }
        
        if (find_exact_match(line, phrase)) {
            printf("%s:%d: %s\n", path, line_num, line);
        }
    }
    
    fclose(file);
}

// Рекурсивный поиск в директории
void search_in_dir(const char *path, const char *phrase) {
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
            search_in_dir(full_path, phrase);
        } else if (is_text_file(entry->d_name)) {
            search_in_file(full_path, phrase);
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
        printf("Использование: %s <директория> <слово или фраза>\n", argv[0]);
        printf("Пример: %s ~/files \"точная фраза\"\n", argv[0]);
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

    printf("Поиск фразы '%s' в %s...\n", argv[2], dir_path);
    search_in_dir(dir_path, argv[2]);
    
    return 0;
}
