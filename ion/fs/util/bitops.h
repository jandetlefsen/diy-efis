/* FILE: ion_bitops.h */
/**************************************************************************
* Copyright (C)2009 Spansion LLC and its licensors. All Rights Reserved. 
*
* This software is owned by Spansion or its licensors and published by: 
* Spansion LLC, 915 DeGuigne Dr. Sunnyvale, CA  94088-3453 ("Spansion").
*
* BY DOWNLOADING, INSTALLING OR USING THIS SOFTWARE, YOU AGREE TO BE BOUND 
* BY ALL THE TERMS AND CONDITIONS OF THIS AGREEMENT.
*
* This software constitutes source code for use in programming Spansion's Flash 
* memory components. This software is licensed by Spansion to be adapted only 
* for use in systems utilizing Spansion's Flash memories. Spansion is not be 
* responsible for misuse or illegal use of this software for devices not 
* supported herein.  Spansion is providing this source code "AS IS" and will 
* not be responsible for issues arising from incorrect user implementation 
* of the source code herein.  
*
* SPANSION MAKES NO WARRANTY, EXPRESS OR IMPLIED, ARISING BY LAW OR OTHERWISE, 
* REGARDING THE SOFTWARE, ITS PERFORMANCE OR SUITABILITY FOR YOUR INTENDED 
* USE, INCLUDING, WITHOUT LIMITATION, NO IMPLIED WARRANTY OF MERCHANTABILITY, 
* FITNESS FOR A  PARTICULAR PURPOSE OR USE, OR NONINFRINGEMENT.  SPANSION WILL 
* HAVE NO LIABILITY (WHETHER IN CONTRACT, WARRANTY, TORT, NEGLIGENCE OR 
* OTHERWISE) FOR ANY DAMAGES ARISING FROM USE OR INABILITY TO USE THE SOFTWARE, 
* INCLUDING, WITHOUT LIMITATION, ANY DIRECT, INDIRECT, INCIDENTAL, 
* SPECIAL, OR CONSEQUENTIAL DAMAGES OR LOSS OF DATA, SAVINGS OR PROFITS, 
* EVEN IF SPANSION HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.  
*
* This software may be replicated in part or whole for the licensed use, 
* with the restriction that this Copyright notice must be included with 
* this software, whether used in part or whole, at all times.  
*/



#if !defined( BITOPS_H_01032006 )
#define BITOPS_H_01032006

/*-----------------------------------------------------------------------------
 INCLUDE HEADER FILES
-----------------------------------------------------------------------------*/

#include "../global/global.h"




/*-----------------------------------------------------------------------------
 DEFINE DEFINITIONS
-----------------------------------------------------------------------------*/

/* define bit size */
#define BITS_PER_UINT8  8
#define BITS_UINT8_MASK  (BITS_PER_UINT8-1)
#define SHIFT_PER_UINT8  3
#define BITS_UINT8_ALL_ONE  0xff

#define BITS_PER_UINT16  16
#define BITS_UINT16_MASK  (BITS_PER_UINT16-1)
#define SHIFT_PER_UINT16  4
#define BITS_UINT16_ALL_ONE  0xffff

#define BITS_PER_UINT32  32
#define BITS_UINT32_MASK  (BITS_PER_UINT32-1)
#define SHIFT_PER_UINT32  5
#define BITS_UINT32_ALL_ONE  0xffffffff

#define BITS_ALL_ZERO  0

#define bit_val(val)       (1<<val)




/*-----------------------------------------------------------------------------
 INCLUDE HEADER FILES
-----------------------------------------------------------------------------*/

/*
 Name: bit_set
 Desc: Set a bit at a specified position of a 32-bit value to 1.
 Params:
   - addr: A address of the bit array containing the bit.
   - bit: The position of the bit in the value to be set to 1.
 Returns: None
 Caveats: None
*/

static inline void bit_set( uint32_t *addr, uint32_t bit )
{
   *addr |= (1<<bit);
}




/*
 Name: bit_clear
 Desc: Clear a bit at a specified position of a 32-bit value to 1.
 Params:
   - addr: A address of the bit array containing the bit.
   - bit: The position of the bit in the value to be cleared.
 Returns: None
 Caveats: None
*/

static inline void bit_clear( uint32_t *addr, uint32_t bit )
{
   *addr &= ~(uint32_t)(1<<bit);
}




/*
 Name: bit_get
 Desc: Get the value (1 or 0) of a specified bit in a bit array.
 Params:
   - word: The bit array containing the bit.
   - bit: The position of the bit for which to return the status.
 Returns:
   uint32_t value The return value is a bit at a specified position.
 Caveats: None
*/

static inline uint32_t bit_get( uint32_t word, uint32_t bit )
{
   return (word & (1<<bit)) >> bit;
}




/*
 Name: bit_tst
 Desc: Test bit.
 Params:
   - addr: A address of the bit array containing the bit.
   - bit: The position of the bit in the value to be tested.
 Returns:
   bool true or false
 Caveats: None
*/

static inline bool bit_tst( uint32_t *addr, uint32_t bit )
{
   return (*addr & bit ? true : false);
}




/*
 Name: bit_ffz
 Desc: find first zero.
 Params:
   - word: The value which will be found first zero.
 Returns:
   uint32_t >0 The return value is a position of first zero.
 Caveats: undefined if no zero exists so code should check against ~0UL first.
*/

static inline uint32_t bit_ffz( uint32_t word )
{
   uint32_t k;

   word = ~word;
   k = 31;

   if ( 0x0000ffff & word ) { k -= 16; word <<= 16; }
   if ( 0x00ff0000 & word ) { k -= 8; word <<= 8; }
   if ( 0x0f000000 & word ) { k -= 4; word <<= 4; }
   if ( 0x30000000 & word ) { k -= 2; word <<= 2; }
   if ( 0x40000000 & word ) { k -= 1; }

   return k;
}




/*
 Name: bit_ffo
 Desc: find first one from LSB.
 Params:
   - word: The value which will be found first one.
 Returns:
   uint32_t >0 The return value is a position of first one from LSB.
 Caveats: undefined if no zero exists so code should check against ~0UL first.
*/

static inline uint32_t bit_ffo( uint32_t word )
{
   uint32_t k = 31;

   if ( 0x0000ffff & word ) { k -= 16; word <<= 16; }
   if ( 0x00ff0000 & word ) { k -= 8; word <<= 8; }
   if ( 0x0f000000 & word ) { k -= 4; word <<= 4; }
   if ( 0x30000000 & word ) { k -= 2; word <<= 2; }
   if ( 0x40000000 & word ) { k -= 1; }

   return k;
}




/*
 Name: bit_flo
 Desc: find first one from MSB.
 Params:
   - word: The value which will be found first one.
 Returns:
   uint32_t >0 The return value is a position of first one from MSB.
 Caveats: undefined if no zero exists so code should check against ~0UL first.
*/

static inline uint32_t bit_flo( uint32_t word )
{
   uint32_t k = 0;

   if ( 0xffff0000 & word ) { k += 16; word >>= 16; }
   if ( 0x0000ff00 & word ) { k += 8; word >>= 8; }
   if ( 0x000000f0 & word ) { k += 4; word >>= 4; }
   if ( 0x0000000c & word ) { k += 2; word >>= 2; }
   if ( 0x00000002 & word ) { k += 1; }

   return k;
}
#endif

/*-----------------------------------------------------------------------------
 END OF FILE
-----------------------------------------------------------------------------*/

