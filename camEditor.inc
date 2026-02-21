// ============================================================
//  camEditor.inc  -  Dynamic Camera Sequence Editor
//  Project  : AdityaQ
//  Dev      : AdityaQ  |  FLUXC
//  Command  : /came
//  Requires : easyDialog.inc  (include sebelum file ini)
//  Data dir : scriptfiles/camseq/
//
//  Setup di GameMode:
//    #include <easyDialog>
//    #include <camEditor>
//    OnGameModeInit()      -> CamEditor_Init()
//    OnPlayerConnect(playerid)    -> CamEditor_OnConnect(playerid)
//    OnPlayerDisconnect(playerid) -> CamEditor_OnDisconnect(playerid)
// ============================================================

#if defined _camEditor_included
    #endinput
#endif
#define _camEditor_included

#define CE_DIR         "camseq/"
#define CE_INDEX       "camseq/index.txt"
#define CE_MAX_PTS     50
#define CE_MAX_SEQS    50
#define CE_MAX_NAME    24
#define CE_DEF_DUR     3000

#define CE_IDLE        0
#define CE_REC         1
#define CE_REC_WAIT    4
#define CE_PLAY        2
#define CE_LOOP        3

static Float:ce_cx [MAX_PLAYERS][CE_MAX_PTS];
static Float:ce_cy [MAX_PLAYERS][CE_MAX_PTS];
static Float:ce_cz [MAX_PLAYERS][CE_MAX_PTS];
static Float:ce_lx [MAX_PLAYERS][CE_MAX_PTS];
static Float:ce_ly [MAX_PLAYERS][CE_MAX_PTS];
static Float:ce_lz [MAX_PLAYERS][CE_MAX_PTS];
static       ce_dur[MAX_PLAYERS][CE_MAX_PTS];
static       ce_cnt[MAX_PLAYERS];

static ce_name  [MAX_PLAYERS][CE_MAX_NAME];
static ce_selseq[MAX_PLAYERS][CE_MAX_NAME];

static ce_status [MAX_PLAYERS];
static ce_timer  [MAX_PLAYERS];
static ce_seg    [MAX_PLAYERS];
static ce_defdur [MAX_PLAYERS];
static ce_hermite[MAX_PLAYERS];
static ce_camcut [MAX_PLAYERS]; // 0 = CAMERA_MOVE, 1 = CAMERA_CUT
static bool:ce_dialog_fix[MAX_PLAYERS];

static PlayerText:ce_td_bg  [MAX_PLAYERS];
static PlayerText:ce_td_line[MAX_PLAYERS];

forward CE_SegDone(p);

forward dialog_CE_Main(playerid, response, listitem, inputtext[]);
forward dialog_CE_RecName(playerid, response, listitem, inputtext[]);
forward dialog_CE_RecMenu(playerid, response, listitem, inputtext[]);
forward dialog_CE_SetTime(playerid, response, listitem, inputtext[]);
forward dialog_CE_PlayList(playerid, response, listitem, inputtext[]);
forward dialog_CE_ManList(playerid, response, listitem, inputtext[]);
forward dialog_CE_ManAct(playerid, response, listitem, inputtext[]);
forward dialog_CE_Opts(playerid, response, listitem, inputtext[]);

stock CamEditor_Init() {
    for (new i = 0; i < MAX_PLAYERS; i++) {
        ce_td_bg  [i] = PlayerText:INVALID_TEXT_DRAW;
        ce_td_line[i] = PlayerText:INVALID_TEXT_DRAW;
        if (IsPlayerConnected(i)) {
            CE_Reset(i);
            CE_CreateTD(i);
        }
    }
    print("[camEditor] Loaded  |  AdityaQ - FLUXC  |  /came");
}

stock CamEditor_OnConnect(playerid) {
    CE_Reset(playerid);
    CE_CreateTD(playerid);
}

stock CamEditor_OnDisconnect(playerid) {
    CE_StopTimer(playerid);
    CE_DestroyTD(playerid);
}

CMD:came(playerid, params[])
{
    if (ce_status[playerid] == CE_REC_WAIT) {
        CE_AddPoint(playerid);
        ce_status[playerid] = CE_REC;
        CE_UpdateTD(playerid);
        CE_ShowRecMenu(playerid);
        return 1;
    }
    // Stop preview -> balik ke RecMenu
    if (ce_status[playerid] == CE_PLAY && ce_dialog_fix[playerid]) {
        CE_StopTimer(playerid);
        ce_dialog_fix[playerid] = false;
        SetCameraBehindPlayer(playerid);
        ce_status[playerid] = CE_REC;
        CE_UpdateTD(playerid);
        CE_ShowRecMenu(playerid);
        return 1;
    }
    // Play/loop biasa -> stop ke idle
    if (ce_status[playerid] == CE_PLAY || ce_status[playerid] == CE_LOOP) {
        CE_StopTimer(playerid);
        SetCameraBehindPlayer(playerid);
        ce_status[playerid] = CE_IDLE;
        CE_UpdateTD(playerid);
    }
    CE_ShowMain(playerid);
    return 1;
}

