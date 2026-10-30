// One run of a sample line in one style, as `--themes` painted it.
export type Span = { text: string; fg?: string; bg?: string }

// gap: the spaces `--themes` put after the name.
export type Preset = { name: string; gap: string; spans: Span[] }

// The picker's options by name: an absent one is at its first value.
export type Choices = Record<string, string>

// held: the options as the config had them when the picker opened.
export type Picker = { bin: string; current: string; held: Choices; choices: Choices; presets: Preset[] }

declare module 'claude-code' {
  interface PluginState {
    dccstatusline: { picker: Shaped<Picker | null> }
  }
}
