#!/bin/sh

playList="$(command find $USIC_PLAYLIST -type f | fzf)"
music="$(command cat $playList | fzf)"

echo "usic play-list $playList $music"
