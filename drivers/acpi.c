#include "drivers/acpi.h"
#include "kernel/panic.h"
#include "kernel/multiboot.h"

static acpi_rsdt_t *rsdt = NULL;
static acpi_xsdt_t *xsdt = NULL;

static void acpi_rsdt_init();
static void acpi_xsdt_init();
static void acpi_rsdp_check(acpi_rsdp_t *rsdp);
static bool acpi_check(acpi_sdt_t *acpi_table);
static acpi_sdt_t* acpi_rsdt_lookup(const char* signature);
static acpi_sdt_t* acpi_xsdt_lookup(const char* signature);

void acpi_init()
{
    if (mb_acpi_rsdp_v2 != NULL)
        acpi_xsdt_init();
    else
        acpi_rsdt_init();
}

acpi_sdt_t* acpi_lookup(const char* signature)
{
    if (mb_acpi_rsdp_v2 != NULL)
        return acpi_xsdt_lookup(signature);
    else
        return acpi_rsdt_lookup(signature);
}

static void acpi_rsdt_init()
{
    kassert(mb_acpi_rsdp_v1 != NULL);

    acpi_rsdp_t *rsdp = (acpi_rsdp_t*)mb_acpi_rsdp_v1->rsdp;
    acpi_rsdp_check(rsdp);

    rsdt = (acpi_rsdt_t*)(uint64_t)rsdp->rsdt_address;
    if (!acpi_check((acpi_sdt_t*)rsdt))
    {
        char sign[sizeof(rsdt->header.signature) + 1] = {0};
        memcpy(sign, rsdt->header.signature, sizeof(rsdt->header.signature));
        panic("Bad ACPI RSDT checksum\n"
              "RSDT signature: %s",
              sign);
    }   

    printk("Found valid ACPI RSDT at %p\n", rsdt);
}

static void acpi_xsdt_init()
{
    kassert(mb_acpi_rsdp_v2 != NULL);

    acpi_rsdp_t *rsdp = (acpi_rsdp_t*)mb_acpi_rsdp_v2->rsdp;
    acpi_rsdp_check(rsdp);

    // RSDP 2.0 checksum check
    uint32_t checksum = 0;
    for (int i = 0; i < (int)rsdp->length; i++)
        checksum += ((uint8_t*)rsdp)[i];

    if ((checksum & 0xFF) != 0)
    {
        char sign[sizeof(rsdp->signature) + 1] = {0};;
        memcpy(sign, rsdp->signature, sizeof(rsdp->signature));
        panic("Bad ACPI 2.0 RSDP checksum\n"
              "RSDP signature: %s\n",
              sign);
    }

    xsdt = (acpi_xsdt_t*)(uint64_t)rsdp->xsdt_address;
    if (!acpi_check((acpi_sdt_t*)xsdt))
    {
        char sign[sizeof(xsdt->header.signature) + 1] = {0};
        memcpy(sign, xsdt->header.signature, sizeof(xsdt->header.signature));
        panic("Bad ACPI XSDT checksum\n"
              "XSDT signature: %s\n",
              sign);
    }

    printk("Found valid ACPI XSDT at %p\n", xsdt);
}

static void acpi_rsdp_check(acpi_rsdp_t *rsdp)
{
    // RSDP 1.0 checksum check
    uint32_t checksum = 0;
    for (int i = 0; i < (int)offsetof(acpi_rsdp_t, length); i++)
        checksum += ((uint8_t*)rsdp)[i];

    if ((checksum & 0xFF) != 0)
    {
        char sign[sizeof(rsdp->signature) + 1] = {0};
        memcpy(sign, rsdp->signature, sizeof(rsdp->signature));
        panic("Bad ACPI 1.0 RSDP checksum\n"
              "RSDP signature: %s\n",
              sign);
    }
}

static bool acpi_check(acpi_sdt_t *acpi_table)
{
    kassert_dbg(acpi_table != NULL);

    uint32_t checksum = 0;
    for (int i = 0; i < (int)acpi_table->header.length; i++)
        checksum += ((uint8_t*)acpi_table)[i];

    return (checksum & 0xFF) == 0;
}

static acpi_sdt_t* acpi_rsdt_lookup(const char* signature)
{
    size_t size = (rsdt->header.length - sizeof(rsdt->header)) / sizeof(uint32_t);
    for (size_t i = 0; i < size; i++)
    {
        acpi_sdt_t* sdt = (acpi_sdt_t*)(uint64_t)rsdt->entries[i];
        if (memcmp(signature, &sdt->header.signature, sizeof(sdt->header.signature)) == 0)
        {
            // Do checksum check
            return acpi_check((acpi_sdt_t*)rsdt) ? sdt : NULL;
        }
    }

    return NULL;
}

static acpi_sdt_t* acpi_xsdt_lookup(const char* signature)
{
    size_t size = (xsdt->header.length - sizeof(xsdt->header)) / sizeof(uint64_t);
    for (size_t i = 0; i < size; i++)
    {
        acpi_sdt_t* sdt = (acpi_sdt_t*)xsdt->entries[i];
        if (memcmp(signature, &sdt->header.signature, sizeof(sdt->header.signature)) == 0)
        {
            // Do checksum check
            return acpi_check((acpi_sdt_t*)xsdt) ? sdt : NULL;
        }
    }

    return NULL;
}
