#!/bin/sh

ethctl phy ext 0x1c 0x1c 0xFc00
ethctl phy ext 0x1c 0x00 0x1940
ethctl phy ext 0x1c 0x1c 0xfc03
ethctl phy ext 0x1c 0x00 0x1140
ethctl phy ext 0x1c 0x04 0x0060
ethctl phy ext 0x1c 0x1c 0x8c00

#100-FX 1000-X autodetect
ethctl phy ext 0x1C 0x1C 0xCC0E