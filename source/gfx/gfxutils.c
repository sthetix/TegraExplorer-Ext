#include "gfx.h"
#include "gfxutils.h"
#include <power/max17050.h>
#include <power/max17050.h>
#include <power/bq24193.h>
#include "../hid/hid.h"
#include <utils/sprintf.h>
#include <string.h>

void gfx_printTopInfo() {
    SETCOLOR(COLOR_CYAN, COLOR_BARS);
    gfx_con_setpos(0, 0);
    gfx_printf("TegraExplorer-Ext %d.%d.%d", LP_VER_MJ, LP_VER_MN, LP_VER_BF);
    RESETCOLOR;
}

void gfx_printBottomInfo(int page, int total_pages, int total_entries) {
    int battery = 0;
    max17050_get_property(MAX17050_RepSOC, &battery);
    int current_charge_status = 0;
    bq24193_get_property(BQ24193_ChargeStatus, &current_charge_status);

    SETCOLOR(COLOR_SOFT_WHITE, COLOR_BARS);

    // Build info string grouped together
    char info[64];
    s_printf(info, "Battery: %d%% | Page %d/%d | %d Entries", battery >> 8, page, total_pages, total_entries);

    // Fixed center position based on actual text length
    // Text is ~38 chars, screen is 1280px, center is ~640
    // Using the menu's char width multiplier (18) for better positioning
    int str_len = strlen(info);
    int x_pos = (1280 - str_len * 18) / 2;
    if (x_pos < 0) x_pos = 0;

    gfx_con_setpos(x_pos, 704);
    gfx_puts(info);
    RESETCOLOR;
}

void gfx_clearscreen(){
    gfx_boxGrey(0, 16, 1279, 703, 0x1b);

    gfx_boxGrey(0, 703, 1279, 719, 0x3D);
    gfx_boxGrey(0, 0, 1279, 15, 0x3D);


    gfx_printTopInfo();
}

MenuEntry_t YesNoEntries[] = {
    {.optionUnion = COLORTORGB(COLOR_YELLOW), .name = "No"},
    {.R = 255, .name = "Yes"}
};

int MakeYesNoHorzMenu(int spacesBetween, u32 bg){
    return MakeHorizontalMenu(YesNoEntries, ARR_LEN(YesNoEntries), spacesBetween, bg, 0);
}

int MakeHorizontalMenu(MenuEntry_t *entries, int len, int spacesBetween, u32 bg, int startPos){
    u32 initialX = 0, initialY = 0;
    u32 highlight = startPos;
    gfx_con_getpos(&initialX, &initialY);

    while (1){
        for (int i = 0; i < len; i++){
            (highlight == i) ? SETCOLOR(bg, entries[i].optionUnion) : SETCOLOR(entries[i].optionUnion, bg);
            gfx_puts(entries[i].name);
            gfx_con.y -= spacesBetween * 16;
        }
        gfx_con_setpos(initialX, initialY);
        Input_t *input = hidWait();
        if (input->a)
            return highlight;
        else if (input->b)
            return 0;
        else if ((input->left || input->down) && highlight > 0)
            highlight--;
        else if ((input->right || input->up) && highlight < len - 1)
            highlight++;
    }
}