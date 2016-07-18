/*
diy-efis
Copyright (C) 2016 Kotuku Aerospace Limited

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

If a file does not contain a copyright header, either because it is incomplete
or a binary file then the above copyright notice will apply.

Portions of this repository may have further copyright notices that may be
identified in the respective files.  In those cases the above copyright notice and
the GPL3 are subservient to that copyright notice.

Portions of this repository contain code fragments from the following
providers.


If any file has a copyright notice or portions of code have been used
and the original copyright notice is not yet transcribed to the repository
then the origional copyright notice is to be respected.

If any material is included in the repository that is not open source
it must be removed as soon as possible after the code fragment is identified.
*/
#ifndef PPS_MAPS_H
#define	PPS_MAPS_H

#ifdef	__cplusplus
extern "C"
  {
#endif

#include <p33Exxxx.h>
// based on register base of 0x0670
typedef enum _rpo_pin
  {
  RP20R = 0x0680,
  RP35R = 0x0681,
  RP36R = 0x0682,
  RP37R = 0x0683,
  RP38R = 0x0684,
  RP39R = 0x0685,
  RP40R = 0x0686,
  RP41R = 0x0687,
  RP42R = 0x0688,
  RP43R = 0x0689,
  RP54R = 0x068A,
  RP55R = 0x068B,
  RP56R = 0x068C,
  RP57R = 0x068D,
  RP97R = 0x068F,
  RP118R = 0x0691,
  RP120R = 0x0692,
  } rpo_pin;
  
// based on register base of 0x06A0
typedef enum _rpi_function {
  rpi_int1 = 0x01,
  rpi_int2 = 0x02,
  rpi_t2ck = 0x06,
  rpi_ic1 = 0x0e,
  rpi_ic2 = 0x0f,
  rpi_ic3 = 0x10,
  rpi_ic4 = 0x11,
  rpi_ocfa = 0x16,
  rpi_flt1 = 0x18,
  rpi_flt2 = 0x19,
  rpi_u1rx = 0x24,
  rpi_u1cts = 0x25,
  rpi_u2rx = 0x26,
  rpi_u2cts = 0x27,
  rpi_sdi2 = 0x2c,
  rpi_sck2 = 0x2d,
  rpi_ss2 = 0x2e,
  rpi_c1rx = 0x34,
  rpi_synci1 = 0x4b,
  rpi_dtcmp1 = 0x4d,
  rpi_dtcmp2 = 0x4e,
  rpi_dtcmp3 = 0x4f,
  rpi_sent1 = 0x58,
  rpi_sent2 = 0x5a,
} rpi_function;
  
typedef enum _rpi_pin {
  rpi_vss = 0x00,
  rpi_cmp1 = 0x01,
  rpi_cmp2 = 0x02,
  rpi_cmp3 = 0x03,
  rpi_cmp4 = 0x04,
  rpi_cmp5 = 0x0c,
  rpi_rpi16 = 0x10,
  rpi_rpi17 = 0x11,
  rpi_rpi18 = 0x12,
  rpi_rpi19 = 0x13,
  rpi_rp20 = 0x14,
  rpi_rpi24 = 0x18,
  rpi_rpi25 = 0x19,
  rpi_rpi27 = 0x1b,
  rpi_rpi28 = 0x1c,
  rpi_rpi32 = 0x20,
  rpi_rpi33 = 0x21,
  rpi_rpi34 = 0x22,
  rpi_rp35 = 0x23,
  rpi_rp36 = 0x24,
  rpi_rp37 = 0x25,
  rpi_rp38 = 0x26,
  rpi_rp39 = 0x27,
  rpi_rp40 = 0x28,
  rpi_rpi44 = 0x2c,
  rpi_rpi45 = 0x2d,
  rpi_rpi46 = 0x2e,
  rpi_rpi47 = 0x2f,
  rpi_rp48 = 0x30,
  rpi_rpi50 = 0x32,
  rpi_rpi51 = 0x33,
  rpi_rpi52 = 0x34,
  rpi_rpi53 = 0x35,
  rpi_rp54 = 0x36,
  rpi_rp55 = 0x37,
  rpi_rp56 = 0x38,
  rpi_rp57 = 0x39,
  rpi_rpi58 = 0x3a,
  rpi_rpi60 = 0x3c,
  rpi_rpi61 = 0x3d,
  rpi_rpi63 = 0x3f,
  rpi_rp69 = 0x45,
  rpi_rp70 = 0x46,
  rpi_rpi72 = 0x48,
  rpi_rp41 = 0x29,
  rpi_rp42 = 0x2a,
  rpi_rp43 = 0x2b,
  rpi_rp49 = 0x31,
  rpi_rpi96 = 0x60,
  rpi_rp97 = 0x61,
  rpi_rpi94 = 0x5e,
  rpi_rpi95 = 0x5f,
  rpi_rp118 = 0x76,
  rpi_rpi119 = 0x77,
  rpi_rp120 = 0x78,
  rpi_rpi121 = 0x79,
  rpi_rpi124 = 0x7c,
  rpi_rp125 = 0x7d,
  rpi_rp126 = 0x7e,
  rpi_rp127 = 0x7f,
  rpi_rp176 = 0xb0,
  rpi_rp177 = 0xb1,
  rpi_rp178 = 0xb2,
  rpi_rp179 = 0xb3,
  rpi_rp180 = 0xb4,
  rpi_rp181 = 0xb5,
  } rpi_pin;

typedef enum _rpo_function {
  rpo_default = 0x00,
  rpo_u1tx,
  rpo_u1rts,
  rpo_u2tx,
  rpo_u2rts,
  rpo_sdo1,
  rpo_sck1out,
  rpo_ss1out,
  rpo_sd02,
  rpo_sck2out,
  rpo_ss2out,
  rpo_csdo,
  rpo_csckout,
  rpo_cofsout,
  rpo_c1tx,
  rpo_c2tx,
  rpo_oc1,
  rpo_oc2,
  rpo_oc3,
  rpo_oc4,
  rpo_oc5,
  rpo_oc6
  } rpo_function;

/**
 * Map an output pin to a device
 * @param pin       pin  0..13 not checked for device
 * @param value     map to assign
 */
extern void map_rpo(rpo_pin pin, rpo_function value);
/**
 * Map an input pin
 * @param pin
 * @param value
 */
extern void map_rpi(rpi_pin pin, rpi_function value);

#ifdef	__cplusplus
  }
#endif

#endif	/* PPS_MAPS_H */

