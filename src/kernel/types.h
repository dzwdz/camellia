#pragma once
#include <camellia/types.h>

#define FORWARD_STRUCT(name) struct name; typedef struct name name;

FORWARD_STRUCT(CpuRegs)
FORWARD_STRUCT(Handle)
FORWARD_STRUCT(Pagedir)
FORWARD_STRUCT(Proc)
FORWARD_STRUCT(VfsBackend)
FORWARD_STRUCT(VfsMount)
FORWARD_STRUCT(VfsReq)

/* arch-specific stuff */
FORWARD_STRUCT(GfxInfo)

#undef FORWARD_STRUCT
