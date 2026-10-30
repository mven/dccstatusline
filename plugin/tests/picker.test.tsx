import { expect, mock, test } from 'claude-code/testing'
import type { On } from 'claude-code'

import { parse } from '../hooks/register'

const SKILL = 'dccstatusline:theme'
const BIN = '/home/u/.local/bin/dccstatusline'
const CONFIG = '/home/u/.config/dccstatusline/config'

// Two lines of `dccstatusline --themes`, cut short after the branch.
const THEMES =
  'default     \x1b[0;94m~/src/acme\x1b[0;90m │ \x1b[0;92mmain\x1b[0m\n' +
  'tokyonight  \x1b[0;38;2;122;162;247m~/src/acme\x1b[0;38;2;86;95;137m │ ' +
  '\x1b[0;38;2;158;206;106mmain\x1b[0m\n'

const PANE = {
  plugin: 'dccstatusline',
  component: 'Pane',
  requestId: 'theme-picker',
  surface: 'terminal',
  props: {
    title: 'dccstatusline themes',
    isFocused: true,
    bodyColumns: 120,
    placement: 'inline',
    scroll: { offset: 0, bodyRows: 10 },
    view: {},
  },
} as const

// `/dccstatusline:theme` as the person types it, on a 97-column terminal.
const TYPED = {
  command: SKILL,
  args: '',
  origin: { kind: 'composer' },
  presentation: { isFullscreen: false, columns: 97 },
} as const

const RAN = { exitCode: 0, stderr: '', isStdoutTruncated: false, isStderrTruncated: false }

// Nothing sits beneath the plugin: these hooks stand for the engine. A status
// line that runs `command`, a config that by default names tokyonight alone,
// and `--themes`, whose runs land in the array returned.
function engine(
  on: On,
  { command = '~/.local/bin/dccstatusline', config = '[statusline]\ntheme = tokyonight\n' } = {},
) {
  const argv: string[][] = []

  mock.env(on, { HOME: '/home/u' })
  on('settings.read', () => ({ value: { statusLine: { type: 'command', command } } }))
  on('fs.read', (_, e) => {
    if (e.path !== CONFIG) throw new Error(`${e.path}: no such file`)

    return { value: config }
  })
  on('process.run', (_, e) => {
    argv.push([...e.argv])

    return { value: { ...RAN, stdout: THEMES } }
  })

  return argv
}

// The skill beneath the plugin's hook: each run lands in the array returned.
function skill(on: On) {
  const runs: string[] = []

  on('command.run', (_, e) => {
    runs.push(`/${e.command} ${e.args}`.trim())

    return { text: '' }
  })

  return runs
}

test('the command opens the picker itself, and Enter runs it again with the pick', async ($, on) => {
  const argv = engine(on)
  const runs = skill(on)

  on('ui.open', () => ({ value: { isPlaced: true } }))
  on('ui.close', () => ({ value: undefined }))

  const ran = await $.command.run(TYPED)

  expect(ran.text).toMatch(/Pick a theme/)
  expect(argv).toEqual([[BIN, '--themes', '']])
  expect(runs).toEqual([])

  const ui = await $.ui.mount(PANE)
  const branches = await ui.findAll({ type: 'Text', text: /^main$/ })

  expect(branches.map(t => t.props.color)).toEqual(['greenBright', '#9ece6a'])
  expect(await ui.find({ type: 'Text', text: /in use: tokyonight/ })).toBeDefined()

  await ui.press({ key: 'default' })
  expect(runs).toEqual([`/${SKILL} default`])
  await ui.unmount()
})

test('an option reruns --themes with its INI laid over, and the pick names it', async ($, on) => {
  const argv = engine(on)
  const runs = skill(on)

  on('ui.open', () => ({ value: { isPlaced: true } }))
  on('ui.close', () => ({ value: undefined }))
  await $.command.run(TYPED)

  const ui = await $.ui.mount(PANE)

  expect(await ui.find({ type: 'Text', text: /\son$/ })).toBeDefined()

  await ui.press({ key: 'sep' })
  await ui.press({ key: 'glyphs' })
  expect(argv.at(-1)).toEqual([
    BIN,
    '--themes',
    '[statusline]\nseparator = " ❯ "\n[git]\nformat = {branch}\n' +
      '[plan_short]\nformat = {window} {pct} {resets}\n[plan_long]\nformat = {window} {pct} {resets}\n',
  ])
  expect(await ui.find({ type: 'Text', text: /\soff$/ })).toBeDefined()

  // Round to the first value, which writes nothing.
  await ui.press({ key: 'glyphs' })
  expect(argv.at(-1)).toEqual([BIN, '--themes', '[statusline]\nseparator = " ❯ "\n'])

  await ui.press({ key: 'default' })
  expect(runs).toEqual([`/${SKILL} default sep=❯`])
  await ui.unmount()
})

