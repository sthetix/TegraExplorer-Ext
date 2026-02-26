#include "mainmenu.h"
#include "../gfx/gfx.h"
#include "../gfx/gfxutils.h"
#include "../gfx/menu.h"
#include "tools.h"
#include "../hid/hid.h"
#include "../fs/menus/explorer.h"
#include <utils/btn.h>
#include <storage/nx_sd.h>
#include "tconf.h"
#include "../keys/keys.h"
#include "../storage/mountmanager.h"
#include "../storage/gptmenu.h"
#include "../storage/emummc.h"
#include <utils/util.h>
#include "../fs/fsutils.h"
#include <soc/fuse.h>
#include "../utils/utils.h"
#include "../config.h"
#include "../fs/readers/folderReader.h"
#include <string.h>
#include <mem/heap.h>
#include "../fs/menus/filemenu.h"

//#define INCLUDE_BUILTIN_SCRIPTS 1
//#define SCRIPT_ONLY 1

#ifdef INCLUDE_BUILTIN_SCRIPTS
#include "../../build/TegraExplorer/script/builtin.h"
#endif

extern hekate_config h_cfg;

enum {
    #ifndef SCRIPT_ONLY
    MainExplore = 0,
    MainBrowseSd,
    MainMountSd,
    MainBrowseEmmc,
    MainBrowseEmummc,
    MainTools,
    MainPartitionSd,
    MainViewKeys,
    MainViewCredits,
    MainLoad,
    #else
    MainLoad = 0,
    #endif
    MainLoadLockpick,
    MainReboot,
    MainRebootRCM,
    MainRebootNormal,
    MainRebootHekate,
    MainScripts,
    MainExit,
    MainPowerOff,
};

MenuEntry_t mainMenuEntries[] = {
    #ifndef SCRIPT_ONLY
    [MainExplore] = {.optionUnion = COLORTORGB(COLOR_TURQUOISE) | SKIPBIT, .name = "-- Explore --"},
    [MainBrowseSd] = {.optionUnion = COLORTORGB(COLOR_SOFT_WHITE), .name = "Browse SD"},
    [MainMountSd] = {.optionUnion = COLORTORGB(COLOR_SOFT_WHITE)}, // To mount/unmount the SD
    [MainBrowseEmmc] = {.optionUnion = COLORTORGB(COLOR_SOFT_WHITE), .name = "Browse EMMC"},
    [MainBrowseEmummc] = {.optionUnion = COLORTORGB(COLOR_SOFT_WHITE), .name = "Browse EMUMMC"},
    [MainTools] = {.optionUnion = COLORTORGB(COLOR_TURQUOISE) | SKIPBIT, .name = "\n-- Tools --"},
    [MainPartitionSd] = {.optionUnion = COLORTORGB(COLOR_SOFT_WHITE), .name = "Partition the sd"},
    [MainViewKeys] = {.optionUnion = COLORTORGB(COLOR_SOFT_WHITE), .name = "View dumped keys"},
    [MainViewCredits] = {.optionUnion = COLORTORGB(COLOR_SOFT_WHITE), .name = "Credits"},
    [MainLoad] = {.optionUnion = COLORTORGB(COLOR_TURQUOISE) | SKIPBIT, .name = "\n-- Load --"},
    #else
    [MainLoad] = {.optionUnion = COLORTORGB(COLOR_TURQUOISE), .name = "\n-- Load --"},
    #endif
    [MainLoadLockpick] = {.optionUnion = COLORTORGB(COLOR_SOFT_WHITE), .name = "Load Lockpick_RCM"},
    [MainReboot] = {.optionUnion = COLORTORGB(COLOR_TURQUOISE) | SKIPBIT, .name = "\n-- Reboot --"},
    [MainRebootRCM] = {.optionUnion = COLORTORGB(COLOR_SOFT_WHITE), .name = "Reboot to RCM"},
    [MainRebootNormal] = {.optionUnion = COLORTORGB(COLOR_SOFT_WHITE), .name = "Reboot normally"},
    [MainRebootHekate] = {.optionUnion = COLORTORGB(COLOR_SOFT_WHITE), .name = "Reboot to Hekate"},
    [MainScripts] = {.optionUnion = COLORTORGB(COLOR_TURQUOISE) | SKIPBIT, .name = "\n-- Scripts --"},
    // MainExit and MainPowerOff are added dynamically after scripts
};

void HandleSD(){
    gfx_clearscreen();
    TConf.curExplorerLoc = LOC_SD;
    if (!sd_mount() || sd_get_card_removed()){
        gfx_printf("Sd is not mounted!");
        hidWait();
    }
    else
        FileExplorer("sd:/");
}

void HandleEMMC(){
   GptMenu(MMC_CONN_EMMC);
}

void HandleEMUMMC(){
    GptMenu(MMC_CONN_EMUMMC);
}

