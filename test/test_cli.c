// dccstatusline — MIT
// test_cli: the real binary, spawned with pipes — the program never forks,
// but its test harness may

#include <errno.h>
#include <signal.h>
#include <spawn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "check.h"

extern char **environ;

static const char *dcc_bin;
static const char *ref_path;

static const char *const names[] =   // every preset, in --themes order
  { "default", "catppuccin", "nord", "gruvbox", "tokyonight",
    "dracula", "onedark", "rosepine", "kanagawa", "solarized" };

static bool run_bin(const char *, const char *, const char *, size_t, char *, size_t,
                    size_t *);
static void expect_line(const char *, const char *, const char *);
static void expect_fallback(const char *, size_t, const char *);
static void test_styled(void);
static void test_cwd_shortened(void);
static void test_failure_modes(void);
static void test_print_config(void);
static void test_reference_is_default(void);
static void test_themes(void);
static void test_themes_overlay(void);

// arg and arg2 are the optional command-line arguments; a NULL ends the list,
// so run_bin(NULL, NULL, ...) runs the bare binary.
static bool
run_bin(const char *arg, const char *arg2, const char *in, size_t inlen,
        char *out, size_t outcap, size_t *outlen)
{
  posix_spawn_file_actions_t fa;
  pid_t pid;
  int inpipe[2], outpipe[2], status;
  char *argv[] = { (char *)dcc_bin, (char *)arg, (char *)arg2, NULL };
  size_t used = 0;

  if(pipe(inpipe) != 0 || pipe(outpipe) != 0) return(false);

  posix_spawn_file_actions_init(&fa);
  posix_spawn_file_actions_adddup2(&fa, inpipe[0], 0);
  posix_spawn_file_actions_adddup2(&fa, outpipe[1], 1);
  posix_spawn_file_actions_addclose(&fa, inpipe[1]);
  posix_spawn_file_actions_addclose(&fa, outpipe[0]);

  if(posix_spawn(&pid, dcc_bin, &fa, NULL, argv, environ) != 0)
  {
    posix_spawn_file_actions_destroy(&fa);

    return(false);
  }

  posix_spawn_file_actions_destroy(&fa);
  close(inpipe[0]);
  close(outpipe[1]);

  {
    size_t done = 0;

    while(done < inlen)
    {
      ssize_t put = write(inpipe[1], in + done, inlen - done);

      if(put < 0)
      {
        if(errno == EINTR) continue;

        break;   // EPIPE when the child stopped reading: fine
      }

      done += (size_t)put;
    }
  }

  close(inpipe[1]);

  for(;;)
  {
    ssize_t got = read(outpipe[0], out + used, outcap - used);

    if(got < 0 && errno == EINTR) continue;

    if(got <= 0) break;

    used += (size_t)got;

    if(used == outcap) break;
  }

  close(outpipe[0]);
  *outlen = used;

  if(waitpid(pid, &status, 0) != pid) return(false);

  return(WIFEXITED(status) && WEXITSTATUS(status) == 0);
}

static void
expect_line(const char *payload, const char *want, const char *what)
{
  char out[8192];
  size_t n = 0;

  CHECK(run_bin(NULL, NULL, payload, strlen(payload), out, sizeof out, &n),
        "%s: exit 0", what);
  CHECK(n == strlen(want) && memcmp(out, want, n) == 0,
        "%s: got \"%.*s\" want \"%s\"", what, (int)n, out, want);
}

static void
expect_fallback(const char *payload, size_t len, const char *what)
{
  static const char prefix[] = "\x1b[0;2mdccstatusline v";
  char out[8192];
  size_t n = 0;

  CHECK(run_bin(NULL, NULL, payload, len, out, sizeof out, &n), "%s: exit 0", what);
  CHECK(n >= sizeof prefix - 1 && memcmp(out, prefix, sizeof prefix - 1) == 0,
        "%s: fallback line (got %zu bytes)", what, n);
  CHECK(n && out[n - 1] == '\n', "%s: newline-terminated", what);
}

