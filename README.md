# usic

A minimal local command-line music player written in Rust.

`usic-server` owns playback and state. `usic` and `usic-tui` are clients that send commands to the server over a Unix domain socket.

## Requirements

- Rust stable
- macOS or Linux

## Setup

```bash
cargo build
cargo install --path .
usic server start
```

On first run, usic creates:

```text
~/.config/usic/config.toml
~/.local/state/usic/
```

Edit `config.toml` and set `music_dir` to your local music directory.

## Commands

```bash
usic server start
usic server stop
usic status

usic play [query]
usic pause
usic next
usic prev
usic later [query]
usic seek <MM:SS|+SECONDS|-SECONDS>
usic volume <0..1|up|down|mute>
usic mode <sequence|shuffle|single>

usic tui
```

`play` and `later` use fuzzy matching against the local music library. If `query` is omitted, the CLI prompts for one.

Playlist files are managed from the TUI and stored as plain text files in `music_dir/playlists`, one track per line. Tracks are stored as paths relative to `music_dir`. `All` is a virtual playlist backed by scanning `music_dir`.

## macOS Media Controls

On macOS, Cargo builds a small Swift sidecar named `usic-macos-media` next to `usic-server`. `usic-server` starts it automatically, and the sidecar registers with the system media remote command center. AirPods and system media controls can send play, pause, toggle pause, next, and previous commands to the server. AirPods volume gestures are handled by macOS as output-device volume and are not mirrored into usic's app-level volume setting.

`cargo install --path .` installs the Rust binaries only. For macOS media controls after installation, put `usic-macos-media` next to `usic-server` or somewhere in `PATH`.

## TUI

`usic tui` starts a terminal client. It does not play audio directly; it sends the same IPC commands as the CLI.

Common keys:

- `space`: pause/resume
- `n` / `p`: next/previous
- `j` / `k`: move down/up in the playlist
- `left` / `right`: seek backward/forward
- `+` / `-`: volume up/down
- `m`: mute
- `s`: cycle play mode
- `/`: search tracks; `enter` plays the selected result, `tab` queues it for later
- `l`: queue the selected playlist track for later
- `o`: focus playlists
- `c`: create a playlist from selected tracks
- `a` / `r`: multi-select tracks and add/remove them from a playlist
- In fuzzy/multi-select lists, use `up` / `down`, `ctrl-j` / `ctrl-k`, or `ctrl-n` / `ctrl-p` to move without typing `j`/`k`
- `q`: quit TUI

## Configuration

Example:

```toml
music_dir = "/Users/me/Music"
default_playlist = "All"
default_mode = "sequence"
volume_step = 0.1
seek_step_secs = 10
scan_extensions = ["mp3", "flac", "wav", "ogg", "m4a"]
socket_path = "/Users/me/.local/state/usic/usic.sock"
```