void ViewKeys(){
    gfx_clearscreen();
    for (int i = 0; i < 3; i++){
        gfx_printf("\nBis key 0%d:   ", i);
        PrintKey(dumpedKeys.bis_key[i], AES_128_KEY_SIZE * 2);
    }
    
    gfx_printf("\nMaster key 0: ");
    PrintKey(dumpedKeys.master_key, AES_128_KEY_SIZE);
    gfx_printf("\nHeader key:   ");
    PrintKey(dumpedKeys.header_key, AES_128_KEY_SIZE * 2);
    gfx_printf("\nSave mac key: ");
    PrintKey(dumpedKeys.save_mac_key, AES_128_KEY_SIZE);

    u8 fuseCount = 0;
    for (u32 i = 0; i < 32; i++){
        if ((fuse_read_odm(7) >> i) & 1)
            fuseCount++;
    }

    gfx_printf("\n\nPkg1 ID: '%s'\nFuse count: %d", TConf.pkg1ID, fuseCount);

    hidWait();
}

void ViewCredits(){
    gfx_clearscreen();
    gfx_printf("\nTegraexplorer v%d.%d.%d\nBy SuchMemeManySkill\n\nBased on Lockpick_RCM & Hekate, from shchmue & CTCaer\n\n\n", LP_VER_MJ, LP_VER_MN, LP_VER_BF);

    if (hidRead()->r)
        gfx_printf("%k\"I'm not even sure if it works\" - meme", COLOR_ORANGE);

    hidWait();
}

extern bool sd_mounted;
extern bool is_sd_inited;
extern int launch_payload(char *path);

void RebootToHekate(){
    launch_payload("sd:/bootloader/update.bin");
}

void LaunchLockpick(){
    // Check for Lockpick_RCM_Pro.bin first (HATS pack uses this)
    if (FileExists("sd:/bootloader/payloads/Lockpick_RCM_Pro.bin"))
        launch_payload("sd:/bootloader/payloads/Lockpick_RCM_Pro.bin");
    // Fallback to regular Lockpick_RCM.bin
    else if (FileExists("sd:/bootloader/payloads/Lockpick_RCM.bin"))
        launch_payload("sd:/bootloader/payloads/Lockpick_RCM.bin");
}

void MountOrUnmountSD(){
    gfx_clearscreen();
    if (sd_mounted)
        sd_unmount();
    else if (!sd_mount())
        hidWait();
}

menuPaths mainMenuPaths[] = {
    #ifndef SCRIPT_ONLY
    [MainBrowseSd] = HandleSD,
    [MainMountSd] = MountOrUnmountSD,
    [MainBrowseEmmc] = HandleEMMC,
    [MainBrowseEmummc] = HandleEMUMMC,
    [MainPartitionSd] = FormatSD,
    [MainViewKeys] = ViewKeys,
    [MainViewCredits] = ViewCredits,
    #endif
    [MainLoadLockpick] = LaunchLockpick,
    [MainRebootHekate] = RebootToHekate,
    [MainRebootRCM] = reboot_rcm,
    [MainRebootNormal] = reboot_normal,
    [MainPowerOff] = power_off,
};