static void
test_styled(void)
{
  static const char ini[] =
    "[statusline]\n"
    "sections = context plan_short\n"
    "separator = \" | \"\n"
    "[context]\n"
    "format = {used}/{ceiling} {pct}\n"
    "fg = default\n"
    "used_fg = default\n"
    "pct_fg = default\n"
    "[plan_short]\n"
    "format = {window} {pct}\n"
    "fg = default\n"
    "pct_fg = red\n";
  static const char payload[] =
    "{\"context_window\":{\"total_input_tokens\":50000,"
    "\"context_window_size\":200000},"
    "\"rate_limits\":{\"five_hour\":{\"used_percentage\":23.5}}}";
  FILE *f = fopen("cli_config.ini", "w");

  CHECK(f && fputs(ini, f) >= 0 && fclose(f) == 0, "fixture config written");
  CHECK(setenv("DCCSTATUSLINE_CONFIG", "cli_config.ini", 1) == 0, "env");

  expect_line(payload,
              "\x1b[0m50,000"
              "\x1b[0m/"
              "\x1b[0m200,000"
              "\x1b[0m "
              "\x1b[0m25%"
              "\x1b[0;90m | "
              "\x1b[0m5h"
              "\x1b[0m "
              "\x1b[0;31m24%"
              "\x1b[0m\n",
              "styled end-to-end");
}

// The whole cwd pipeline through the real binary: shrink, then a column cap.
static void
test_cwd_shortened(void)
{
  static const char ini[] =
    "[statusline]\n"
    "sections = cwd\n"
    "[cwd]\n"
    "style   = shrink\n"
    "depth   = 2\n"
    "max_len = 20\n"
    "fg = default\n"
    "path_fg = default\n";
  static const char payload[] =
    "{\"cwd\":\"/mnt/volumes/source/acme/platform/terraform/projects\"}";
  FILE *f = fopen("cli_cwd_config.ini", "w");

  CHECK(f && fputs(ini, f) >= 0 && fclose(f) == 0, "cwd fixture config written");
  CHECK(setenv("DCCSTATUSLINE_CONFIG", "cli_cwd_config.ini", 1) == 0, "env");

  expect_line(payload, "\x1b[0m\xe2\x80\xa6/terraform/projects\x1b[0m\n",
              "cwd shortened end-to-end");
}

static void
test_failure_modes(void)
{
  static char big[100 * 1024];

  expect_fallback("this is not json at all", 23, "garbage stdin");
  expect_fallback("", 0, "empty stdin");
  expect_fallback("{}", 2, "empty object");
  expect_fallback("[1,2,3]", 7, "non-object json");

  memset(big, 'x', sizeof big);
  expect_fallback(big, sizeof big, "oversized stdin");
}

// --print-config is what the plugin's configure skill reads as the dialect, so
// it must be examples/config to the byte, multibyte glyphs included.
static void
test_print_config(void)
{
  static char want[16 * 1024], got[16 * 1024];
  FILE *f = fopen(ref_path, "rb");
  size_t wn = 0, gn = 0;

  CHECK(f && (wn = fread(want, 1, sizeof want, f)) > 0 && feof(f),
        "reference config read whole");

  if(f) fclose(f);

  CHECK(run_bin("--print-config", NULL, "", 0, got, sizeof got, &gn),
        "--print-config: exit 0");
  CHECK(gn == wn && memcmp(got, want, wn) == 0,
        "--print-config: %zu bytes, reference has %zu", gn, wn);
}

// The reference promises it is the compiled-in defaults, and the skill tells
// Claude that defaults need no restating. Hold it to that through the binary.
static void
test_reference_is_default(void)
{
  static const char payload[] =
    "{\"cwd\":\"/nonexistent/acme/platform\","
    "\"model\":{\"id\":\"claude-opus-5-5\",\"display_name\":\"Opus\"},"
    "\"effort\":{\"level\":\"high\"},"
    "\"context_window\":{\"total_input_tokens\":84213,"
    "\"context_window_size\":200000},"
    "\"rate_limits\":{\"five_hour\":{\"used_percentage\":23.5},"
    "\"seven_day\":{\"used_percentage\":41.0}}}";
  char dflt[8192], ref[8192];
  size_t dn = 0, rn = 0;
  FILE *f = fopen("cli_empty.ini", "w");

  CHECK(f && fclose(f) == 0, "empty config written");
  CHECK(setenv("DCCSTATUSLINE_CONFIG", "cli_empty.ini", 1) == 0, "env");
  CHECK(run_bin(NULL, NULL, payload, sizeof payload - 1, dflt, sizeof dflt, &dn),
        "defaults: exit 0");
  CHECK(setenv("DCCSTATUSLINE_CONFIG", ref_path, 1) == 0, "env");
  CHECK(run_bin(NULL, NULL, payload, sizeof payload - 1, ref, sizeof ref, &rn),
        "reference: exit 0");
  CHECK(dn > 0 && dn == rn && memcmp(dflt, ref, dn) == 0,
        "reference renders as the defaults: \"%.*s\" vs \"%.*s\"",
        (int)rn, ref, (int)dn, dflt);
}

