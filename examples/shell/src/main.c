/*
 * Copyright (c) 2025 Blues Inc.
 *
 * MIT License. Use of this source code is governed by licenses granted
 * by the copyright holder including that found in the LICENSE file.
 *
 * Author: Alex R. Bucknall
 */

#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>

// Include Notecard note-c library
#include <note.h>

LOG_MODULE_REGISTER(main);

static int cmd_notecard_sync(const struct shell *shell, size_t argc, char **argv)
{
    J *req = NoteNewRequest("hub.sync");
    J *rsp = NoteRequestResponse(req);

    shell_fprintf(shell, SHELL_NORMAL, "Notehub Product UID: %s\n", CONFIG_BLUES_NOTEHUB_PRODUCT_UID);

    if (rsp != NULL)
    {
        shell_fprintf(shell, SHELL_NORMAL, "Syncing Notecard with Notehub...\n");
    }
    else
    {
        shell_fprintf(shell, SHELL_ERROR, "Failed to sync Notecard with Notehub.\n");
    }

    NoteDeleteResponse(rsp);
    return 0;
}

static int cmd_notecard_status(const struct shell *shell, size_t argc, char **argv)
{
    J *req = NoteNewRequest("card.status");
    J *rsp = NoteRequestResponse(req);

    if (rsp != NULL)
    {
        char *status = JGetString(rsp, "status");
        bool usb = JGetBool(rsp, "usb");
        const char* usb_str = (usb == true) ? "true" : "false";
        int storage = JGetInt(rsp, "storage");
        int time = JGetInt(rsp, "time");
        bool connected = JGetBool(rsp, "connected");
        const char* connected_str = (connected == true) ? "true" : "false";
        bool cell = JGetBool(rsp, "cell");
        const char* cell_str = (cell == true) ? "true" : "false";
        bool sync = JGetBool(rsp, "sync");
        const char* sync_str = (sync == true) ? "true" : "false";
        int inbound = JGetInt(rsp, "inbound");
        int outbound = JGetInt(rsp, "outbound");

        shell_fprintf(shell, SHELL_NORMAL, "status: %s\nusb: %s\nstorage: %d\ntime: %d\nconnected: %s\ncell: %s\nsync: %s\ninbound: %d\noutbound: %d\n", status, usb_str, storage, time, connected_str, cell_str, sync_str, inbound, outbound);

        NoteDeleteResponse(rsp);

        return 0;
    }
    else {
        shell_fprintf(shell, SHELL_ERROR, "Failed to get Notecard status.\n");
        NoteDeleteResponse(rsp);

        return -1;
    }
}

SHELL_STATIC_SUBCMD_SET_CREATE(
    notecard_cmds,
    SHELL_CMD(status, NULL, "Get Notecard status", cmd_notecard_status),
    SHELL_CMD(sync, NULL, "Sync Notecard with Notehub", cmd_notecard_sync),
    SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(notecard, &notecard_cmds, "Notecard commands", NULL);

int main(void)
{
    // Initialize note-c hooks
    NoteSetUserAgent((char *)"note-zephyr");

    return 0;
}
