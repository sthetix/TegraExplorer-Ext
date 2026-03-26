#pragma once
#include <utils/types.h>
#include "../err.h"

typedef struct {
    u8 mmcTarget;      /* MMC_CONN_EMMC or MMC_CONN_EMUMMC selected by user */
    u8 systemMounted;  /* 1 if SYSTEM is currently mounted as bis:, 0 otherwise */
} pb_state_t;

extern pb_state_t pb_state;

int        PayloadBreakSelectMMC(void);
ErrCode_t  PayloadBreakMountSystem(void);
void       PayloadBreakCleanup(void);
ErrCode_t  PayloadBreakWriteBoot0(const char *srcPath);
ErrCode_t  PayloadBreakWriteBcpkg2(const char *srcPath);
