# Описание программы

Простейший вычислитель с использованием библиотеки [SystemC](https://github.com/accellera-official/systemc)

Характеристики модулей:
- Память (Memory)
   - Размер 256 байт
   - Побайтовая адресация
- Процессор (Processor)
   - 4 8-битных регистра R0-R3

## Система команд

Программа для исполнения хранится в той же памяти, где и данные. Запрос на исполнение программы содержит адрес первой инструкции программы.
Размер каждой инструкции - 4 байта. Первый байт - тип инструкции. Адрес в памяти - 1 байт, индекс регистра - 1 байт. Каждая инструкция дополняется нулями до выравнивания размера до 4-х байт

Поддерживаемые инструкции
- 0x01: LOAD RDST [@ADDR] - загрузка значения из памяти по адресу @ADDR в регистр RDST
- 0x02: ADD RDST RSRC1 RSRC2 - сложение значений из регистров RSRC1 и RSRC2, и сохранение результата в регистр RDST
- 0x03: STORE [@ADDR] RSRC - запись значения регистра RSRC в память по адресу @ADDR
- 0x00: END - инструкция завершения программы  

# Построение проекта

## Системные требования

При разработке программы использованы:
- OS Ubuntu 24.04
- SystemC 3.0.1

## Компиляция

Проект по умолчанию предполагает установку  библиотеки SystemC в `/opt/systemc/`. В случае, если путь установки отличается, есть возможность указать путь в переменной окружения `SYSTEMC_HOME`:
```
export SYSTEMC_HOME=/path/to/systemc
```

Компиляция
```
make
```

Должен появиться файл `test_processor`

# Запуск
Указать путь до библиотеки SystemC  в переменной окружения `LD_LIBRARY_PATH`
```
export LD_LIBRARY_PATH=/opt/systemc/:$LD_LIBRARY_PATH
```

Запуск
```
./test_processor
```

Пример вывода:

```
    ./test_processor 
    SystemC 3.0.1-Accellera --- May  6 2025 08:01:13
    Copyright (c) 1996-2024 by all Contributors,
    ALL RIGHTS RESERVED
    TestBench: Signalling to start program
    OK: Received to execute from address 33
    OK: LOAD 10 to R2 from address 3
    OK: LOAD 15 to R1 from address 4
    OK: ADD R1+R2=R3, result 25
    OK: STORE 25 from R3 to address 5
    OK: LOAD 25 to R0 from address 5
    END instruction
```