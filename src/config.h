#ifndef DCC_CONFIG_H
#define DCC_CONFIG_H

#include <stdbool.h>
#include <stdint.h>

#include "color.h"
#include "schema.h"
#include "util.h"

typedef enum
{
  CWD_ABBREV = 0,
  CWD_FULL,
  CWD_BASENAME,
  CWD_SHRINK,
  CWD_REPO,
} cwd_style_t;

// How {resets} renders. The countdown is time remaining; the clock styles are
// the local instant of the reset itself. The long window prefixes its weekday
// and counts down in days, since "19:00" alone cannot say which of seven.
typedef enum
{
  RESETS_COUNTDOWN = 0,
  RESETS_CLOCK,
  RESETS_CLOCK12,
} resets_style_t;

typedef struct
{
  sv_t format;
  sv_t label;
  color_t fg, bg;                    // base style: literals and un-overridden tokens
  color_t tok_fg[DCC_MAX_TOKENS];    // indexed per dcc_sections token order
  color_t tok_bg[DCC_MAX_TOKENS];
  resets_style_t resets;             // plan sections only
} section_cfg_t;

typedef struct
{
  uint8_t order[SEC_COUNT];
  uint8_t norder;
  sv_t separator;
  color_t sep_fg, sep_bg;
  sv_t thousands;
  cwd_style_t cwd_style;
  uint16_t cwd_depth;      // trailing components kept whole; 0 = off
  uint16_t cwd_max_len;    // codepoint budget for the rendered path; 0 = off
  bool themed;             // a theme key has been applied; the first one wins
  bool basic;              // profile = basic: no token counts or reset times
  section_cfg_t sec[SEC_COUNT];
} config_t;

// Defaults are the shipped theme — identical to examples/config. config_load
// overlays an INI buffer onto them per key: an unparseable value keeps that
// key's default (one stderr note), unknown sections and keys are ignored.
// Views alias the config buffer, which must outlive the render.
void config_defaults(config_t *);
void config_load(config_t *, char *, size_t);

// Overlays a compiled-in preset — colors only — as if its INI text
// sat where the call happens, so later keys still override it. False for an
// unknown name. Views alias one static buffer, hence the first-wins rule.
bool config_theme(config_t *, sv_t);

// The i-th preset's name, in --themes order; NULL past the last.
const char *config_theme_name(size_t);

// $DCCSTATUSLINE_CONFIG, else $XDG_CONFIG_HOME/dccstatusline/config, else
// ~/.config/dccstatusline/config. False when no path can be resolved or fit.
bool config_path(char *, size_t);

#ifdef CONFIG_INTERNAL
#define CUR_NONE   (-2)
#define CUR_GLOBAL (-1)

typedef struct
{
  const char *name;
  const char *ini;
} theme_t;