Dialog:CE_Main(playerid, response, listitem, inputtext[]) {
    if (!response) return;
    switch (listitem) {
        case 0: CE_ShowRecName(playerid);
        case 1: CE_ShowPlayList(playerid);
        case 2: CE_ShowManList(playerid);
        case 3: CE_ShowOpts(playerid);
    }
}

Dialog:CE_RecName(playerid, response, listitem, inputtext[]) {
    if (!response) { CE_ShowMain(playerid); return; }

    new len = strlen(inputtext);
    if (len < 1 || len > CE_MAX_NAME - 1) {
        Dialog_Show(playerid, CE_RecName, DIALOG_STYLE_INPUT,
            "{FF4444}Nama tidak valid",
            "1-23 karakter, huruf/angka/underscore:",
            "OK", "Batal");
        return;
    }
    new valid = 1;
    for (new i = 0; i < len; i++) {
        new c = inputtext[i];
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') || c == '_')) {
            valid = 0; break;
        }
    }
    if (!valid) {
        Dialog_Show(playerid, CE_RecName, DIALOG_STYLE_INPUT,
            "{FF4444}Karakter tidak valid",
            "Hanya huruf, angka, underscore:",
            "OK", "Batal");
        return;
    }
    CE_StopTimer(playerid);
    CE_Reset(playerid);
    format(ce_name[playerid], CE_MAX_NAME, "%s", inputtext);
    ce_status[playerid] = CE_REC;
    ce_defdur[playerid] = CE_DEF_DUR;

    CE_AddPoint(playerid);
    CE_UpdateTD(playerid);
    CE_ShowRecMenu(playerid);
}

Dialog:CE_RecMenu(playerid, response, listitem, inputtext[]) {
    if (!response) {
        CE_StopTimer(playerid);
        ce_status[playerid] = CE_IDLE;
        CE_UpdateTD(playerid);
        SendClientMessage(playerid, 0xFFFFFFFF, "[camEditor] Rekaman dibatalkan.");
        return;
    }
    switch (listitem) {
        case 0: {
            ce_status[playerid] = CE_REC_WAIT;
            CE_UpdateTD(playerid);
            SendClientMessage(playerid, 0xFFFFFFFF, "[camEditor] Jalan ke posisi berikutnya, lalu /came untuk rekam.");
        }
        case 1: {
            if (ce_cnt[playerid] < 2) {
                SendClientMessage(playerid, 0xFFFFFFFF, "[camEditor] Minimal 2 titik dulu.");
                CE_ShowRecMenu(playerid);
            } else {
                ce_status[playerid] = CE_PLAY;
                ce_dialog_fix[playerid] = true;
                CE_StartPlay(playerid);
                CE_UpdateTD(playerid);
                SendClientMessage(playerid, 0xFFFFFFFF, "[camEditor] Preview... /came untuk stop.");
            }
        }
        case 2: CE_ShowSetTime(playerid);
        case 3: {
            ce_camcut[playerid] = (ce_camcut[playerid] == 0) ? 1 : 0;
            new msg[48];
            format(msg, sizeof(msg), "[camEditor] Camera type: %s", ce_camcut[playerid] == 0 ? "CAMERA_MOVE" : "CAMERA_CUT");
            SendClientMessage(playerid, 0xFFFFFFFF, msg);
            CE_ShowRecMenu(playerid);
        }
        case 4: {
            if (ce_cnt[playerid] < 2) {
                SendClientMessage(playerid, 0xFFFFFFFF, "[camEditor] Minimal 2 titik dulu.");
                CE_ShowRecMenu(playerid);
                return;
            }
            if (CE_SaveFile(playerid)) {
                new msg[72];
                format(msg, sizeof(msg), "[camEditor] '%s' tersimpan! (%d titik)", ce_name[playerid], ce_cnt[playerid]);
                SendClientMessage(playerid, 0xFFFFFFFF, msg);
            } else {
                SendClientMessage(playerid, 0xFFFFFFFF, "[camEditor] Gagal simpan! Cek folder scriptfiles/camseq/");
            }
            CE_StopTimer(playerid);
            ce_status[playerid] = CE_IDLE;
            CE_UpdateTD(playerid);
            CE_ShowMain(playerid);
        }
        case 5: {
            if (ce_cnt[playerid] > 1) {
                ce_cnt[playerid]--;
                new msg[56];
                format(msg, sizeof(msg), "[camEditor] Titik #%d dihapus.", ce_cnt[playerid] + 1);
                SendClientMessage(playerid, 0xFFFFFFFF, msg);
            } else {
                SendClientMessage(playerid, 0xFFFFFFFF, "[camEditor] Tidak bisa hapus titik pertama.");
            }
            CE_UpdateTD(playerid);
            CE_ShowRecMenu(playerid);
        }
    }
}

