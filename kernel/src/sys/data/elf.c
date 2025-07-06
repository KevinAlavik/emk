/* EMK 1.0 Copyright (c) 2025 Piraterna */
#include <boot/emk.h>
#include <lib/assert.h>
#include <lib/string.h>
#include <mm/pmm.h>
#include <mm/vmm.h>
#include <sys/data/elf.h>
#include <util/align.h>
#include <util/log.h>

typedef struct {
    uint32_t e_magic;
    uint8_t e_class;
    uint8_t e_data;
    uint8_t e_version;
    uint8_t e_osabi;
    uint8_t e_abiversion;
    uint8_t e_pad[7];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version2;
    uint64_t e_entry;
    uint64_t e_phoff;
    uint64_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
} __attribute__((packed)) elf_header_t;

typedef struct {
    uint32_t p_type;
    uint32_t p_flags;
    uint64_t p_offset;
    uint64_t p_vaddr;
    uint64_t p_paddr;
    uint64_t p_filesz;
    uint64_t p_memsz;
    uint64_t p_align;
} __attribute__((packed)) elf_pheader_t;

#define ELF_MAGIC 0x464C457F

#define PT_LOAD 1

#define PF_X 0x1 // Execute
#define PF_W 0x2 // Write
#define PF_R 0x4 // Read

uint64_t elf_load(bool user, void* data, vctx_t* ctx) {
    assert(data);
    assert(ctx);
    elf_header_t* header = (elf_header_t*)data;

    if (header->e_magic != ELF_MAGIC) {
        log("error: Invalid ELF magic: 0x%x", header->e_magic);
        return 0;
    }

    if (header->e_class != 2) {
        log("error: Unsupported ELF class (not 64-bit): %u", header->e_class);
        return 0;
    }

    if (header->e_type != 2) {
        log("error: Unsupported ELF type (not executable): %u", header->e_type);
        return 0;
    }

    elf_pheader_t* ph = (elf_pheader_t*)((uint8_t*)data + header->e_phoff);

    for (uint16_t i = 0; i < header->e_phnum; i++) {
        if (ph[i].p_type != PT_LOAD)
            continue;

        uint64_t vaddr_start = ALIGN_DOWN(ph[i].p_vaddr, PAGE_SIZE);
        uint64_t vaddr_end = ALIGN_UP(ph[i].p_vaddr + ph[i].p_memsz, PAGE_SIZE);
        size_t pages = (vaddr_end - vaddr_start) / PAGE_SIZE;

        uint64_t flags = VALLOC_READ;
        if (ph[i].p_flags & PF_W)
            flags |= VALLOC_WRITE;
        if (ph[i].p_flags & PF_X)
            flags |= VALLOC_EXEC;
        if (user)
            flags |= VALLOC_USER;

        uint64_t phys = (uint64_t)palloc(pages, false);
        if (!phys) {
            log("error: Out of physical memory while loading ELF segment.");
            return 0;
        }

        if (!vadd(ctx, vaddr_start, phys, pages, flags)) {
            log("error: Failed to map ELF segment at 0x%lx", vaddr_start);
            pfree((void*)phys, pages);
            return 0;
        }

        memset((void*)HIGHER_HALF(phys), 0, pages * PAGE_SIZE);

        uint64_t file_offset = ph[i].p_offset;
        uint64_t mem_offset = ph[i].p_vaddr - vaddr_start;
        void* dest = (void*)(HIGHER_HALF(phys) + mem_offset);
        void* src = (uint8_t*)data + file_offset;
        memcpy(dest, src, ph[i].p_filesz);
    }

    return header->e_entry;
}
