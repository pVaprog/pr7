#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ctype.h>
#include <wordexp.h>

#define MAX_PATH 4096

// Проверяем, является ли файл текстовым (по расширению)
int is_text_file(const char *filename) {
    const char *ext = strrchr(filename, '.');
    if (!ext) return 0;  // Файлы без расширения не считаем текстовыми
    
    // Список текстовых расширений
    const char *text_exts[] = {"txt", "c", "h", "md", "cpp", "hpp", "py", "java", "sh", "csv", "log", NULL};
    
    for (int i = 0; text_exts[i]; i++) {
        if (strcasecmp(ext + 1, text_exts[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

// Проверяем, является ли найденное совпадение целым словом
int is_whole_word(const char *line, const char *word, size_t pos) {
    // Проверяем символ перед словом
    if (pos > 0 && isalnum(line[pos - 1])) {
        return 0;
    }
    
    // Проверяем символ после слова
    size_t word_len = strlen(word);
    if (line[pos + word_len] != '\0' && isalnum(line[pos + word_len])) {
        return 0;
    }
    
    return 1;
}

// Ищем слово в файле и выводим результаты
void search_in_file(const char *path, const char *word) {
    FILE *file = fopen(path, "r");
    if (!file) {
        perror("Ошибка открытия файла");
        return;
    }

    char line[MAX_PATH];
    int line_num = 0;
    int found_any = 0;
    
    while (fgets(line, sizeof(line), file)) {
        line_num++;
        const char *match = line;
        
        // Ищем все вхождения слова в строке
        while ((match = strstr(match, word)) != NULL) {
            size_t pos = match - line;
            if (is_whole_word(line, word, pos)) {
                // Убираем символ новой строки для красивого вывода
                char *newline = strchr(line, '\n');
                if (newline) *newline = '\0';
                
                printf("%s:%d: %s\n", path, line_num, line);
                found_any = 1;
                break; // Нашли одно вхождение - переходим к следующей строке
            }
            match++;
        }
    }
    
    fclose(file);
}

// Рекурсивно ищем в директории (включая скрытые файлы и папки)
void search_in_dir(const char *path, const char *word) {
    DIR *dir = opendir(path);
    if (!dir) {
        perror("Ошибка открытия директории");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        // Пропускаем . и ..
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // Формируем полный путь
        char full_path[MAX_PATH];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

        struct stat statbuf;
        if (lstat(full_path, &statbuf) == -1) {
            perror("Ошибка получения информации о файле");
            continue;
        }

        if (S_ISDIR(statbuf.st_mode)) {
            // Рекурсивный поиск в поддиректориях
            search_in_dir(full_path, word);
        } else if (is_text_file(entry->d_name)) {
            // Поиск в текстовых файлах
            search_in_file(full_path, word);
        }
    }

    closedir(dir);
}

// Разворачиваем путь с ~ в абсолютный путь
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
        printf("Использование: %s <директория> <слово_для_поиска>\n", argv[0]);
        printf("Пример: %s ~/files искомое_слово\n", argv[0]);
        return 1;
    }

    // Обрабатываем путь директории
    char dir_path[MAX_PATH];
    strncpy(dir_path, argv[1], sizeof(dir_path));
    expand_path(dir_path);

    const char *word = argv[2];
    
    // Проверяем существование директории
    struct stat statbuf;
    if (stat(dir_path, &statbuf) == -1 || !S_ISDIR(statbuf.st_mode)) {
        fprintf(stderr, "Ошибка: директория '%s' не существует или недоступна\n", dir_path);
        return 1;
    }

    printf("Поиск слова '%s' в директории %s...\n", word, dir_path);
    search_in_dir(dir_path, word);
    
    return 0;
}
