/*
 * LSM-Tree storage engine
 * Copyright (c) 2011, BohuTANG <overred.shuttler at gmail dot com>
 * All rights reserved.
 * Code is licensed with BSD. See COPYING.BSD file.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include "buffer.h"

int main()
{
	struct buffer *buf = buffer_new(1024);
	buffer_putc(buf,'c');
	buffer_putstr(buf, "str");
	buffer_putnstr(buf, "str", 8);
	buffer_putint(buf, 999999);

	buffer_dump(buf);

	return 0;
}
