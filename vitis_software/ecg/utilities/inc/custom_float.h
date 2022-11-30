/*
 * custom_float.h
 *
 *  Created on: Aug 1st, 2020
 *      Author: Yarib Nevarez
 */

#ifndef LIBS_C_UTILITIES_INC_CUSTOM_FLOAT_H_
#define LIBS_C_UTILITIES_INC_CUSTOM_FLOAT_H_

#define BUILD_FLOAT(s, exponent, mantissa) ((0x80000000 & ((s) << 31)) | (0x7f800000 & (((exponent) + 0x7f) << 23)) | ((mantissa) & 0x7FFFFF))

#define DATA16_GET_EXPONENT(x) ((0x60 | ((x) >> 11 )) - 0x7F)
#define DATA16_GET_MANTISSA(x) ((0x800 | (0x7FF & (x))) << 12)

#define DATA08_GET_EXPONENT(x) ((0x70 | (0x0F & ((x) >> 4))) - 0x7F)
#define DATA04_GET_EXPONENT(x) ((0x70 | (0x0F & (x))) - 0x7F)

#define DATA32_GET_EXPONENT(x) ((0xFF & ((x) >> 23)) - 0x7F)
#define DATA32_GET_MANTISSA(x) (0x00800000 | ((0x7FFFFF) & (x)))
#define DATA32_GET_SIGN(x)     (0x80000000 & (x))

#define DATA16_TO_FLOAT32(d)  ((0xFFFF & (d)) ? (0x30000000 | (((unsigned int) (0xFFFF & (d))) << 12)) : 0)
#define DATA08_TO_FLOAT32(d)  ((0x00FF & (d)) ? (0x38000000 | (((unsigned int) (0x00FF & (d))) << 19)) : 0)

#define FLOAT32_TO_DATA16(d)  (((0xF0000000 & (unsigned int) (d)) == 0x30000000) ? (0x0000FFFF & (((unsigned int) (d)) >> 12)) : 0)
#define FLOAT32_TO_DATA08(d)  (((0xF8000000 & (unsigned int) (d)) == 0x38000000) ? (0x000000FF & (((unsigned int) (d)) >> 19)) : 0)

#endif /* LIBS_C_UTILITIES_INC_CUSTOM_FLOAT_H_ */
