# texteditor

Терминальный текстовый редактор на C++20 — учебный проект с упором на аккуратную,
хорошо протестированную реализацию структуры хранения текста.

Ядро — **gap buffer** собственной реализации: с SSO, поддержкой кастомных аллокаторов,
полным набором итераторов и гарантиями безопасности при исключениях. Слои редактора и
терминала пока в разработке (см. статус ниже и [ROADMAP.md](ROADMAP.md)).

> **Статус проекта:** ранняя разработка. Готов и покрыт тестами слой `buffer/`.
> Слои `editor/` и `terminal/`, а также `main.cpp` — пока пустые заглушки, поэтому
> **бинарник `texteditor` ещё не собирается** (нет `main()`). Собирается и запускается
> цель `tests`.

## Архитектура

Строгая слоистость, зависимости идут только вниз: `terminal → editor → buffer`.
Каждый слой ничего не знает о слоях выше.

```
src/
  buffer/     — gap_buffer: хранение текста (нет зависимостей от UI)   ✅ готов
  editor/     — логика: режимы, курсор, команды (vim-like)            ⬜ в планах
  terminal/   — raw-ввод и отрисовка в терминал                       ⬜ в планах
  main.cpp    — точка входа, главный цикл                             ⬜ в планах
tests/        — gtest; buffer/ покрыт ~100 тестами
benchmarks/   — Google Benchmark (заготовка)
scripts/      — coverage.sh
```

## `gap_buffer`

Gap buffer хранит текст как единый массив с «дырой» (gap) в позиции курсора: вставка и
удаление рядом с курсором — амортизированно O(1), а перемещение курсора двигает дыру.
Это классическая структура для текстовых редакторов.

Что уже реализовано:

- **Small String Optimization** — короткий текст (≤ 16 символов) живёт на стеке, без
  обращения к куче.
- **Кастомные аллокаторы** — шаблон по `Allocator`-концепту, с `allocator_traits` и
  rebind; аллокатор занимает ноль байт при пустом состоянии (`[[no_unique_address]]`).
- **Полный набор итераторов** — `random_access_iterator`, const/non-const, реверсивные,
  `operator<=>`, корректная арифметика через gap.
- **Безопасность при исключениях** — при броске в конструкторе/`reserve` объект
  откатывается в согласованное состояние, память освобождается.
- **Богатый API** — конструкторы из `std::string` / `const char*` / размера, `insert`,
  `erase_in_front_of_cursor` / `erase_in_back_of_cursor`, `step` (движение курсора),
  `reserve` / `shrink_to_fit` / `resize`, индексация `operator[]` / `at`, `to_string`.

Пример:

```cpp
#include "buffer/buffer.hpp"

gap_buffer buf("hello");   // CTAD → gap_buffer<std::allocator<char>>
buf.step(-2);              // курсор между 'l' и 'l'
buf.insert("XYZ");         // "helXYZlo"
buf.erase_in_front_of_cursor();
// buf.to_string() == "helXYlo"
```

## Сборка и запуск

Требуется C++20-компилятор (clang++ рекомендуется), CMake ≥ 3.20 и GoogleTest.

```bash
# Arch Linux
sudo pacman -S clang cmake gtest llvm
```

### Тесты

Пока `main.cpp` пуст, собираем и гоняем именно цель `tests`:

```bash
cmake -B build -DCMAKE_CXX_COMPILER=clang++
cmake --build build --target tests -j$(nproc)
./build/tests
# или: cd build && ctest --output-on-failure
```

### Основной бинарник

Заработает, как только в `src/main.cpp` появится `main()`:

```bash
cmake --build build -j$(nproc)
./build/texteditor
```

## Качество кода

- **CI** (`.github/workflows/ci.yml`) — матрица gcc × clang, конфигурации default и ASan,
  на каждый push и pull request.
- **AddressSanitizer** — ловит утечки и ошибки памяти, в т.ч. при исключениях:
  ```bash
  cmake -B build-asan -DCMAKE_CXX_COMPILER=clang++ -DASAN=ON
  cmake --build build-asan --target tests && ./build-asan/tests
  ```
- **Coverage** (clang source-based) — HTML-отчёт в `coverage/index.html`:
  ```bash
  ./scripts/coverage.sh
  ```
- **Тесты по категориям** — construction / correctness / smoke / memory / регрессии
  итератора. Подробности и соглашения — в [DEVNOTES.md](DEVNOTES.md).

## Что планируется

Кратко (полный план с вехами — в [ROADMAP.md](ROADMAP.md)):

1. **Модель хранения** — построчный гибрид: весь файл как `gap_buffer<std::string>`
   (внешний gap по строкам) + активный `gap_buffer<char>` для строки под курсором.
   Даёт O(1) навигацию и O(1) аморт. вставку/удаление строк; выбран под цель редактора
   кода (LSP/подсветка/отрисовка — построчные). Требует темплейтизации буфера по `T`.
2. **Слой `editor/`** (ближайший фокус) — модальный, vim-like: режимы Normal/Insert/Visual,
   движения (`hjkl`, `w`/`b`, `gg`/`G`), правки (`i`/`a`/`o`, `x`, `dd`), visual-выделение
   и операторы `d`/`y`/`c`, регистр и `p`. Курсор `(row, col)`: строка = позиция внешнего
   gap, колонка = позиция gap активной строки.
3. **Слой `terminal/`** — raw-режим (`termios`), декодирование ввода (escape-последовательности),
   отрисовка видимой области и статус-строки, главный цикл в `main.cpp`.
4. **Файловый I/O** — открытие/сохранение файла, `texteditor <file>`, команды `:w` / `:q`.
5. **Производительность** — бенчмарки, diff-отрисовка, undo/redo, профиль памяти на файлах
   с большим числом строк.

## Документация

- [ROADMAP.md](ROADMAP.md) — детальный план по вехам и зафиксированные проектные решения.
- [DEVNOTES.md](DEVNOTES.md) — сборка, тесты, coverage, соглашения по написанию тестов.
