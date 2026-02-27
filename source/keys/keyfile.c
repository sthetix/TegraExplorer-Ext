#include "keys.h"
#include "keyfile.h"
#include <utils/types.h>
#include <libs/fatfs/ff.h>
#include <string.h>
#include <stdlib.h>
#include <utils/ini.h>
#include "../tegraexplorer/tconf.h"
#include <storage/nx_sd.h>
#include "../gfx/gfx.h"
#include <utils/sprintf.h>

#define GetHexFromChar(c) ((c & 0x0F) + (c >= 'A' ? 9 : 0))

#define DPRINTF(x)

// ==================== Helper Functions ====================

// Check if key data is non-null (not all zeros)
static inline int _key_exists_save(const void *data) {
    return memcmp(data, "\x00\x00\x00\x00\x00\x00\x00\x00", 8) != 0;
}

char *getKey(const char *search, link_t *inilist){
    LIST_FOREACH_ENTRY(ini_sec_t, ini_sec, inilist, link){
        if (ini_sec->type == INI_CHOICE){
            LIST_FOREACH_ENTRY(ini_kv_t, kv, &ini_sec->kvs, link)
	    	{
                if (!strcmp(search, kv->key))
                    return kv->val;
	    	}
        }
    }
    return NULL;
}

void AddKey(u8 *buff, char *in, u32 len){
    if (in == NULL || strlen(in) != len * 2)
        return;

    for (int i = 0; i < len; i++)
        buff[i] = (u8)((GetHexFromChar(in[i * 2]) << 4) | GetHexFromChar(in[i * 2 + 1]));
}

int GetKeysFromFile(char *path){
    if (!sd_mount())
        return 1;

    LIST_INIT(iniList); // Whatever we'll just let this die in memory hell
    if (!ini_parse(&iniList, path, false))
        return 1;

    // add biskeys, mkey 0, header_key, save_mac_key
    AddKey(dumpedKeys.bis_key[0], getKey("bis_key_00", &iniList), AES_128_KEY_SIZE * 2);
    AddKey(dumpedKeys.bis_key[1], getKey("bis_key_01", &iniList), AES_128_KEY_SIZE * 2);
    AddKey(dumpedKeys.bis_key[2], getKey("bis_key_02", &iniList), AES_128_KEY_SIZE * 2);
    AddKey(dumpedKeys.master_key, getKey("master_key_00", &iniList), AES_128_KEY_SIZE);
    AddKey(dumpedKeys.header_key, getKey("header_key", &iniList), AES_128_KEY_SIZE * 2);
    AddKey(dumpedKeys.save_mac_key, getKey("save_mac_key", &iniList), AES_128_KEY_SIZE);

    // Verify at least one key was actually loaded (not all zeros)
    if (!_key_exists_save(dumpedKeys.bis_key[0]) &&
        !_key_exists_save(dumpedKeys.master_key)) {
        return 1; // No valid keys found in file
    }

    return 0;
}

// Helper to append a key line to the buffer
static void _append_key_line(char **buf, u32 *pos, u32 max_size, const char *key_name, const u8 *key_data, u32 len) {
    if (!_key_exists_save(key_data))
        return;

    *pos += s_printf(*buf + *pos, "%s = ", key_name);
    for (u32 i = 0; i < len; i++)
        *pos += s_printf(*buf + *pos, "%02x", key_data[i]);
    *pos += s_printf(*buf + *pos, "\n");
}

// Save dumped keys to prod.keys file
int SaveKeysToFile(const char *path) {
    if (!sd_mount())
        return 1;

    // Allocate buffer for key file content (8KB should be plenty)
    u32 buf_size = 0x2000;
    char *buf = (char *)calloc(buf_size, 1);
    if (!buf)
        return 1;

    u32 pos = 0;

    // BIS keys (most important for encrypted partition access)
    _append_key_line(&buf, &pos, buf_size, "bis_key_00", dumpedKeys.bis_key[0], AES_128_KEY_SIZE * 2);
    _append_key_line(&buf, &pos, buf_size, "bis_key_01", dumpedKeys.bis_key[1], AES_128_KEY_SIZE * 2);
    _append_key_line(&buf, &pos, buf_size, "bis_key_02", dumpedKeys.bis_key[2], AES_128_KEY_SIZE * 2);

    // Master keys
    _append_key_line(&buf, &pos, buf_size, "master_key_00", dumpedKeys.master_key, AES_128_KEY_SIZE);

    // Header key (for NCA0/NCAs)
    _append_key_line(&buf, &pos, buf_size, "header_key", dumpedKeys.header_key, AES_128_KEY_SIZE * 2);

    // Save MAC key (for save files)
    _append_key_line(&buf, &pos, buf_size, "save_mac_key", dumpedKeys.save_mac_key, AES_128_KEY_SIZE);

    // Only proceed if we have actual content
    if (pos == 0) {
        free(buf);
        return 1;
    }

    // Ensure directory exists
    f_mkdir("sd:/switch");

    // Write to file
    int res = sd_save_to_file(buf, pos, path);
    free(buf);

    return res;
}