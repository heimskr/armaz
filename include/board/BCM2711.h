//
// bcm2711.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2020  R. Stange <rsta2@o2online.de>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
#pragma once

#include "Config.h"

#ifdef HIGH_PERIPHERAL_MODE
#define ARM_IO_BASE 0x47e000000ul
#else
#define ARM_IO_BASE 0xfe000000ul
#endif

// External Mass Media Controller 2 (SD Card)
#define ARM_EMMC2_BASE (ARM_IO_BASE + 0x340000)

// Hardware Random Number Generator RNG200
#define ARM_HW_RNG200_BASE (ARM_IO_BASE + 0x104000)

// Generic Interrupt Controller (GIC-400)
#define ARM_GICD_BASE 0xff841000ul
#define ARM_GICC_BASE 0xff842000ul
#define ARM_GIC_END   0xff847ffful

// BCM54213PE Gigabit Ethernet Transceiver (external)
#define ARM_BCM54213_BASE 0xfd580000ul
#define ARM_BCM54213_MDIO     (ARM_BCM54213_BASE + 0x0e14)
#define ARM_BCM54213_MDIO_END (ARM_BCM54213_BASE + 0x0e1b)
#define ARM_BCM54213_END      (ARM_BCM54213_BASE + 0xffff)

// PCIe Host Bridge
#define ARM_PCIE_HOST_BASE 0xfd500000ul
#define ARM_PCIE_HOST_END  (ARM_PCIE_HOST_BASE + 0x930f)

// xHCI USB Host Controller
#ifdef USE_XHCI_INTERNAL
#define ARM_XHCI_BASE (ARM_IO_BASE + 0x9c0000)
#else
#define ARM_XHCI_BASE 0x600000000ul
#endif
#define ARM_XHCI_END (ARM_XHCI_BASE + 0x0fff)
