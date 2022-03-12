#ifndef APIC_H
#define APIC_H

#include "acpi.h"

/**
 * Initializes APIC. 
 * Requires ACPI to be initialized.
 */
void apic_init();

/**
 * Signals end-of-interrupt to the APIC. 
 * Must be called before interrupt handler finishes.
 */
void apic_eoi();

#endif
