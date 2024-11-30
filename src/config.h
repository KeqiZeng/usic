#pragma once
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

/* usic library */
const std::string_view USIC_LIBARY       = "/Users/ketch/Music/usic";
const std::string_view PLAY_LISTS_PATH   = "playLists/";
const std::string_view DEFAULT_PLAY_LIST = "Default.txt";

/* playlist */
const bool REPETITIVE = false;
const bool RANDOM     = true;

/* move cursor */
const int STEP_SECONDS = 10;

/* volume */
const float VOLUME_STEP = 0.1F;

/* progress bar */
const int PROGRESS_BAR_LENGTH = 50;
const char BAR_ELEMENT        = '-';
const char CURSOR_ELEMENT     = 'o';

/* commands */
const std::string LOAD                 = "load";
const std::string PLAY                 = "play";
const std::string PLAY_LATER           = "play_later";
const std::string PLAY_NEXT            = "play_next";
const std::string PLAY_PREV            = "play_prev";
const std::string PAUSE                = "pause";
const std::string CURSOR_FORWARD       = "cursor_forward";
const std::string CURSOR_BACKWARD      = "cursor_backward";
const std::string SET_CURSOR           = "set_cursor";
const std::string GET_CURRENT_PROGRESS = "get_current_progress";
const std::string VOLUME_UP            = "volume_up";
const std::string VOLUME_DOWN          = "volume_down";
const std::string SET_VOLUME           = "set_volume";
const std::string GET_VOLUME           = "get_volume";
const std::string MUTE                 = "mute";
const std::string SET_RANDOM           = "set_random";
const std::string SET_REPETITIVE       = "set_repetitive";
const std::string GET_LIST             = "get_list";
const std::string QUIT                 = "quit";

const std::unordered_map<std::string, std::vector<std::string>> COMMANDS = {
    {LOAD, {"l"}},
    {PLAY, {"p"}},
    {PLAY_LATER, {"pl", "later"}},
    {PLAY_NEXT, {"pn", "next"}},
    {PLAY_PREV, {"pp", "prev"}},
    {PAUSE, {"ps"}},
    {CURSOR_FORWARD, {"cf", "forward"}},
    {CURSOR_BACKWARD, {"cb", "backward"}},
    {SET_CURSOR, {"sc", "cursor"}},
    {GET_CURRENT_PROGRESS, {"progress"}},
    {VOLUME_UP, {"vu"}},
    {VOLUME_DOWN, {"vd"}},
    {SET_VOLUME, {"sv", "setv"}},
    {GET_VOLUME, {"gv", "getv"}},
    {MUTE, {"m"}},
    {SET_RANDOM, {"sr", "random"}},
    {SET_REPETITIVE, {"sp", "repetitive"}},
    {GET_LIST, {"list", "ls"}},
    {QUIT, {"q"}}
};