/**********************************************************************
 * $Id$
 *
 * GEOS - Geometry Engine Open Source
 * http://geos.refractions.net
 *
 * Copyright (C) 2001-2002 Vivid Solutions Inc.
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU Lesser General Public Licence as published
 * by the Free Software Foundation. 
 * See the COPYING file for more information.
 *
 **********************************************************************/


#include <geos/io.h>
#include <geos/util.h>

namespace geos {
namespace io {

int ByteOrderValues::ENDIAN_BIG = 1;
int ByteOrderValues::ENDIAN_LITTLE = 2;

int
ByteOrderValues::getInt(const byte *buf, int byteOrder)
{
	if ( byteOrder == ENDIAN_BIG )
	{
		return ((int)buf[0]<<24)|
			((int)buf[1]<<16)|
			((int)buf[2]<<8)|
			((int)buf[3]);
	}
	else // ENDIAN_LITTLE
	{
		return ((int)buf[3]<<24)|
			((int)buf[2]<<16)|
			((int)buf[1]<<8)|
			((int)buf[0]);
	}
}

void
ByteOrderValues::putInt(int intValue, byte *buf, int byteOrder)
{
	if ( byteOrder == ENDIAN_BIG )
	{
		buf[0] = (byte)(intValue >> 24);
		buf[1] = (byte)(intValue >> 16);
		buf[2] = (byte)(intValue >> 8);
		buf[3] = (byte) intValue;
	}
	else // ENDIAN_LITTLE
	{
		buf[3] = (byte)(intValue >> 24);
		buf[2] = (byte)(intValue >> 16);
		buf[1] = (byte)(intValue >> 8);
		buf[0] = (byte) intValue;
	}
}

int64
ByteOrderValues::getLong(const byte *buf, int byteOrder)
{
	if ( byteOrder == ENDIAN_BIG )
	{
		return
			(int64) (buf[0]) << 56
			| (int64) (buf[1] & 0xff) << 48
			| (int64) (buf[2] & 0xff) << 40
			| (int64) (buf[3] & 0xff) << 32
			| (int64) (buf[4] & 0xff) << 24
			| (int64) (buf[5] & 0xff) << 16
			| (int64) (buf[6] & 0xff) <<  8
			| (int64) (buf[7] & 0xff);
	}
	else // ENDIAN_LITTLE
	{
		return
			(int64) (buf[7]) << 56
			| (int64) (buf[6] & 0xff) << 48
			| (int64) (buf[5] & 0xff) << 40
			| (int64) (buf[4] & 0xff) << 32
			| (int64) (buf[3] & 0xff) << 24
			| (int64) (buf[2] & 0xff) << 16
			| (int64) (buf[1] & 0xff) <<  8
			| (int64) (buf[0] & 0xff);
	}
}

void
ByteOrderValues::putLong(int64 longValue, byte *buf, int byteOrder)
{
	if ( byteOrder == ENDIAN_BIG )
	{
		buf[0] = (byte)(longValue >> 56);
		buf[1] = (byte)(longValue >> 48);
		buf[2] = (byte)(longValue >> 40);
		buf[3] = (byte)(longValue >> 32);
		buf[4] = (byte)(longValue >> 24);
		buf[5] = (byte)(longValue >> 16);
		buf[6] = (byte)(longValue >> 8);
		buf[7] = (byte) longValue;
	}
	else // ENDIAN_LITTLE
	{
		buf[7] = (byte)(longValue >> 56);
		buf[6] = (byte)(longValue >> 48);
		buf[5] = (byte)(longValue >> 40);
		buf[4] = (byte)(longValue >> 32);
		buf[3] = (byte)(longValue >> 24);
		buf[2] = (byte)(longValue >> 16);
		buf[1] = (byte)(longValue >> 8);
		buf[0] = (byte) longValue;
	}
}

double
ByteOrderValues::getDouble(const byte *buf, int byteOrder)
{
	int64 longValue = getLong(buf, byteOrder);
	double ret;
	memcpy(&ret, &longValue, sizeof(double));
}

void
ByteOrderValues::putDouble(double doubleValue, byte *buf, int byteOrder)
{
	int64 longValue;
	memcpy(&longValue, &doubleValue, sizeof(double));
	putLong(longValue, buf, byteOrder);
}

} // namespace io
} // namespace geos

/**********************************************************************
 * $Log$
 * Revision 1.1  2005/04/20 17:22:47  strk
 * Added initial implementation of WKBReaderT and ByteOrderDataInStreamT
 * class templates and ByteOrderValues class.
 * Work is unfinished as WKBReader requires new interface of CoordinateSequence
 * taking higher dimensions into account.
 *
 **********************************************************************/