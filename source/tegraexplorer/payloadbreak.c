#include "payloadbreak.h"
#include "tconf.h"
#include "../storage/mountmanager.h"
#include "../storage/emummc.h"
#include "../storage/emmcfile.h"
#include "../storage/nx_emmc.h"
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

/*
 * PayloadBreakWriteBoot0
 *
 * Writes a raw binary file from SD to the BOOT0 eMMC partition.
 *
 * CRITICAL ordering constraint (research pitfall 5): DumpOrWriteEmmcPart for
 * "BOOT0" calls emummc_storage_set_mmc_partition(&emmc_storage, 1), switching
 * the eMMC partition context from GPP to BOOT0.  If SYSTEM (GPP) is still
 * mounted as bis: when this happens, the FatFS driver's partition context is
 * lost — always unmount before the BOOT0 write.
 *
 * srcPath: full SD path to the Package1 binary (e.g. "sd:/payloadbreak/pkg1.bin")
 *
 * Note: DumpOrWriteEmmcPart writes starting at BOOT0 LBA 0.  Phase 4 is
 * responsible for staging a BOOT0 image with Package1 at the correct byte
 * offset (0x100000 / sector 2048).
 *
 * Returns ErrCode_t with err == 0 on success.
 */
ErrCode_t PayloadBreakWriteBoot0(const char *srcPath) {
    /* CRITICAL: Unmount bis: before BOOT0 write.
     * DumpOrWriteEmmcPart("BOOT0") calls emummc_storage_set_mmc_partition(&emmc_storage, 1)
     * which changes the eMMC partition context from GPP to BOOT0.
     * If SYSTEM (GPP) is still mounted as bis:, the FatFS driver's partition context is lost.
     */
    if (pb_state.systemMounted) {
        unmountMMCPart();
        pb_state.systemMounted = 0;
    }

    ErrCode_t err = DumpOrWriteEmmcPart(srcPath, "BOOT0", 1, 1);
    return err;
}

/*
 * PayloadBreakWriteBcpkg2
 *
 * Writes a raw Package2 binary from SD to all three BCPKG2 partitions
 * (BCPKG2-1, BCPKG2-2, BCPKG2-3) sequentially, halting on the first failure.
 *
 * Pre-checks that all three GPT partitions exist before touching any of them
 * (research pitfall 3: BCPKG2 partition not found after emuMMC switch).
 * By verifying all three before writing any, partial writes are avoided.
 *
 * BCPKG2 partitions are raw (no BIS XTS encryption) — isSystemPartCrypt only
 * covers partition indices 0, 1, 8, 9, 10.  DumpOrWriteEmmcPart handles this
 * transparently.
 *
 * srcPath: full SD path to the Package2 binary (e.g. "sd:/payloadbreak/pkg2.bin")
 *
 * Returns ErrCode_t with err == 0 on success, or the first error encountered.
 */
ErrCode_t PayloadBreakWriteBcpkg2(const char *srcPath) {
    /* Unmount bis: if still mounted — same partition context safety as BOOT0 */
    if (pb_state.systemMounted) {
        unmountMMCPart();
        pb_state.systemMounted = 0;
    }

    /* Verify all three BCPKG2 partitions exist in GPT before writing any */
    link_t *gpt = GetCurGPT();
    if (gpt == NULL)
        return newErrCode(TE_ERR_PARTITION_NOT_FOUND);

    const char *bcpkg2Names[] = { "BCPKG2-1", "BCPKG2-2", "BCPKG2-3" };
    for (int i = 0; i < 3; i++) {
        if (nx_emmc_part_find(gpt, bcpkg2Names[i]) == NULL)
            return newErrCode(TE_ERR_PARTITION_NOT_FOUND);
    }

    /* Write to all three partitions sequentially — halt on first failure */
    for (int i = 0; i < 3; i++) {
        ErrCode_t err = DumpOrWriteEmmcPart(srcPath, bcpkg2Names[i], 1, 1);
        if (err.err != 0)
            return err;
    }

    return newErrCode(0);
}
