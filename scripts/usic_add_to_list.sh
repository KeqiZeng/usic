#!/bin/sh

music="$(command find $USIC_DATABASE \( -name "*.mp3" -o -name "*.flac" -o -name "*.wav" \) | fzf)"
playList="$1"

# Check if the line already exists in the file
if ! command grep -Fxq "$music" "$USIC_PLAYLIST/$playList"; then
    # If the line doesn't exist, append it to the file
    command echo "$music" >> "$USIC_PLAYLIST/$playList"
fi