Dialog:CE_SetTime(playerid, response, listitem, inputtext[]) {
    if (!response) { CE_ShowRecMenu(playerid); return; }
    new ms = strval(inputtext);
    if (ms < 100 || ms > 30000) {
        Dialog_Show(playerid, CE_SetTime, DIALOG_STYLE_INPUT,
            "{FF4444}Nilai tidak valid",
            "Masukkan angka 100 - 30000 (ms):",
            "OK", "Kembali");
        return;
    }
    ce_defdur[playerid] = ms;
    if (ce_cnt[playerid] > 0)
        ce_dur[playerid][ce_cnt[playerid] - 1] = ms;
    new msg[56];
    format(msg, sizeof(msg), "[camEditor] Durasi segmen: %d ms", ms);
    SendClientMessage(playerid, 0xFFFFFFFF, msg);
    CE_ShowRecMenu(playerid);
}

Dialog:CE_PlayList(playerid, response, listitem, inputtext[]) {
    if (!response) { CE_ShowMain(playerid); return; }
    new names[CE_MAX_SEQS][CE_MAX_NAME];
    new cnt = 0;
    CE_GetList(names, cnt);
    if (listitem >= cnt) return;
    CE_StopTimer(playerid);
    if (CE_LoadFile(playerid, names[listitem])) {
        ce_status[playerid] = CE_PLAY;
        CE_StartPlay(playerid);
        CE_UpdateTD(playerid);
        new msg[72];
        format(msg, sizeof(msg), "[camEditor] Memutar '%s'... /came untuk stop.", names[listitem]);
        SendClientMessage(playerid, 0xFFFFFFFF, msg);
    } else {
        SendClientMessage(playerid, 0xFFFFFFFF, "[camEditor] Gagal muat sequence.");
    }
}

Dialog:CE_ManList(playerid, response, listitem, inputtext[]) {
    if (!response) { CE_ShowMain(playerid); return; }
    new names[CE_MAX_SEQS][CE_MAX_NAME];
    new cnt = 0;
    CE_GetList(names, cnt);
    if (listitem >= cnt) return;
    format(ce_selseq[playerid], CE_MAX_NAME, "%s", names[listitem]);
    CE_ShowManAct(playerid);
}

Dialog:CE_ManAct(playerid, response, listitem, inputtext[]) {
    if (!response) { CE_ShowManList(playerid); return; }
    switch (listitem) {
        case 0: {
            CE_StopTimer(playerid);
            if (CE_LoadFile(playerid, ce_selseq[playerid])) {
                ce_status[playerid] = CE_PLAY;
                CE_StartPlay(playerid);
                CE_UpdateTD(playerid);
                new msg[72];
                format(msg, sizeof(msg), "[camEditor] Memutar '%s'...", ce_selseq[playerid]);
                SendClientMessage(playerid, 0xFFFFFFFF, msg);
            }
        }
        case 1: {
            CE_StopTimer(playerid);
            if (CE_LoadFile(playerid, ce_selseq[playerid])) {
                ce_status[playerid] = CE_LOOP;
                CE_StartPlay(playerid);
                CE_UpdateTD(playerid);
                new msg[72];
                format(msg, sizeof(msg), "[camEditor] Loop '%s'... /came untuk stop.", ce_selseq[playerid]);
                SendClientMessage(playerid, 0xFFFFFFFF, msg);
            }
        }
        case 2: {
            CE_StopTimer(playerid);
            SetCameraBehindPlayer(playerid);
            ce_status[playerid] = CE_IDLE;
            CE_UpdateTD(playerid);
            SendClientMessage(playerid, 0xFFFFFFFF, "[camEditor] Playback dihentikan.");
            CE_ShowMain(playerid);
        }
        case 3: {
            new path[64];
            format(path, sizeof(path), "%s%s.camseq", CE_DIR, ce_selseq[playerid]);
            if (fexist(path)) {
                fremove(path);
                CE_RebuildIndex();
                new msg[64];
                format(msg, sizeof(msg), "[camEditor] '%s' dihapus.", ce_selseq[playerid]);
                SendClientMessage(playerid, 0xFFFFFFFF, msg);
            }
            CE_ShowManList(playerid);
        }
    }
}

