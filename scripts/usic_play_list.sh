#!/bin/sh

playList="$(command find "$USIC_PLAYLIST" -type f | fzf)"
music="$(command cat "$playList" | fzf)"

usic 1>/dev/null && usic play-list "$playList" "$music"
