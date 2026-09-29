# Assets — where everything comes from

Every file in `assets/` is either made for this project or taken from a project that allows it. Keep this file up to date when you add something.

## wallpapers/

| File | Source | License |
|---|---|---|
| `hills-day.svg`, `hills-day-720x1280.png` | Made for Minus (drawn in code: sky gradient, sun, three hills) | same as the project, GPL-3.0 |
| `hills-night.svg`, `hills-night-720x1280.png` | Made for Minus (night sky, stars, moon, dark hills) | same as the project, GPL-3.0 |

The `.svg` is the source, the `.png` is the ready-to-use version at the Galaxy S3 screen size (720×1280).

## icons/minus/

`terminal`, `files`, `video`, `settings`, `keyboard`, `lock`, `power` — made for Minus in the style of the XP-inspired design. Each one as `.svg` (source), `-48.png` and `-96.png`.
License: same as the project, GPL-3.0.

## icons/tango/

From the **Tango Desktop Project** (tango.freedesktop.org), taken from the Debian/Ubuntu package `tango-icon-theme`. The authors released them into the **public domain** — no conditions. Details in `icons/tango/COPYING`.

Each icon as `.svg` (original) and `-48.png`, `-96.png` (rendered for the phone).

## logo/

| File | Source | License |
|---|---|---|
| `minus-logo.svg` + PNG 64/128/512 | Made for Minus — green tile with a white circle and a minus | GPL-3.0 |
| `start-button.svg` + PNG 24/48 | Made for Minus — the glyph on the *minus* start button | GPL-3.0 |

## fonts/

**Noto Sans** (Regular, Bold) and **Noto Sans Mono** (Regular) by the Noto Project Authors.
License: **SIL Open Font License 1.1** — see `fonts/OFL.txt`. It must stay next to the fonts.

These are static versions cut down to Latin + Cyrillic (~36 KB each instead of ~2 MB), so they fit into the small boot partition and still show Russian text.