Dialog:CE_Opts(playerid, response, listitem, inputtext[]) {
    if (!response) { CE_ShowMain(playerid); return; }
    switch (listitem) {
        case 0: {
            CE_StopTimer(playerid);
            SetCameraBehindPlayer(playerid);
            ce_status[playerid] = CE_IDLE;
            CE_UpdateTD(playerid);
            SendClientMessage(playerid, 0xFFFFFFFF, "[camEditor] Kamera dikembalikan.");
            CE_ShowMain(playerid);
        }
        case 1: {
            ce_hermite[playerid] = (ce_hermite[playerid] == 0) ? 1 : 0;
            SendClientMessage(playerid, 0xFFFFFFFF,
                ce_hermite[playerid] == 1 ?
                "[camEditor] Mode simpan: Hermite" :
                "[camEditor] Mode simpan: Linear");
            CE_ShowOpts(playerid);
        }
        case 2: {
            ce_camcut[playerid] = (ce_camcut[playerid] == 0) ? 1 : 0;
            SendClientMessage(playerid, 0xFFFFFFFF,
                ce_camcut[playerid] == 0 ?
                "[camEditor] Camera type: MOVE" :
                "[camEditor] Camera type: CUT");
            CE_ShowOpts(playerid);
        }
    }
}

static stock CE_ShowMain(playerid) {
    new title[96];
    switch (ce_status[playerid]) {
        case CE_IDLE:     format(title, sizeof(title), "{FFFF44}camEditor  {AAAAAA}| Idle");
        case CE_REC:      format(title, sizeof(title), "{FFFF44}camEditor  {FF4444}| REC: %s (%d titik)", ce_name[playerid], ce_cnt[playerid]);
        case CE_REC_WAIT: format(title, sizeof(title), "{FFFF44}camEditor  {FF9900}| Standby: %s (%d titik)", ce_name[playerid], ce_cnt[playerid]);
        case CE_PLAY:     format(title, sizeof(title), "{FFFF44}camEditor  {44FF44}| Playing: %s", ce_name[playerid]);
        case CE_LOOP:     format(title, sizeof(title), "{FFFF44}camEditor  {44FFFF}| Loop: %s", ce_name[playerid]);
    }
    Dialog_Show(playerid, CE_Main, DIALOG_STYLE_LIST, title,
        "{FF6644}Rekam Sequence Baru\n{44AAFF}Putar Sequence\n{AA44FF}Kelola Sequence\n{AAAAAA}Pengaturan",
        "Pilih", "Tutup");
}

static stock CE_ShowRecName(playerid) {
    Dialog_Show(playerid, CE_RecName, DIALOG_STYLE_INPUT,
        "{FF6644}Rekam Sequence Baru",
        "Nama sequence (huruf/angka/underscore, maks 23 karakter):",
        "Mulai", "Batal");
}

static stock CE_ShowRecMenu(playerid) {
    new title[72], modestr[10], typestr[12];
    format(modestr, sizeof(modestr), ce_hermite[playerid] == 1 ? "Hermite" : "Linear");
    format(typestr, sizeof(typestr), ce_camcut[playerid] == 0 ? "MOVE" : "CUT");
    format(title, sizeof(title), "{AAAAAA}Titik: {FFFFFF}%d {AAAAAA}Dur: {FFFFFF}%dms {AAAAAA}Type: {FFFFFF}%s", ce_cnt[playerid], ce_defdur[playerid], typestr);
    new body[320];
    format(body, sizeof(body),
        "{44FF44}+ Tambah Titik  {888888}(tutup -> gerak -> /came)\n{FFFF44}  Preview\n{AAAAAA}  Set Durasi Segmen\n{FFFFFF}  Toggle Type: {FFFF44}%s\n{44AAFF}  Selesai dan Simpan\n{FF4444}  Hapus Titik Terakhir",
        typestr);
    Dialog_Show(playerid, CE_RecMenu, DIALOG_STYLE_LIST, title, body, "Pilih", "Batalkan");
}

static stock CE_ShowSetTime(playerid) {
    new body[128];
    format(body, sizeof(body), "Durasi saat ini: {FFFF44}%d ms\n{FFFFFF}Masukkan nilai baru (100-30000):", ce_defdur[playerid]);
    Dialog_Show(playerid, CE_SetTime, DIALOG_STYLE_INPUT, "{AAAAAA}Set Durasi Segmen", body, "Set", "Kembali");
}

static stock CE_ShowPlayList(playerid) {
    new names[CE_MAX_SEQS][CE_MAX_NAME];
    new cnt = 0;
    CE_GetList(names, cnt);
    if (cnt == 0) {
        Dialog_Show(playerid, CE_PlayList, DIALOG_STYLE_MSGBOX,
            "{44AAFF}Putar Sequence", "{AAAAAA}Belum ada sequence tersimpan.", "OK", "");
        return;
    }
    new body[CE_MAX_SEQS * (CE_MAX_NAME + 20)];
    for (new i = 0; i < cnt; i++) {
        new row[CE_MAX_NAME + 20];
        format(row, sizeof(row), "{44AAFF}  {FFFFFF}%s\n", names[i]);
        strcat(body, row, sizeof(body));
    }
    Dialog_Show(playerid, CE_PlayList, DIALOG_STYLE_LIST, "{44AAFF}Putar Sequence", body, "Putar", "Kembali");
}

