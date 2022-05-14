#ifndef APIC_H
#define APIC_H

#include "drivers/acpi.h"

/**
 * Initializes APIC. 
 * Requires ACPI to be initialized.
 */
void apic_init();

/**
 * Initializes APIC timer
 */
void apic_setup_timer();

/**
 * Signals end-of-interrupt to the APIC. 
 * Must be called before interrupt handler finishes.
 */
void apic_eoi();

#endif