// Each preset paints the line in three colors of its palette and one gray: the
// first for where you are (cwd and branch), the second for the model, the third
// for every usage figure, and the gray for the separator and all literals.
// Never the separator, a format, the profile, sections, cwd shortening, or
// resets — a theme recolors the line, it does not rewrite or rearrange it.
static const theme_t themes[] =
{
  { "default", "" },
  { "catppuccin",
    "[statusline]\n"
    "separator_fg = #7f849c\n"
    "[cwd]\n"
    "fg = #cba6f7\n"
    "[git]\n"
    "fg = #cba6f7\n"
    "[model]\n"
    "fg = #89b4fa\n"
    "effort_fg = #7f849c\n"
    "[context]\n"
    "fg = #7f849c\n"
    "used_fg = #fab387\n"
    "pct_fg = #fab387\n"
    "[plan_short]\n"
    "fg = #7f849c\n"
    "pct_fg = #fab387\n"
    "[plan_long]\n"
    "fg = #7f849c\n"
    "pct_fg = #fab387\n" },
  { "nord",
    "[statusline]\n"
    "separator_fg = #616e88\n"
    "[cwd]\n"
    "fg = #88c0d0\n"
    "[git]\n"
    "fg = #88c0d0\n"
    "[model]\n"
    "fg = #81a1c1\n"
    "effort_fg = #616e88\n"
    "[context]\n"
    "fg = #616e88\n"
    "used_fg = #5e81ac\n"
    "pct_fg = #5e81ac\n"
    "[plan_short]\n"
    "fg = #616e88\n"
    "pct_fg = #5e81ac\n"
    "[plan_long]\n"
    "fg = #616e88\n"
    "pct_fg = #5e81ac\n" },
  { "gruvbox",
    "[statusline]\n"
    "separator_fg = #928374\n"
    "[cwd]\n"
    "fg = #fabd2f\n"
    "[git]\n"
    "fg = #fabd2f\n"
    "[model]\n"
    "fg = #8ec07c\n"
    "effort_fg = #928374\n"
    "[context]\n"
    "fg = #928374\n"
    "used_fg = #fe8019\n"
    "pct_fg = #fe8019\n"
    "[plan_short]\n"
    "fg = #928374\n"
    "pct_fg = #fe8019\n"
    "[plan_long]\n"
    "fg = #928374\n"
    "pct_fg = #fe8019\n" },
  { "tokyonight",
    "[statusline]\n"
    "separator_fg = #737aa2\n"
    "[cwd]\n"
    "fg = #7aa2f7\n"
    "[git]\n"
    "fg = #7aa2f7\n"
    "[model]\n"
    "fg = #bb9af7\n"
    "effort_fg = #737aa2\n"
    "[context]\n"
    "fg = #737aa2\n"
    "used_fg = #ff9e64\n"
    "pct_fg = #ff9e64\n"
    "[plan_short]\n"
    "fg = #737aa2\n"
    "pct_fg = #ff9e64\n"
    "[plan_long]\n"
    "fg = #737aa2\n"
    "pct_fg = #ff9e64\n" },
  // These take their colors from Ghostty's theme of the same name.
  { "dracula",
    "[statusline]\n"
    "separator_fg = #6272a4\n"
    "[cwd]\n"
    "fg = #bd93f9\n"
    "[git]\n"
    "fg = #bd93f9\n"
    "[model]\n"
    "fg = #ff79c6\n"
    "effort_fg = #6272a4\n"
    "[context]\n"
    "fg = #6272a4\n"
    "used_fg = #8be9fd\n"
    "pct_fg = #8be9fd\n"
    "[plan_short]\n"
    "fg = #6272a4\n"
    "pct_fg = #8be9fd\n"
    "[plan_long]\n"
    "fg = #6272a4\n"
    "pct_fg = #8be9fd\n" },
  { "onedark",
    "[statusline]\n"
    "separator_fg = #767676\n"
    "[cwd]\n"
    "fg = #61afef\n"
    "[git]\n"
    "fg = #61afef\n"
    "[model]\n"
    "fg = #c678dd\n"
    "effort_fg = #767676\n"
    "[context]\n"
    "fg = #767676\n"
    "used_fg = #98c379\n"
    "pct_fg = #98c379\n"
    "[plan_short]\n"
    "fg = #767676\n"
    "pct_fg = #98c379\n"
    "[plan_long]\n"
    "fg = #767676\n"
    "pct_fg = #98c379\n" },
  { "rosepine",
    "[statusline]\n"
    "separator_fg = #6e6a86\n"
    "[cwd]\n"
    "fg = #ebbcba\n"
    "[git]\n"
    "fg = #ebbcba\n"
    "[model]\n"
    "fg = #c4a7e7\n"
    "effort_fg = #6e6a86\n"
    "[context]\n"
    "fg = #6e6a86\n"
    "used_fg = #9ccfd8\n"
    "pct_fg = #9ccfd8\n"
    "[plan_short]\n"
    "fg = #6e6a86\n"
    "pct_fg = #9ccfd8\n"
    "[plan_long]\n"
    "fg = #6e6a86\n"
    "pct_fg = #9ccfd8\n" },
  { "kanagawa",
    "[statusline]\n"
    "separator_fg = #727169\n"
    "[cwd]\n"
    "fg = #7e9cd8\n"
    "[git]\n"
    "fg = #7e9cd8\n"
    "[model]\n"
    "fg = #957fb8\n"
    "effort_fg = #727169\n"
    "[context]\n"
    "fg = #727169\n"
    "used_fg = #e6c384\n"
    "pct_fg = #e6c384\n"
    "[plan_short]\n"
    "fg = #727169\n"
    "pct_fg = #e6c384\n"
    "[plan_long]\n"
    "fg = #727169\n"
    "pct_fg = #e6c384\n" },
  { "solarized",
    "[statusline]\n"
    "separator_fg = #586e75\n"
    "[cwd]\n"
    "fg = #268bd2\n"
    "[git]\n"
    "fg = #268bd2\n"
    "[model]\n"
    "fg = #2aa198\n"
    "effort_fg = #586e75\n"
    "[context]\n"
    "fg = #586e75\n"
    "used_fg = #b58900\n"
    "pct_fg = #b58900\n"
    "[plan_short]\n"
    "fg = #586e75\n"
    "pct_fg = #b58900\n"
    "[plan_long]\n"
    "fg = #586e75\n"
    "pct_fg = #b58900\n" },
};

static void handle_line(config_t *, int *, char *, size_t);
static sv_t parse_value(char *, size_t);
static sv_t trim(sv_t);
static void set_global(config_t *, sv_t, sv_t);
static void set_section(config_t *, int, sv_t, sv_t);
static void set_resets(section_cfg_t *, sv_t);
static void set_theme(config_t *, sv_t);
static void set_profile(config_t *, sv_t);
static bool parse_u16(sv_t, uint16_t *);
static bool token_color(section_cfg_t *, const section_desc_t *, sv_t, sv_t);
static void set_color(color_t *, sv_t, sv_t);
static void parse_order(config_t *, sv_t);
static int token_index(const section_desc_t *, sv_t);
static color_t named16(uint8_t);
static void set_tok_fg(config_t *, section_id_t, const char *, color_t);
#endif

#endif