static stock CE_ShowManList(playerid) {
    new names[CE_MAX_SEQS][CE_MAX_NAME];
    new cnt = 0;
    CE_GetList(names, cnt);
    if (cnt == 0) {
        Dialog_Show(playerid, CE_ManList, DIALOG_STYLE_MSGBOX,
            "{AA44FF}Kelola Sequence", "{AAAAAA}Belum ada sequence tersimpan.", "OK", "");
        return;
    }
    new body[CE_MAX_SEQS * (CE_MAX_NAME + 20)];
    for (new i = 0; i < cnt; i++) {
        new row[CE_MAX_NAME + 20];
        format(row, sizeof(row), "{AA44FF}  {FFFFFF}%s\n", names[i]);
        strcat(body, row, sizeof(body));
    }
    Dialog_Show(playerid, CE_ManList, DIALOG_STYLE_LIST, "{AA44FF}Kelola Sequence", body, "Pilih", "Kembali");
}

static stock CE_ShowManAct(playerid) {
    new title[48];
    format(title, sizeof(title), "{AA44FF}%s", ce_selseq[playerid]);
    Dialog_Show(playerid, CE_ManAct, DIALOG_STYLE_LIST, title,
        "{44FF44}  Putar Sekali\n{44FFFF}  Putar Loop\n{FF8844}  Stop Playback\n{FF4444}  Hapus Sequence",
        "Pilih", "Kembali");
}

static stock CE_ShowOpts(playerid) {
    new modestr[10];
    format(modestr, sizeof(modestr), ce_hermite[playerid] == 1 ? "Hermite" : "Linear");
    new typestr[12];
    format(typestr, sizeof(typestr), ce_camcut[playerid] == 0 ? "CAMERA_MOVE" : "CAMERA_CUT");
    new body[200];
    format(body, sizeof(body),
        "{FF4444}  Stop dan Kembalikan Kamera\n{FFFFFF}  Toggle Mode Simpan: {FFFF44}%s\n{FFFFFF}  Camera Type: {FFFF44}%s",
        modestr, typestr);
    Dialog_Show(playerid, CE_Opts, DIALOG_STYLE_LIST, "{AAAAAA}Pengaturan", body, "Pilih", "Kembali");
}

static stock CE_CreateTD(playerid) {

    ce_td_bg[playerid] = CreatePlayerTextDraw(playerid, 176.0, 386.0, "LD_SPAC:white");
    PlayerTextDrawLetterSize(playerid, ce_td_bg[playerid], 0.0,   0.0);
    PlayerTextDrawTextSize(playerid, ce_td_bg[playerid], 288.0, 20.0);
    PlayerTextDrawColor(playerid, ce_td_bg[playerid], 0x00000099);
    PlayerTextDrawBackgroundColor(playerid, ce_td_bg[playerid], 0x00000000);
    PlayerTextDrawFont(playerid, ce_td_bg[playerid], 0);
    PlayerTextDrawUseBox(playerid, ce_td_bg[playerid], false);

    ce_td_line[playerid] = CreatePlayerTextDraw(playerid, 320.0, 390.0, "_");
    PlayerTextDrawAlignment(playerid, ce_td_line[playerid], 2);
    PlayerTextDrawLetterSize(playerid, ce_td_line[playerid], 0.28, 1.2);
    PlayerTextDrawColor(playerid, ce_td_line[playerid], 0xFFFFFFFF);
    PlayerTextDrawSetShadow(playerid, ce_td_line[playerid], 0);
    PlayerTextDrawSetOutline(playerid, ce_td_line[playerid], 1);
    PlayerTextDrawBackgroundColor(playerid, ce_td_line[playerid], 0x000000CC);
    PlayerTextDrawFont(playerid, ce_td_line[playerid], 2);
    PlayerTextDrawSetProportional(playerid, ce_td_line[playerid], true);
}

static stock CE_DestroyTD(playerid) {
    if (ce_td_bg[playerid] != PlayerText:INVALID_TEXT_DRAW) {
        PlayerTextDrawDestroy(playerid, ce_td_bg[playerid]);
        ce_td_bg[playerid] = PlayerText:INVALID_TEXT_DRAW;
    }
    if (ce_td_line[playerid] != PlayerText:INVALID_TEXT_DRAW) {
        PlayerTextDrawDestroy(playerid, ce_td_line[playerid]);
        ce_td_line[playerid] = PlayerText:INVALID_TEXT_DRAW;
    }
}

