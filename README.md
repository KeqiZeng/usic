A lightweight command-line offline music player built with [miniaudio](https://github.com/mackron/miniaudio) and modern C++.

# Features

- Simple and fully customizable command-line interface
- Low resource usage (both CPU and memory)
- Support for multiple music formats (FLAC, MP3, WAV)
- Support common music playback controls
- Playlist management
- Progress bar visualization
- Volume control
- Support multiple play modes - sequence, shuffle, single

# Requirements

`git`

`make`

`camke`

`ninja`

`C++ Compiler` that supports `C++20`

`fzf`(optional)

# Setup

1. Clone this repository `git clone https://github.com/KeqiZeng/usic`.
2. Change directory to `cd usic`.
3. Set the `USIC_LIBRARY` in `src/config.h`.
4. Put audio files (only flac, mp3 and wav are supported) to the `USIC_LIBRARY` directory you set in last step.
5. Run the following commands to compile `usic` and install it to `/usr/local/bin`
```bash
sudo make clean release install
```
6. `usic play` or `usic play "audio_filename.flac"` to play an audio file.

# Commands

- `usic`: Start the server

The following commands require [`fzf`](https://github.com/junegunn/fzf) to be installed: 
- `usic add_music_to_list [playlist_file]`: Fuzzy select a track in `USIC_LABRARY` using fzf and add it to the given playlist.
- `usic fuzzy_play`: Fuzzy select a track in `USIC_LABRARY` using fzf and play it.
- `usic fuzzy_play_later`: Fuzzy select a track in `USIC_LABRARY` using fzf and play it after the current one is over.

The following commands require server is running:

- `usic quit`: Quit the server

## Basic Playback

- `usic play [audio_file]`: Play specified track (or the first one in playlist if not provided)
- `usic pause`: Pause/Resume playback
- `usic play_next`: Play next track
- `usic play_prev`: Play previous track
- `usic play_later <music_file>`: Play a specified track after the current one is over

## Playlist Management

- `usic load <playlist_file>`: Load a playlist
- `usic get_list`: Show the current playlist, the playing track is at the top of the list

## Playback Control

- `usic seek_forward`: Move cursor forward
- `usic seek_backward`: Move cursor backward
- `usic seek_to <MM:SS>`: Seek cursor to specific time
- `usic get_progress`: Show current playback progress

## Volume Control

- `usic volume_up`: Increase volume
- `usic volume_down`: Decrease volume
- `usic set_volume <level>`: Set volume (0-1)
- `usic get_volume`: Show the current volume
- `usic mute`: Toggle mute

## Play Mode

- `usic set_mode`: Set the play mode, the argument should be one of [sequence, shuffle, single]
- `usic get_mode`: Show the current play mode

# Configuration

`usic` is configured by editing `src/config.h`, where more details can be found.

# License

This project is licensed under the [MIT License](https://opensource.org/licenses/MIT).