test('the profile row previews basic, and the pick names it', async ($, on) => {
  const argv = engine(on)
  const runs = skill(on)

  on('ui.open', () => ({ value: { isPlaced: true } }))
  on('ui.close', () => ({ value: undefined }))
  await $.command.run(TYPED)

  const ui = await $.ui.mount(PANE)

  expect(await ui.find({ type: 'Text', text: /\sdetailed$/ })).toBeDefined()

  await ui.press({ key: 'profile' })
  expect(argv.at(-1)).toEqual([BIN, '--themes', '[statusline]\nprofile = basic\n'])
  expect(await ui.find({ type: 'Text', text: /\sbasic$/ })).toBeDefined()

  await ui.press({ key: 'default' })
  expect(runs).toEqual([`/${SKILL} default profile=basic`])
  await ui.unmount()
})

test('the options start where the config is, and one moved back to its first value is named', async ($, on) => {
  const argv = engine(on, {
    config:
      '[statusline]\ntheme = tokyonight\nseparator = " ∷ "\nprofile = basic   # just the percentages\n' +
      '[git]\nformat = {branch}\n',
  })
  const runs = skill(on)

  on('ui.open', () => ({ value: { isPlaced: true } }))
  on('ui.close', () => ({ value: undefined }))
  await $.command.run(TYPED)

  // Glyphs off writes three formats, and the config holds only one.
  expect(argv).toEqual([[BIN, '--themes', '[statusline]\nseparator = " ∷ "\n[statusline]\nprofile = basic\n']])

  const ui = await $.ui.mount(PANE)

  expect(await ui.find({ type: 'Text', text: /\s∷$/ })).toBeDefined()
  expect(await ui.find({ type: 'Text', text: /\son$/ })).toBeDefined()
  expect(await ui.find({ type: 'Text', text: /\sbasic$/ })).toBeDefined()

  // ∷ is the last separator, so the next one is the first.
  await ui.press({ key: 'sep' })
  expect(argv.at(-1)).toEqual([BIN, '--themes', '[statusline]\nprofile = basic\n'])

  await ui.press({ key: 'default' })
  expect(runs).toEqual([`/${SKILL} default sep=│ profile=basic`])
  await ui.unmount()
})

test('a command that names a preset goes straight to the skill', async ($, on) => {
  const runs = skill(on)

  await $.command.run({ ...TYPED, args: 'nord' })
  expect(runs).toEqual([`/${SKILL} nord`])
})

test('a status line that is not dccstatusline leaves the command to the skill', async ($, on) => {
  const argv = engine(on, { command: 'bash ~/bin/line.sh' })
  const runs = skill(on)

  await $.command.run(TYPED)
  expect(argv).toEqual([])
  expect(runs).toEqual([`/${SKILL}`])
})

test('a picker that is not placed closes, and leaves the command to the skill', async ($, on) => {
  engine(on)
  const runs = skill(on)
  let closed = false

  on('ui.open', () => ({ value: { isPlaced: false, reason: 'too narrow' } }))
  on('ui.close', () => {
    closed = true

    return { value: undefined }
  })

  await $.command.run(TYPED)
  expect(closed).toBe(true)
  expect(runs).toEqual([`/${SKILL}`])
})

test('a binary that predates themes lists none', () => {
  expect(parse('\x1b[0;90mdccstatusline v0.0.4 (build #12)\x1b[0m\n')).toEqual([])
})

test('each sequence starts over, in 16 colors or 24-bit, fg and bg', () => {
  expect(parse('x  \x1b[0;91;48;2;255;0;0ma\x1b[0;38;2;128;128;128mb\x1b[0m\n')).toEqual([
    {
      name: 'x',
      gap: '  ',
      spans: [
        { text: 'a', fg: 'redBright', bg: '#ff0000' },
        { text: 'b', fg: '#808080' },
      ],
    },
  ])
})
