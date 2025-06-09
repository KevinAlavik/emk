/* EMK 1.0 Copyright (c) 2025 Piraterna */
#ifndef MADT_H
#define MADT_H

#include <stdint.h>
#include <sys/acpi.h>

#define MADT_ENTRY_LAPIC 0          /* Local APIC */
#define MADT_ENTRY_IOAPIC 1         /* I/O APIC */
#define MADT_ENTRY_IOAPIC_SRC_OVR 2 /* I/O APIC Interrupt Source Override */
#define MADT_ENTRY_IOAPIC_NMI_SRC 3 /* I/O APIC Non-Maskable Interrupt Source  \
                                     */
#define MADT_ENTRY_LAPIC_NMI 4      /* Local APIC Non-Maskable Interrupt */
#define MADT_ENTRY_LAPIC_ADDR_OVR 5 /* Local APIC Address Override */
#define MADT_ENTRY_LX2APIC 9        /* Local x2APIC */

typedef struct acpi_madt {
    acpi_sdt_header_t sdt;
    uint32_t lapic_base;
    uint32_t flags;
    char table[];
} __attribute__((packed)) acpi_madt_t;

typedef struct acpi_madt_entry {
    uint8_t type;
    uint8_t length;
} __attribute__((packed)) acpi_madt_entry_t;

/* Entry Type 0: Processor Local APIC */
typedef struct acpi_madt_lapic {
    acpi_madt_entry_t header;
    uint8_t acpi_proc_id; /* ACPI Processor ID */
    uint8_t apic_id;      /* APIC ID */
    uint32_t flags;       /* Bit 0: Processor Enabled, Bit 1: Online Capable */
} __attribute__((packed)) acpi_madt_lapic_t;

/* Entry Type 1: I/O APIC */
typedef struct acpi_madt_ioapic {
    acpi_madt_entry_t header;
    uint8_t ioapic_id;    /* I/O APIC's ID */
    uint8_t reserved;     /* Reserved (0) */
    uint32_t ioapic_addr; /* I/O APIC Address */
    uint32_t gsi_base;    /* Global System Interrupt Base */
} __attribute__((packed)) acpi_madt_ioapic_t;

/* Entry Type 2: I/O APIC Interrupt Source Override */
typedef struct acpi_madt_ioapic_src_ovr {
    acpi_madt_entry_t header;
    uint8_t bus_source; /* Bus Source */
    uint8_t irq_source; /* IRQ Source */
    uint32_t gsi;       /* Global System Interrupt */
    uint16_t flags;     /* Flags */
} __attribute__((packed)) acpi_madt_ioapic_src_ovr_t;

/* Entry Type 3: I/O APIC Non-Maskable Interrupt Source */
typedef struct acpi_madt_ioapic_nmi_src {
    acpi_madt_entry_t header;
    uint8_t nmi_source; /* NMI Source */
    uint8_t reserved;   /* Reserved */
    uint16_t flags;     /* Flags */
    uint32_t gsi;       /* Global System Interrupt */
} __attribute__((packed)) acpi_madt_ioapic_nmi_src_t;

/* Entry Type 4: Local APIC Non-Maskable Interrupt */
typedef struct acpi_madt_lapic_nmi {
    acpi_madt_entry_t header;
    uint8_t acpi_proc_id; /* ACPI Processor ID (0xFF for all processors) */
    uint16_t flags;       /* Flags */
    uint8_t lint;         /* LINT# (0 or 1) */
} __attribute__((packed)) acpi_madt_lapic_nmi_t;

/* Entry Type 5: Local APIC Address Override */
typedef struct acpi_madt_lapic_addr_ovr {
    acpi_madt_entry_t header;
    uint16_t reserved;   /* Reserved */
    uint64_t lapic_addr; /* 64-bit Physical Address of Local APIC */
} __attribute__((packed)) acpi_madt_lapic_addr_ovr_t;

/* Entry Type 9: Processor Local x2APIC */
typedef struct acpi_madt_lx2apic {
    acpi_madt_entry_t header;
    uint16_t reserved;
    uint32_t x2apic_id; /* Processor's Local x2APIC ID */
    uint32_t flags;     /* Flags (same as Local APIC) */
    uint32_t acpi_id;   /* ACPI ID */
} __attribute__((packed)) acpi_madt_lx2apic_t;

extern acpi_madt_ioapic_t* madt_ioapic_list[256];
extern acpi_madt_ioapic_src_ovr_t* madt_iso_list[256];
extern acpi_madt_lapic_t* madt_lapic_list[256];
extern acpi_madt_lapic_nmi_t* madt_lapic_nmi_list[256];
extern acpi_madt_ioapic_nmi_src_t* madt_ioapic_nmi_list[256];
extern acpi_madt_lx2apic_t* madt_lx2apic_list[256];
extern uint32_t madt_ioapic_len;
extern uint32_t madt_iso_len;
extern uint32_t madt_lapic_len;
extern uint32_t madt_lapic_nmi_len;
extern uint32_t madt_ioapic_nmi_len;
extern uint32_t madt_lx2apic_len;
extern uint64_t lapic_addr;

void madt_init();

#endif // MADT_H