static stock CE_UpdateTD(playerid) {
    new status = ce_status[playerid];

    if (status == CE_IDLE) {
        PlayerTextDrawHide(playerid, ce_td_bg  [playerid]);
        PlayerTextDrawHide(playerid, ce_td_line[playerid]);
        return;
    }

    new str[128];
    switch (status) {
        case CE_REC: {
            format(str, sizeof(str), "~r~[ REC ]  ~w~%s  ~y~Titik: %d  ~w~Dur: %dms",
                ce_name[playerid], ce_cnt[playerid], ce_defdur[playerid]);
        }
        case CE_REC_WAIT: {
            format(str, sizeof(str), "~o~[ WAIT ]  ~w~%s  ~y~Titik: %d  ~w~-> /came rekam",
                ce_name[playerid], ce_cnt[playerid]);
        }
        case CE_PLAY: {
            format(str, sizeof(str), "~g~[ PLAY ]  ~w~%s  ~y~Seg: %d/%d",
                ce_name[playerid], ce_seg[playerid] + 1, ce_cnt[playerid] - 1);
        }
        case CE_LOOP: {
            format(str, sizeof(str), "~b~[ LOOP ]  ~w~%s  ~y~Seg: %d/%d",
                ce_name[playerid], ce_seg[playerid] + 1, ce_cnt[playerid] - 1);
        }
    }

    PlayerTextDrawSetString(playerid, ce_td_line[playerid], str);
    PlayerTextDrawShow(playerid, ce_td_bg  [playerid]);
    PlayerTextDrawShow(playerid, ce_td_line[playerid]);
}

public CE_SegDone(p) {
    if (ce_status[p] != CE_PLAY && ce_status[p] != CE_LOOP) return;
    ce_timer[p] = -1;
    ce_seg[p]++;

    if (ce_seg[p] >= ce_cnt[p] - 1) {
        if (ce_status[p] == CE_LOOP) {
            ce_seg[p] = 0;
            CE_PlaySegment(p);
            CE_UpdateTD(p);
            return;
        }
        SetCameraBehindPlayer(p);
        if (ce_dialog_fix[p]) {
            ce_dialog_fix[p] = false;
            ce_status[p] = CE_REC;
            CE_UpdateTD(p);
            SendClientMessage(p, 0xFFFFFFFF, "[camEditor] Preview selesai.");
            CE_ShowRecMenu(p);
        } else {
            ce_status[p] = CE_IDLE;
            CE_UpdateTD(p);
            SendClientMessage(p, 0xFFFFFFFF, "[camEditor] Sequence selesai.");
        }
        return;
    }

    CE_PlaySegment(p);
    CE_UpdateTD(p);
}

static stock CE_PlaySegment(playerid) {
    new seg = ce_seg[playerid];
    new nxt = seg + 1;
    new dur = ce_dur[playerid][seg];
    if (dur < 100) dur = CE_DEF_DUR;

    new camtype = ce_camcut[playerid] == 1 ? CAMERA_CUT : CAMERA_MOVE;

    InterpolateCameraPos(playerid,
        ce_cx[playerid][seg], ce_cy[playerid][seg], ce_cz[playerid][seg],
        ce_cx[playerid][nxt], ce_cy[playerid][nxt], ce_cz[playerid][nxt],
        dur, camtype);

    InterpolateCameraLookAt(playerid,
        ce_lx[playerid][seg], ce_ly[playerid][seg], ce_lz[playerid][seg],
        ce_lx[playerid][nxt], ce_ly[playerid][nxt], ce_lz[playerid][nxt],
        dur, camtype);

    ce_timer[playerid] = SetTimerEx("CE_SegDone", dur, false, "i", playerid);
}

static stock CE_StartPlay(playerid) {
    CE_StopTimer(playerid);
    ce_seg[playerid] = 0;

    SetPlayerCameraPos(playerid,
        ce_cx[playerid][0], ce_cy[playerid][0], ce_cz[playerid][0]);
    SetPlayerCameraLookAt(playerid,
        ce_lx[playerid][0], ce_ly[playerid][0], ce_lz[playerid][0], CAMERA_CUT);

    CE_PlaySegment(playerid);
}

static stock CE_Reset(playerid) {
    CE_StopTimer(playerid);
    ce_cnt[playerid]     = 0;
    ce_seg[playerid]     = 0;
    ce_hermite[playerid] = 1;
    ce_camcut[playerid]  = 0;
    ce_status[playerid]  = CE_IDLE;
    ce_defdur[playerid]  = CE_DEF_DUR;
    ce_name[playerid][0]   = EOS;
    ce_selseq[playerid][0] = EOS;
    ce_timer[playerid]     = -1;
}

