#!/bin/sh

command find $USIC_DATABASE \( -name "*.mp3" -o -name "*.flac" -o -name "*.wav" \) > $USIC_PLAYLIST/Default.txt