void EnterMainMenu(){
    int res = 0;
    while (1){
        if (sd_get_card_removed())
            sd_unmount();

        #ifndef SCRIPT_ONLY
        // -- Explore --
        mainMenuEntries[MainBrowseSd].hide = !sd_mounted;
        mainMenuEntries[MainMountSd].name = (sd_mounted) ? "Unmount SD" : "Mount SD";
        mainMenuEntries[MainBrowseEmummc].hide = (!emu_cfg.enabled || !sd_mounted);

        // -- Tools --
        mainMenuEntries[MainPartitionSd].hide = (!is_sd_inited || sd_get_card_removed());
        mainMenuEntries[MainViewKeys].hide = !TConf.keysDumped;

        // -- Load --
        mainMenuEntries[MainLoadLockpick].hide = (!sd_mounted || (!FileExists("sd:/bootloader/payloads/Lockpick_RCM_Pro.bin") && !FileExists("sd:/bootloader/payloads/Lockpick_RCM.bin")));

        // -- Reboot --
        mainMenuEntries[MainRebootHekate].hide = (!sd_mounted || !FileExists("sd:/bootloader/update.bin"));
        mainMenuEntries[MainRebootRCM].hide = h_cfg.t210b01;
        #endif
        // -- Scripts --
        #ifndef INCLUDE_BUILTIN_SCRIPTS
        mainMenuEntries[MainScripts].hide = (!sd_mounted || (!FileExists("sd:/tegraexplorer/scripts") && !FileExists("sd:/scripts")));
        #else
        mainMenuEntries[MainScripts].hide = ((!sd_mounted || (!FileExists("sd:/tegraexplorer/scripts") && !FileExists("sd:/scripts"))) && !EMBEDDED_SCRIPTS_LEN);
        #endif

        Vector_t ent = newVec(sizeof(MenuEntry_t), ARRAY_SIZE(mainMenuEntries));
        ent.count = ARRAY_SIZE(mainMenuEntries);
        memcpy(ent.data, mainMenuEntries, sizeof(MenuEntry_t) * ARRAY_SIZE(mainMenuEntries));
        Vector_t scriptFiles = {0};
        Vector_t scriptFilesRoot = {0};
        u8 hasScripts = 0;

        #ifdef INCLUDE_BUILTIN_SCRIPTS
        for (int i = 0; i < EMBEDDED_SCRIPTS_LEN; i++){
            MenuEntry_t m = {.name = embedded_scripts_g[i].name, .optionUnion = COLORTORGB(COLOR_TURQUOISE), .icon = 128};
            vecAdd(&ent, m);
        }
        #endif

        if (sd_mounted && FileExists("sd:/tegraexplorer/scripts")){
            scriptFiles = ReadFolder("sd:/tegraexplorer/scripts", &res);
            if (!res){
                if (!scriptFiles.count){
                    FREE(scriptFiles.data);
                }
                else {
                    hasScripts = 1;
                    vecForEach(FSEntry_t*, scriptFile, (&scriptFiles)){
                        if (!scriptFile->isDir && StrEndsWith(scriptFile->name, ".te")){
                            MenuEntry_t a = MakeMenuOutFSEntry(*scriptFile);
                            vecAdd(&ent, a);
                        }
                    }
                }
            }
        }

        if (sd_mounted && FileExists("sd:/scripts")){
            scriptFilesRoot = ReadFolder("sd:/scripts", &res);
            if (!res){
                if (!scriptFilesRoot.count){
                    FREE(scriptFilesRoot.data);
                }
                else {
                    hasScripts = 1;
                    vecForEach(FSEntry_t*, scriptFile, (&scriptFilesRoot)){
                        if (!scriptFile->isDir && StrEndsWith(scriptFile->name, ".te")){
                            MenuEntry_t a = MakeMenuOutFSEntry(*scriptFile);
                            vecAdd(&ent, a);
                        }
                    }
                }
            }
        }

        // Add Exit and PowerOff AFTER all scripts (at the bottom)
        MenuEntry_t exitHeader = {.optionUnion = COLORTORGB(COLOR_TURQUOISE) | SKIPBIT, .name = "\n-- Exit --"};
        vecAdd(&ent, exitHeader);
        MenuEntry_t powerOffEntry = {.optionUnion = COLORTORGB(COLOR_RED), .name = "Power off"};
        vecAdd(&ent, powerOffEntry);

        // Check if any scripts were added (count would be > base + 2 if scripts exist)
        if (!hasScripts){
            mainMenuEntries[MainScripts].hide = 1;
            if (scriptFiles.data) clearFileVector(&scriptFiles);
            if (scriptFilesRoot.data) clearFileVector(&scriptFilesRoot);
        }
        

        gfx_clearscreen();
        gfx_putc('\n');

        // Calculate base index for dynamic entries (Exit + PowerOff always at end)
        u32 dynamicBase = ent.count - 2; // Exit header and PowerOff are last 2 entries
        u32 baseCount = ARRAY_SIZE(mainMenuEntries) + 2; // Static entries + Exit + PowerOff

        res = newMenu(&ent, res, 79, 30, (ent.count == baseCount) ? ALWAYSREDRAW : ALWAYSREDRAW | ENABLEPAGECOUNT, ent.count - baseCount);

        // Handle Exit header (skip) and PowerOff (last entry)
        if (res == dynamicBase + 1){ // PowerOff is at dynamicBase + 1
            power_off();
        }
        else if (res < MainScripts && mainMenuPaths[res] != NULL)
            mainMenuPaths[res]();
        #ifndef INCLUDE_BUILTIN_SCRIPTS
        else if (hasScripts){
        #else
        else {
            if (res - ARRAY_SIZE(mainMenuEntries) < EMBEDDED_SCRIPTS_LEN){
                char *script = embedded_scripts_g[res - ARRAY_SIZE(mainMenuEntries)].script;
                RunScriptString(script, strlen(script));
            }
            else {
            #endif
                vecDefArray(MenuEntry_t*, entArray, ent);
                MenuEntry_t entry = entArray[res];
                FSEntry_t fsEntry = {.name = entry.name, .sizeUnion = entry.sizeUnion};

                // Check which folder the script is from
                int scriptFound = 0;
                if (scriptFiles.data) {
                    vecForEach(FSEntry_t*, scriptFile, (&scriptFiles)){
                        if (!scriptFile->isDir && strcmp(scriptFile->name, entry.name) == 0){
                            RunScript("sd:/tegraexplorer/scripts", fsEntry);
                            scriptFound = 1;
                            break;
                        }
                    }
                }
                if (!scriptFound && scriptFilesRoot.data) {
                    vecForEach(FSEntry_t*, scriptFile, (&scriptFilesRoot)){
                        if (!scriptFile->isDir && strcmp(scriptFile->name, entry.name) == 0){
                            RunScript("sd:/scripts", fsEntry);
                            break;
                        }
                    }
                }
            #ifdef INCLUDE_BUILTIN_SCRIPTS
            }
            #endif
        }

        if (hasScripts){
            if (scriptFiles.data) clearFileVector(&scriptFiles);
            if (scriptFilesRoot.data) clearFileVector(&scriptFilesRoot);
        }

        free(ent.data);
    }
}

