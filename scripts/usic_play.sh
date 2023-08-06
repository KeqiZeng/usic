#!/bin/sh

# just a demo
# need a play-list version
usic 1>/dev/null && usic play "$(command find $USIC_PLAYLIST -type f | fzf | xargs -I{} sh -c "command cat "{}" | fzf")"
