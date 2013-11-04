/*
 * base64.h
 *
 *  Created on: 2012Äê11ÔÂ14ÈÕ
 *      Author: hhh
 */

#ifndef BASE64_H_
#define BASE64_H_

#include <stdio.h>
#include <stdlib.h>


void encodeblock( unsigned char *in, unsigned char *out, int len );
void decodeblock( unsigned char *in, unsigned char *out );

#endif /* BASE64_H_ */