// --themes is what the plugin's theme skill reads the preset names from, one
// per line, name first. Its stderr must stay silent: a preset value that fails
// to parse says so there, and would otherwise ship as a quietly kept default.
static void
test_themes(void)
{
  char out[16 * 1024], err[256];
  size_t n = 0, en = 0, i, pos = 0;
  bool quiet = false;
  const int saved = dup(2);
  FILE *const f = fopen("cli_themes.err", "w+");
  const bool captured = saved >= 0 && f && dup2(fileno(f), 2) == 2;
  const bool ran = run_bin("--themes", NULL, "", 0, out, sizeof out, &n);

  if(saved >= 0)   // the child has the file; CHECK's own reports must not
  {
    dup2(saved, 2);
    close(saved);
  }

  CHECK(captured, "stderr captured");
  CHECK(ran, "--themes: exit 0");

  if(f)
  {
    rewind(f);
    en = fread(err, 1, sizeof err, f);
    quiet = !en && !ferror(f);   // a read that failed is not a silent stderr
    fclose(f);
  }

  CHECK(quiet, "--themes: stderr \"%.*s\"", (int)en, err);

  for(i = 0; i < sizeof names / sizeof names[0]; i++)
  {
    const size_t len = strlen(names[i]);
    const char *const eol = memchr(out + pos, '\n', n - pos);

    CHECK(eol && n - pos > len && memcmp(out + pos, names[i], len) == 0
          && out[pos + len] == ' ',
          "--themes: line %zu is %s", i + 1, names[i]);

    if(!eol) return;

    pos = (size_t)(eol - out) + 1;
  }

  CHECK(pos == n, "--themes: one line per preset, nothing after");
}

// An overlay is the plugin picker's preview of its options, laid over every
// preset as keys below a theme line are.
static void
test_themes_overlay(void)
{
  static const char overlay[] =
    "[statusline]\nseparator = \" \xe2\x80\xa2 \"\n"   // " • "
    "[git]\nformat = {branch}\n"                         // glyphs off
    "[plan_short]\nformat = {window} {pct} {resets}\n"
    "[plan_long]\nformat = {window} {pct} {resets}\n";
  static const char bullet[] = " \xe2\x80\xa2 ";
  static const char *const glyph[] = { "\xe2\x8e\x87", "\xe2\x86\xbb" };   // ⎇ ↻
  char out[16 * 1024];
  size_t n = 0, pos = 0, lines = 0;

  CHECK(run_bin("--themes", overlay, "", 0, out, sizeof out, &n),
        "--themes overlay: exit 0");

  while(pos < n)
  {
    const char *const eol = memchr(out + pos, '\n', n - pos);
    const size_t end = eol ? (size_t)(eol - out) : n;
    size_t seps = 0, glyphs = 0, j, g;

    for(j = pos; j < end; j++)
    {
      seps += end - j >= sizeof bullet - 1
              && memcmp(out + j, bullet, sizeof bullet - 1) == 0;

      for(g = 0; g < sizeof glyph / sizeof glyph[0]; g++)
        glyphs += end - j >= 3 && memcmp(out + j, glyph[g], 3) == 0;
    }

    CHECK(seps == 5 && glyphs == 0,
          "--themes overlay: line %zu has %zu bullets, %zu glyphs",
          lines + 1, seps, glyphs);
    lines++;
    pos = end + 1;
  }

  CHECK(lines == sizeof names / sizeof names[0],
        "--themes overlay: %zu lines", lines);
}

int
main(int argc, char **argv)
{
  if(argc != 3)
  {
    fprintf(stderr, "usage: %s <path-to-dccstatusline> <reference-config>\n",
            argv[0]);
    return(1);
  }

  dcc_bin = argv[1];
  ref_path = argv[2];
  signal(SIGPIPE, SIG_IGN);   // the oversized-stdin case writes into a wall

  test_styled();
  test_cwd_shortened();
  test_failure_modes();
  test_print_config();
  test_reference_is_default();
  test_themes();
  test_themes_overlay();

  return(check_failures ? 1 : 0);
}
