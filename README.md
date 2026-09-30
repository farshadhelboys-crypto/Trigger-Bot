# FARSHAD PM — Trigger Bot

**ساخته شده توسط فرشادی پی ام**  
Support: Telegram **@farshad_pm_org**

Modern Windows trigger assistant with neon UI, configurable hotkeys, fast-sniper mode, and professional logging.

## Features

| Feature | Description |
|--------|-------------|
| **Trigger** | Hold activation key → detects center-pixel change → auto fire |
| **Custom hotkeys** | Click any hotkey row and press a new key |
| **Fast Sniper** | When enabled, sends `Q` twice within ~0.5s (scope toggle helper) |
| **Sensitivity** | Color tolerance + reaction delay |
| **Logger** | Timestamped in-app log + `logs/farshad_pm.log` |
| **Config** | Saved to `%APPDATA%\\FarshadPM\\config.ini` |

## Build (Visual Studio)

1. Open `FarshadPM.sln`
2. Configuration: **Release | x64**
3. Build → `bin\\Release\\FarshadPM.exe`

Or use GitHub Actions: push to `master` → download **FarshadPM-exe** artifact.

## Assets

Place branding images in `assets/`:

- `logo_cheats.png` — main / header (CHEATS artwork)
- `logo_aiming.png` — aiming variant
- `icon.ico` — optional window icon

## Default keys

- Activate trigger: **T**
- Toggle bot armed: **F6**
- Toggle fast sniper: **F7**
- Emergency stop: **END**

## Disclaimer

For educational / single-player / offline use only. Using automation in online multiplayer may violate game Terms of Service.
