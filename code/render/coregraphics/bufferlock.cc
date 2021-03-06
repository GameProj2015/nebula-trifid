//------------------------------------------------------------------------------
//  bufferlock.cc
//  (C) 2012-2015 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "coregraphics/bufferlock.h"
#if __OGL4__
namespace CoreGraphics
{
__ImplementClass(CoreGraphics::BufferLock, 'BUFL', OpenGL4::OGL4BufferLock);
}
#else
#error "Bufferlock class not implemented on this platform!"
#endif