static stock CE_StopTimer(playerid) {
    if (ce_timer[playerid] != -1) {
        KillTimer(ce_timer[playerid]);
        ce_timer[playerid] = -1;
    }
}

static stock CE_AddPoint(playerid) {
    if (ce_cnt[playerid] >= CE_MAX_PTS) {
        SendClientMessage(playerid, 0xFFFFFFFF, "[camEditor] Titik maksimal tercapai.");
        return;
    }
    new Float:cx, Float:cy, Float:cz;
    new Float:fx, Float:fy, Float:fz;
    GetPlayerCameraPos(playerid, cx, cy, cz);
    GetPlayerCameraFrontVector(playerid, fx, fy, fz);

    new idx = ce_cnt[playerid];
    ce_cx[playerid][idx] = cx;
    ce_cy[playerid][idx] = cy;
    ce_cz[playerid][idx] = cz;
    // lookat = posisi kamera + arah depan kamera * jarak
    ce_lx[playerid][idx] = cx + fx * 10.0;
    ce_ly[playerid][idx] = cy + fy * 10.0;
    ce_lz[playerid][idx] = cz + fz * 10.0;
    ce_dur[playerid][idx] = ce_defdur[playerid];
    ce_cnt[playerid]++;

    new msg[96];
    format(msg, sizeof(msg), "[camEditor] Titik #%d direkam | cam: %.1f %.1f %.1f | dur: %dms",
        ce_cnt[playerid], cx, cy, cz, ce_defdur[playerid]);
    SendClientMessage(playerid, 0xFFFFFFFF, msg);
}

static stock CE_SaveFile(playerid) {
    new path[64];
    format(path, sizeof(path), "%s%s.camseq", CE_DIR, ce_name[playerid]);
    new File:f = fopen(path, io_write);
    if (!f) return 0;

    new camtype[12];
    format(camtype, sizeof(camtype), ce_camcut[playerid] == 0 ? "CAMERA_MOVE" : "CAMERA_CUT");

    // ── Header ────────────────────────────────────────────────
    new line[256];
    fwrite(f, "// ============================================================\r\n");
    format(line, sizeof(line), "//  sequence : %s\r\n", ce_name[playerid]);         fwrite(f, line);
    format(line, sizeof(line), "//  points   : %d\r\n", ce_cnt[playerid]);          fwrite(f, line);
    format(line, sizeof(line), "//  type     : %s\r\n", camtype);                   fwrite(f, line);
    fwrite(f, "//  project  : AdityaQ - FLUXC\r\n");
    fwrite(f, "// ============================================================\r\n\r\n");

    // ── Kode siap pakai per segmen ────────────────────────────
    for (new i = 0; i < ce_cnt[playerid] - 1; i++) {
        new nxt = i + 1;
        format(line, sizeof(line), "// segment %d -> %d  (%dms)\r\n", i + 1, nxt + 1, ce_dur[playerid][i]);
        fwrite(f, line);
        format(line, sizeof(line),
            "InterpolateCameraPos(playerid, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %d, %s);\r\n",
            ce_cx[playerid][i],   ce_cy[playerid][i],   ce_cz[playerid][i],
            ce_cx[playerid][nxt], ce_cy[playerid][nxt], ce_cz[playerid][nxt],
            ce_dur[playerid][i], camtype);
        fwrite(f, line);
        format(line, sizeof(line),
            "InterpolateCameraLookAt(playerid, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %d, %s);\r\n\r\n",
            ce_lx[playerid][i],   ce_ly[playerid][i],   ce_lz[playerid][i],
            ce_lx[playerid][nxt], ce_ly[playerid][nxt], ce_lz[playerid][nxt],
            ce_dur[playerid][i], camtype);
        fwrite(f, line);
    }

    // ── Data mentah untuk loader (tetap disimpan) ─────────────
    fwrite(f, "// ── raw data (jangan diedit) ──\r\n");
    format(line, sizeof(line), "I %d %d\r\n", ce_hermite[playerid], ce_camcut[playerid]);
    fwrite(f, line);
    for (new i = 0; i < ce_cnt[playerid]; i++) {
        format(line, sizeof(line), "P %.2f %.2f %.2f %.2f %.2f %.2f %d\r\n",
            ce_cx[playerid][i], ce_cy[playerid][i], ce_cz[playerid][i],
            ce_lx[playerid][i], ce_ly[playerid][i], ce_lz[playerid][i],
            ce_dur[playerid][i]);
        fwrite(f, line);
    }
    fclose(f);
    CE_AddIndex(ce_name[playerid]);
    return 1;
}

