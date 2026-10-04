#include "memory.h"
#include "command.h"
#include "device.h"

#include <stdio.h>
#include <stdint.h>
#include "hardware/regs/addressmap.h"
#include "pico/stdlib.h"
#include <stdlib.h>


extern char __flash_binary_start;
extern char __flash_binary_end;
extern char __boot2_start__;
extern char __boot2_end__;
extern char __etext;
extern char __data_start__;
extern char __data_end__;
extern char __bss_start__;
extern char __bss_end__;
extern char __HeapLimit;
extern char __StackBottom;
extern char __StackTop;

static void row(const char *name, uintptr_t start, uintptr_t end) {
    printf("%-10s 0x%08x 0x%08x %8u\n",
            name, (unsigned)start, (unsigned)end, (unsigned)(end - start));
}

void mem_info(void) {

    printf("area, start, end, size\n");   // шапка таблицы: область, начало, конец, размер

    row("flash", XIP_BASE, XIP_BASE + PICO_FLASH_SIZE_BYTES);    // flash — XIP_BASE и PICO_FLASH_SIZE_BYTES
    row("sram", SRAM_BASE, SRAM_END); // sram — базовый адрес из SDK, размер из datasheet
    row("rom", ROM_BASE, ROM_BASE + 16 * 1024); // rom — базовый адрес из SDK, размер из datasheet

    row("image", (uintptr_t)&__flash_binary_start, (uintptr_t)&__flash_binary_end); // image — от __flash_binary_start до __flash_binary_end
    row("free", (uintptr_t)&__flash_binary_end, XIP_BASE + PICO_FLASH_SIZE_BYTES); // free  — от __flash_binary_end до конца флеш-памяти
    row("boot2", (uintptr_t)&__boot2_start__, (uintptr_t)&__boot2_end__); // boot2 — от __boot2_start__ до __boot2_end__
    row("text", (uintptr_t)&__boot2_end__, (uintptr_t)&__etext); // text  — от __boot2_end__ до __etext: код и константы

    row("data flash", (uintptr_t)&__etext, (uintptr_t)&__etext + (&__data_end__ - &__data_start__)); // data flash — хранение .data, от __etext, длиной с .data
    row("data ram", (uintptr_t)&__data_start__, (uintptr_t)&__data_end__); // data ram   — работа .data, от __data_start__ до __data_end__
    row("bss", (uintptr_t)&__bss_start__, (uintptr_t)&__bss_end__); // bss        — от __bss_start__ до __bss_end__
    row("heap", (uintptr_t)&__bss_end__, (uintptr_t)&__HeapLimit); // heap       — от __bss_end__ до __HeapLimit
    row("stack", (uintptr_t)&__StackBottom, (uintptr_t)&__StackTop); // stack      — от __StackBottom до __StackTop


    uint32_t boot2_size = (unsigned)(&__boot2_end__ - &__boot2_start__);
    uint32_t text_size = (unsigned)(&__etext - &__boot2_end__);
    uint32_t data_flash_size = (unsigned)(&__data_end__ - &__data_start__);
    uint32_t flash_image_size = boot2_size + text_size + data_flash_size;
    uint32_t flash_free_size = PICO_FLASH_SIZE_BYTES - flash_image_size;

    uint32_t data_ram_size = (unsigned)(&__data_end__ - &__data_start__);
    uint32_t bss_size = (unsigned)(&__bss_end__ - &__bss_start__);
    uint32_t ram_used_size = data_ram_size + bss_size;
    uint32_t heap_size = (unsigned)(&__HeapLimit - &__bss_end__);
    uint32_t stack_size = (unsigned)(&__StackTop - &__StackBottom);


    printf("\ntotal\n");
    printf("  flash image %8d = boot2 %d + text %d + data %d\n", flash_image_size, boot2_size, text_size, data_flash_size); // итог: образ во флеш и из чего он сложился
    printf("  flash free  %8d of %d\n", flash_free_size, PICO_FLASH_SIZE_BYTES); // итог: свободно во флеш-памяти из всего её объёма
    printf("  ram used    %8d = data %d + bss %d\n", ram_used_size, data_ram_size, bss_size); // итог: занято в ОЗУ — .data и .bss
    printf("  ram free    %8d for heap and %d for stack\n", heap_size, stack_size); // итог: свободно в ОЗУ — под кучу и под стек
}

int main(void);

uint32_t data_variable = 100;
uint32_t bss_variable;

void fw_info(void) {
    data_variable++; // считаем вызов: data_variable и bss_variable на единицу больше
    bss_variable++;
    
    uint32_t stack_variable = 1946;
    uint32_t *heap_variable = malloc(sizeof(uint32_t));

    if (heap_variable != NULL) {
        *heap_variable = 1951;
    }
    
    printf("object, address, value\n"); // шапка: объект, адрес, значение
    
    uint16_t *main_code = (uint16_t *)((uintptr_t)main & ~1u);
    uint16_t *fw_info_code = (uint16_t *)((uintptr_t)fw_info & ~1u);

    printf("%-15s 0x%08x  0x%04x\n", "main", main, *main_code); // main, fw_info  — адрес с признаком Thumb и два байта по сброшенному адресу
    printf("%-15s 0x%08x  0x%04x\n", "fw_info", fw_info, *fw_info_code);

    printf("%-15s 0x%08x\n", "commands", commands); // commands       — адрес массива
    
    for (int i = 0; i < command_count; i++) {
        printf("- %-13s 0x%08x\n", commands[i].name, commands[i].handler);
    } // обработчики    — имя команды и адрес обработчика, строкой на команду
    
    printf("%-15s 0x%08x  %s\n", "DEVICE_PROJECT", &DEVICE_PROJECT, DEVICE_PROJECT); // константы      — адрес и значение строк паспорта из device.h
    printf("%-15s 0x%08x  %s\n", "DEVICE_BOARD", &DEVICE_BOARD, DEVICE_BOARD);

    printf("%-15s 0x%08x  %d\n", "data_variable", &data_variable, data_variable); // data_variable  — адрес и значение, секция .data
    printf("%-15s 0x%08x  %d\n", "bss_variable", &bss_variable, bss_variable); // bss_variable   — адрес и значение, секция .bss
    printf("%-15s 0x%08x  %d\n", "stack_variable", &stack_variable, stack_variable); // stack_variable — адрес и значение
    printf("%-15s 0x%08x  %d\n", "heap_variable", heap_variable, *heap_variable); // heap_variable  — адрес и значение

    free(heap_variable); // возвращаем блок кучи
}
