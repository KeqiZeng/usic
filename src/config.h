#pragma once
#include <string>
#include <unordered_map>
#include <vector>

/* usic library */
const std::string USIC_LIBARY = "/Users/ketch/Music/usic";
const std::string PLAY_LISTS_PATH = "playLists/";
const std::string DEFAULT_PLAY_LIST = "Default.txt";

/* playlist */
#define REPETITIVE false
#define RANDOM true
#define RELOAD true

/* move cursor */
#define STEP_SECONDS 10

/* volume */
#define VOLUME_STEP 0.1

/* progress bar */
#define PROGRESS_BAR_LENGTH 100
#define BAR_ELEMENT '-'
#define CURSOR_ELEMENT 'o'

/* commands */
#define LOAD "load"
#define PLAY "play"
#define PLAY_LATER "play_later"
#define PLAY_NEXT "play_next"
#define PLAY_PREV "play_prev"
#define PAUSE "pause"
#define CURSOR_FORWARD "cursor_forward"
#define CURSOR_BACKWARD "cursor_backward"
#define SET_CURSOR "set_cursor"
#define GET_CURRENT_PROGRESS "get_current_progress"
#define VOLUME_UP "volume_up"
#define VOLUME_DOWN "volume_down"
#define SET_VOLUME "set_volume"
#define GET_VOLUME "get_volume"
#define MUTE "mute"
#define SET_RANDOM "set_random"
#define SET_REPETITIVE "set_repetitive"
#define LIST "list"
#define QUIT "quit"

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
    {LIST, {"ls"}},
    {QUIT, {"q"}}};