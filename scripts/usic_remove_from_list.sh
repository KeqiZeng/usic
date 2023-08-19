#!/bin/sh

playList=$1
music="$(command cat "$USIC_PLAYLIST/$playList" | fzf)"

if [ "$(command wc -l < "$USIC_PLAYLIST/$playList")" -eq 1 ] && [ "$(command cat "$USIC_PLAYLIST/$playList")" = "$music" ]; then
	# clear the file
	: > "$USIC_PLAYLIST/$playList"
else
	command grep -vFx "$music" "$USIC_PLAYLIST/$playList" > "$USIC_PLAYLIST/$playList.tmp" && mv "$USIC_PLAYLIST/$playList.tmp" "$USIC_PLAYLIST/$playList"
fi
