# Рекурсивный поиск слова в файлах

## Описание
Утилита для поиска заданного слова во всех текстовых файлах, 
начиная с текущей директории (рекурсивно). Поддерживает:
- Рекурсивный обход поддиректорий
- Поиск целых слов и фраз
- Обработку текстовых файлов (.txt, .c, .h, .md)

## Сборка и запуск

1. Соберите программу:
```
make
```
2. Для запуска в консоле введите:
```
./wordsearch ~/PATH искомое_слово/"искомая_фраза"
```
