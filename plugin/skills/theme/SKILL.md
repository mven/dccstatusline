---
name: theme
description: Show dccstatusline's preset themes in color and pick one with the arrow keys. Use when the user wants a theme or color scheme for dccstatusline, names a preset (catppuccin, nord, gruvbox, tokyonight, dracula, onedark, rosepine, kanagawa, solarized), or asks what the themes look like.
---

# Pick a dccstatusline theme

A theme is a compiled-in preset of colors, selected with one `theme = <name>`
line. dccstatusline re-reads its config on every refresh, so the preset the
user picks shows up on the real status line as soon as the turn ends.

When the user types `/dccstatusline:theme` on its own, the plugin opens its
picker before this skill runs: every preset drawn in its own colors in a
panel, chosen with the arrow keys. Below the presets, the separator row
chooses the separator (`│` by default), the glyphs row turns off the `⎇`
before the branch and the `↻` before both reset times, and the profile row
switches from detailed to basic, which leaves out the token counts and the
reset times. Each row starts at what the config already sets, and the presets
are drawn with the rows and redraw as they change. The pick comes back as
`/dccstatusline:theme <name> [sep=<glyph>] [glyphs=on|off] [profile=detailed|basic]`:
a preset, then each option the user set or the config already set. Esc closes
the picker with nothing picked.

So you run with no preset named only when there was no picker: the user asked
in their own words, Claude Code has no mods, or the picker couldn't open.
Then the preview is the colored `--themes` output in the transcript, and a
menu picks: a menu can't show color, and the status line hides while it's
open.

## 0. Find the binary and the config

Follow steps 0 and 1 of `${CLAUDE_PLUGIN_ROOT}/skills/configure/SKILL.md`.
They give you `BIN` and the config path, or a reason to stop.

If the user already named a preset, skip to step 2.

## 1. Show the presets and ask

Run `"BIN" --themes </dev/null`. It prints one sample line per preset, side by
side in real color, and that output is what the user compares. If it's
collapsed, ctrl+o expands it. The first word of each line is the preset's name.

If the output is a single dimmed line reading `dccstatusline v… (build #…)`
instead, the binary predates themes and ignored the flag. Stop and offer an
update: `/dccstatusline:setup` if `BIN` is `${CLAUDE_PLUGIN_DATA}/dccstatusline`,
otherwise the binary they installed by hand.

Then ask with one AskUserQuestion call. Ask nothing about glyphs or the
profile, but a request in the user's own words, such as "nord without the
glyphs" or "nord, basic", maps to the same options as the picker's. If you can't show a menu, list the
names and ask in one line instead. The options are the presets other than the
one in use, in `--themes` order, first four. Describe each by its palette in a
few words. A menu holds only four, so name the rest in the question text:
Other lets the user type any name. Add no previews: the pane can't show color.

Put the step 2 question about keys in the same call when it applies. If the
user dismisses the menu, stop. Nothing has been written.

## 2. Apply

**Keys in the way.** The theme applies where its line sits, and every key after
it wins. If the config holds keys that paint over a preset (`separator`,
`profile`, `format`, or any `fg`, `bg`, `*_fg`, or `*_bg`), ask whether to
remove them.
Leave out the keys the options below write: the pick already chose those.
Name them, and offer "Remove them (Recommended)" and "Keep them". A pasted
`--print-config` restates all of them, which is the usual reason a theme seems
to do nothing. Ask once a session. If the user dismisses the question, stop.

Before the first write, copy any existing config to `<config>.bak`, and note
whether there was no config. Later picks in the same session don't refresh the
backup, so undo always returns to where the user started.

Remove the keys if the user chose to. Then write the theme as the first key
the config reads:

- If the first section header is `[statusline]`, put `theme = <name>` on the
  line right after it.
- Otherwise insert `[statusline]` and `theme = <name>` right before the first
  section header, or at the end of a file with no header. Create the file and
  its directory if needed.

If a `theme` key already exists, change that line instead. Never leave a
second `theme` key: only the first one counts.

Then write each option the pick names, changing the key if the config has it:

- `sep=<glyph>`: `separator = " <glyph> "` on the line right after `theme =`.
- `profile=basic`: `profile = basic` on the line right after `theme =`, or
  after the `separator` line if there is one.
- `glyphs=off`: `format = {branch}` in `[git]`, and
  `format = {window} {pct} {resets}` in `[plan_short]` and `[plan_long]`. Add
  any of those sections at the end if the config has none.

`sep=│`, `profile=detailed` and `glyphs=on` are the defaults, named when the
config set something else: remove the lines the other value writes instead.

## 3. End the turn

End with one short line, such as "nord is on your status line now. Say
another to pick again, or undo to restore your previous config." The status
line redraws once the turn ends.

## 4. Respond

- **another**: ask the step 1 question again, running `--themes` first if its
  output has scrolled out of view. If the picker made the last pick, say
  instead that `/dccstatusline:theme` opens it again.
- **undo**: restore `<config>.bak` over the config, or delete the config if
  there was none before step 2.
- **Anything else**: the pick stands. If step 2 made a `.bak`, tell them it
  holds their previous config. Mention that `/dccstatusline:configure` can
  tune single colors on top of the theme.
