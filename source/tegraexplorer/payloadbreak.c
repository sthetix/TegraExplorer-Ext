#include "payloadbreak.h"
#include "tconf.h"
#include "../storage/mountmanager.h"
#include "../storage/emummc.h"
#include "../gfx/gfx.h"
#include "../gfx/gfxutils.h"
#include "../err.h"

/* Module-level state — zeroed until PayloadBreakSelectMMC succeeds */
pb_state_t pb_state = { .mmcTarget = 0, .systemMounted = 0 };

/* Menu entries for the sysMMC / emuMMC horizontal choice */
static MenuEntry_t MmcSelectEntries[] = {
    {.optionUnion = COLORTORGB(COLOR_TURQUOISE), .name = "sysMMC"},
    {.optionUnion = COLORTORGB(COLOR_TURQUOISE), .name = "emuMMC"}
};

/*
 * PayloadBreakSelectMMC
 *
 * Detects whether emuMMC is available, presents a choice to the user if so,
 * connects the selected MMC, and stores the selection in pb_state.mmcTarget.
 *
 * Returns 0 on success, non-zero if connectMMC() fails (hardware init error).
 * On failure TConf.currentMMCConnected remains MMC_CONN_None and pb_state is
 * not updated.
 */
int PayloadBreakSelectMMC(void) {
    u8 selectedType;
    int choice;
    int res;

    /* Refresh emuMMC detection state from SD card ini */
    emummc_load_cfg();

    if (emu_cfg.enabled != 0) {
        /* emuMMC is present — let the user choose */
        gfx_printf("Select NAND target:\n");
        choice = MakeHorizontalMenu(MmcSelectEntries, 2, 6, COLOR_DEFAULT, 0);

        /*
         * choice == 0  → sysMMC (first entry)
         * choice == 1  → emuMMC (second entry)
         * choice <  0  → back/cancel → default to sysMMC
         */
        if (choice == 1) {
            selectedType = MMC_CONN_EMUMMC;
        } else {
            selectedType = MMC_CONN_EMMC;
        }
    } else {
        /* No emuMMC found — fall back to sysMMC automatically */
        gfx_printf("No emuMMC detected, using sysMMC\n");
        selectedType = MMC_CONN_EMMC;
    }

    /*
     * connectMMC() handles h_cfg.emummc_force_disable internally.
     * Do NOT set it directly here — see research pitfall 1.
     */
    res = connectMMC(selectedType);
    if (res != 0) {
        /* Hardware init failed; leave pb_state unchanged */
        return res;
    }

    pb_state.mmcTarget = selectedType;
    return 0;
}

/*
 * PayloadBreakMountSystem
 *
 * Mounts the SYSTEM partition of the already-connected MMC as the bis: FatFS
 * volume.  On mount failure the hardware connection is torn down (SAFE-02) so
 * no partial state remains.
 *
 * Returns ErrCode_t with err == 0 on success.
 */
ErrCode_t PayloadBreakMountSystem(void) {
    ErrCode_t err = mountMMCPart("SYSTEM");

    if (err.err != 0) {
        /*
         * Mount failed — disconnect the MMC so callers cannot accidentally
         * attempt further operations against a partially-initialised stack.
         * SAFE-02: no partial state on any failure path.
         */
        disconnectMMC();
        pb_state.mmcTarget = 0;
        return err;
    }

    pb_state.systemMounted = 1;
    return newErrCode(0);
}

/*
 * PayloadBreakCleanup
 *
 * Full teardown: unmounts bis: if mounted, disconnects MMC if connected, and
 * resets pb_state to its initial zero state.  Safe to call at any point,
 * including on error paths where only part of the init sequence completed.
 */
void PayloadBreakCleanup(void) {
    if (pb_state.systemMounted) {
        unmountMMCPart();
        pb_state.systemMounted = 0;
    }

    if (TConf.currentMMCConnected != MMC_CONN_None) {
        disconnectMMC();
    }

    pb_state.mmcTarget = 0;
}
