/*
 * base64.h
 *
 *  Created on: 2012��11��14��
 *      Author: hhh
 */

#ifndef BASE64_H_
#define BASE64_H_

#include <stdio.h>
#include <stdlib.h>


extern void encodeblock( unsigned char *in, unsigned char *out, int len );
extern void decodeblock( unsigned char *in, unsigned char *out );

#endif /* BASE64_H_ */