static stock CE_LoadFile(playerid, const name[]) {
    new path[64];
    format(path, sizeof(path), "%s%s.camseq", CE_DIR, name);
    if (!fexist(path)) return 0;
    new File:f = fopen(path, io_read);
    if (!f) return 0;

    ce_cnt[playerid] = 0;
    format(ce_name[playerid], CE_MAX_NAME, "%s", name);

    new line[160];
    while (fread(f, line)) {
        new len = strlen(line);
        while (len > 0 && (line[len-1] == '\r' || line[len-1] == '\n'))
            line[--len] = EOS;
        if (len == 0 || line[0] == '/' || line[0] == '#') continue;

        if (line[0] == 'I') {
            ce_hermite[playerid] = (line[2] == '1') ? 1 : 0;
            // baca camcut dari kolom kedua: "I hermite camcut"
            new pos2 = 4;
            while (line[pos2] == ' ') pos2++;
            if (line[pos2] != EOS)
                ce_camcut[playerid] = (line[pos2] == '1') ? 1 : 0;
        } else if (line[0] == 'P' && ce_cnt[playerid] < CE_MAX_PTS) {
            new idx = ce_cnt[playerid];
            new pos = 2;
            new Float:vals[6];
            new vi = 0;
            while (line[pos] == ' ') pos++;
            while (vi < 6) {
                new start = pos;
                while (line[pos] != ' ' && line[pos] != EOS) pos++;
                new buf[24];
                strmid(buf, line, start, pos, sizeof(buf));
                vals[vi] = floatstr(buf);
                vi++;
                while (line[pos] == ' ') pos++;
            }
            new buf[12];
            strmid(buf, line, pos, strlen(line), sizeof(buf));
            new dur = strval(buf);
            if (dur < 100) dur = CE_DEF_DUR;

            ce_cx[playerid][idx]  = vals[0];
            ce_cy[playerid][idx]  = vals[1];
            ce_cz[playerid][idx]  = vals[2];
            ce_lx[playerid][idx]  = vals[3];
            ce_ly[playerid][idx]  = vals[4];
            ce_lz[playerid][idx]  = vals[5];
            ce_dur[playerid][idx] = dur;
            ce_cnt[playerid]++;
        }
    }
    fclose(f);
    return (ce_cnt[playerid] >= 2) ? 1 : 0;
}

static stock CE_GetList(names[][CE_MAX_NAME], &cnt) {
    cnt = 0;
    if (!fexist(CE_INDEX)) return;
    new File:f = fopen(CE_INDEX, io_read);
    if (!f) return;
    new line[CE_MAX_NAME + 4];
    while (fread(f, line) && cnt < CE_MAX_SEQS) {
        new len = strlen(line);
        while (len > 0 && (line[len-1] == '\r' || line[len-1] == '\n'))
            line[--len] = EOS;
        if (len == 0) continue;
        new path[64];
        format(path, sizeof(path), "%s%s.camseq", CE_DIR, line);
        if (fexist(path)) {
            format(names[cnt], CE_MAX_NAME, "%s", line);
            cnt++;
        }
    }
    fclose(f);
}

static stock CE_AddIndex(const name[]) {
    new names[CE_MAX_SEQS][CE_MAX_NAME];
    new cnt = 0;
    CE_GetList(names, cnt);
    for (new i = 0; i < cnt; i++) {
        if (strcmp(names[i], name, true) == 0) return;
    }
    new File:f = fopen(CE_INDEX, io_append);
    if (!f) return;
    new line[CE_MAX_NAME + 4];
    format(line, sizeof(line), "%s\r\n", name);
    fwrite(f, line);
    fclose(f);
}

static stock CE_RebuildIndex() {
    new names[CE_MAX_SEQS][CE_MAX_NAME];
    new cnt = 0;
    CE_GetList(names, cnt);
    new File:f = fopen(CE_INDEX, io_write);
    if (!f) return;
    for (new i = 0; i < cnt; i++) {
        new line[CE_MAX_NAME + 4];
        format(line, sizeof(line), "%s\r\n", names[i]);
        fwrite(f, line);
    }
    fclose(f);
}

stock CamEditor_Play(playerid, const name[], bool:looped) {
    CE_StopTimer(playerid);
    if (!CE_LoadFile(playerid, name)) return 0;
    ce_status[playerid] = looped ? CE_LOOP : CE_PLAY;
    CE_StartPlay(playerid);
    CE_UpdateTD(playerid);
    return 1;
}

stock CamEditor_Stop(playerid) {
    CE_StopTimer(playerid);
    SetCameraBehindPlayer(playerid);
    ce_status[playerid] = CE_IDLE;
    CE_UpdateTD(playerid);
}

stock CamEditor_IsPlaying(playerid) {
    return (ce_status[playerid] == CE_PLAY || ce_status[playerid] == CE_LOOP);
}
