#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ctype.h>

#define BUFSIZE_PATH 4096
#define BUFSIZE_LINE 2048

// Проверяем, является ли файл текстовым по расширению
int ext_is_text(const char *fname) {
    const char *dot = strrchr(fname, '.');
    if (!dot) return 0;
    const char *text_exts[] = {"txt", "c", "h", "md", "cpp", "hpp", "py", "java", "sh", "csv", "log", NULL};
    for (int i = 0; text_exts[i]; i++) {
        if (strcasecmp(dot + 1, text_exts[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

// Проверяем, допустим ли символ перед/после слова (для строгой границы)
int is_word_separator(char c) {
    // Пробел, табуляция, конец строки, начало строки, перевод строки, знаки препинания
    return c == '\0' || c == '\n' || c == '\r' || isspace((unsigned char)c) ||
           c == '.' || c == ',' || c == ';' || c == ':' || c == '!' || c == '?' || c == '(' || c == ')' || c == '[' || c == ']' || c == '"' || c == '\'';
}

// Строгий поиск слова/фразы по правилам
int strict_phrase_match(const char *line, const char *pattern) {
    size_t linelen = strlen(line);
    size_t patlen = strlen(pattern);

    if (patlen == 0) return 0;
    const char *p = line;
    while ((p = strstr(p, pattern)) != NULL) {
        // Символ до фразы (или начало строки)
        char before = (p == line) ? ' ' : *(p - 1);
        // Символ после фразы (или конец строки)
        char after = ((p + patlen) >= (line + linelen)) ? '\0' : *(p + patlen);

        if (is_word_separator(before) && is_word_separator(after)) {
            return 1;
        }
        p += patlen;
    }
    return 0;
}

// Поиск по одному файлу, возвращает 1 если что-то найдено
int search_file(const char *filepath, const char *pattern) {
    FILE *fp = fopen(filepath, "r");
    if (!fp) {
        perror("Не удалось открыть файл");
        return 0;
    }
    char buffer[BUFSIZE_LINE];
    int linenum = 0, found = 0;
    while (fgets(buffer, sizeof(buffer), fp)) {
        linenum++;
        // Удаляем символ новой строки
        size_t l = strlen(buffer);
        if (l > 0 && buffer[l-1] == '\n') buffer[l-1] = '\0';
        if (strict_phrase_match(buffer, pattern)) {
            printf("%s:%d: %s\n", filepath, linenum, buffer);
            found = 1;
        }
    }
    fclose(fp);
    return found;
}

// Рекурсивный обход папки, возвращает 1 если найдено
int scan_folder(const char *folderpath, const char *pattern) {
    DIR *dp = opendir(folderpath);
    if (!dp) {
        perror("Не удалось открыть директорию");
        return 0;
    }
    struct dirent *de;
    int any_found = 0;
    while ((de = readdir(dp)) != NULL) {
        if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, "..")) continue;
        char newpath[BUFSIZE_PATH];
        snprintf(newpath, sizeof(newpath), "%s/%s", folderpath, de->d_name);

        struct stat s;
        if (stat(newpath, &s) == -1) {
            perror("stat");
            continue;
        }
        if (S_ISDIR(s.st_mode)) {
            if (scan_folder(newpath, pattern)) any_found = 1;
        } else if (ext_is_text(de->d_name)) {
            if (search_file(newpath, pattern)) any_found = 1;
        }
    }
    closedir(dp);
    return any_found;
}

// Поддержка ~ в пути
void expand_home(char *p) {
    if (p[0] == '~') {
        const char *home = getenv("HOME");
        if (home) {
            char temp[BUFSIZE_PATH];
            snprintf(temp, sizeof(temp), "%s%s", home, p + 1);
            strcpy(p, temp);
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Использование: %s <директория> <слово или фраза>\n", argv[0]);
        printf("Пример: %s ~/files \"ваша фраза\"\n", argv[0]);
        return 1;
    }
    char dir[BUFSIZE_PATH];
    strncpy(dir, argv[1], sizeof(dir));
    dir[sizeof(dir) - 1] = '\0';
    expand_home(dir);

    struct stat st;
    if (stat(dir, &st) == -1 || !S_ISDIR(st.st_mode)) {
        fprintf(stderr, "Ошибка: директория '%s' недоступна\n", dir);
        return 1;
    }

    printf("Поиск \"%s\" в %s...\n", argv[2], dir);
    int result = scan_folder(dir, argv[2]);
    if (!result) {
        printf("Совпадений для \"%s\" не найдено в %s\n", argv[2], dir);
    }
    return 0;
}
