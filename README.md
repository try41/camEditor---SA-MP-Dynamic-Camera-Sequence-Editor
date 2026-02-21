# camEditor
> SA-MP / open.mp | AdityaQ — FLUXC

In-game camera sequence editor. Record camera positions while flying,
preview in real-time, and export ready-to-use pawn code.

## Features
- Record camera points using actual camera position & direction
- Free fly mode compatible (GetPlayerCameraPos + GetPlayerCameraFrontVector)
- Preview sequences in-game before saving
- Export `.camseq` file as ready-to-paste Pawn code
- InterpolateCameraPos + InterpolateCameraLookAt per segment
- Toggle CAMERA_MOVE / CAMERA_CUT per sequence
- Adjustable duration per segment (100ms - 30000ms)
- Loop & single playback mode
- HUD status textdraw (bawah tengah layar)
- Multi-sequence management (save, load, delete)
- Max 50 points per sequence, 50 sequences total

## Requirements
- easyDialog.inc
- zcmd (atau YCMD)

## Commands
| Command | Description |
|---------|-------------|
| `/came` | Open camera editor menu |

## Setup
```pawn
#include <easyDialog>
#include <camEditor>

OnGameModeInit()      -> CamEditor_Init()
OnPlayerConnect(playerid)    -> CamEditor_OnConnect(playerid)
OnPlayerDisconnect(playerid) -> CamEditor_OnDisconnect(playerid)
```

## Output File Example
```pawn
// segment 1 -> 2  (3000ms)
InterpolateCameraPos(playerid, 123.45, 456.78, 10.50, 200.10, 300.20, 15.00, 3000, CAMERA_MOVE);
InterpolateCameraLookAt(playerid, 133.45, 466.78, 11.50, 210.10, 310.20, 16.00, 3000, CAMERA_MOVE);
```

## Changelog
### v1.0.0
- Initial release
- Record, preview, save, load, delete sequences
- InterpolateCameraPos & InterpolateCameraLookAt playback
- CAMERA_MOVE / CAMERA_CUT toggle
- HUD textdraw status
- Export siap pakai sebagai kode Pawn

## Include what is needed
https://github.com/Awsomedude/easyDialog
