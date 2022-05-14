#ifndef ACPI_H
#define ACPI_H

#include "common.h"

typedef struct __attribute__((packed)) acpi_sdt_header
{
    char signature[4];
    uint32_t length;
    uint8_t revision;
    uint8_t checksum;
    char oemid[6];
    char oem_tableid[8];
    uint32_t oem_revision;
    uint32_t creator_id;
    uint32_t creator_revision;
} acpi_sdt_header_t;

typedef struct __attribute__((packed)) acpi_sdt
{
    acpi_sdt_header_t header;
    uint8_t data[0];
} acpi_sdt_t;

typedef struct __attribute__((packed)) acpi_rsdt
{
    acpi_sdt_header_t header;
    uint32_t entries[0];
} acpi_rsdt_t;

typedef struct __attribute__((packed)) acpi_xsdt
{
    acpi_sdt_header_t header;
    uint64_t entries[0];
} acpi_xsdt_t;

typedef struct __attribute__((packed)) acpi_rsdp
{
    // ACPI 1.0
    char signature[8];
    uint8_t checksum;
    char oemid[6];
    uint8_t revision;
    uint32_t rsdt_address;
    // ACPI 2.0
    uint32_t length;
    uint64_t xsdt_address;
    uint8_t ext_checksum;
    uint8_t reserved[3];
} acpi_rsdp_t;

/**
 * Founds RSDT/XSDT and checks it to be valid. 
 * Requires mb_fb_info to be valid, panics otherwise
 */
void acpi_init();

/**
 * Looks for ACPI table with specified signature. 
 * 
 * \return Pointer to the table or NULL if table not found or checksum check failed
 */
acpi_sdt_t* acpi_lookup(const char* signature);

#endif
