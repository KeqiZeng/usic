#pragma once
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "runtime.h"

/* usic library */
const std::string_view USIC_LIBRARY    = "/Users/ketch/Music/usic";
const std::string_view PLAY_LISTS_PATH = "playLists/";
const std::string_view DEFAULT_PLAY_LIST;

/* playlist */
// const bool RANDOM      = true;
// const bool SINGLE_LOOP = false;

/* playmode */
const PlayMode DEFAULT_PLAY_MODE = PlayMode::SHUFFLE;

/* move cursor */
const int STEP_SECONDS = 10;

/* volume */
const float VOLUME_STEP = 0.1F;

/* progress bar */
const int PROGRESS_BAR_LENGTH = 50;
const char BAR_ELEMENT        = '-';
const char CURSOR_ELEMENT     = 'o';

/* commands */
const std::string LOAD              = "load";
const std::string PLAY              = "play";
const std::string PLAY_LATER        = "play_later";
const std::string PLAY_NEXT         = "play_next";
const std::string PLAY_PREV         = "play_prev";
const std::string PAUSE             = "pause";
const std::string SEEK_FORWARD      = "seek_forward";
const std::string SEEK_BACKWARD     = "seek_backward";
const std::string SEEK_TO           = "seek_to";
const std::string GET_PROGRESS      = "get_progress";
const std::string VOLUME_UP         = "volume_up";
const std::string VOLUME_DOWN       = "volume_down";
const std::string SET_VOLUME        = "set_volume";
const std::string GET_VOLUME        = "get_volume";
const std::string MUTE              = "mute";
const std::string SET_MODE          = "set_mode";
const std::string GET_MODE          = "get_mode";
const std::string GET_LIST          = "get_list";
const std::string QUIT              = "quit";
const std::string PRINT_VERSION     = "--version";
const std::string PRINT_HELP        = "--help";
const std::string ADD_MUSIC_TO_LIST = "add_music_to_list";
const std::string FUZZY_PLAY        = "fuzzy_play";
const std::string FUZZY_PLAY_LATER  = "fuzzy_play_later";

const std::unordered_map<std::string, std::vector<std::string>> COMMANDS = {
    {LOAD, {"l"}},
    {PLAY, {"p"}},
    {PLAY_LATER, {"pl", "later"}},
    {PLAY_NEXT, {"pn", "next"}},
    {PLAY_PREV, {"pp", "prev"}},
    {PAUSE, {"ps"}},
    {SEEK_FORWARD, {"sf", "forward"}},
    {SEEK_BACKWARD, {"sb", "backward"}},
    {SEEK_TO, {"to", "seek"}},
    {GET_PROGRESS, {"gp", "progress"}},
    {VOLUME_UP, {"vu"}},
    {VOLUME_DOWN, {"vd"}},
    {SET_VOLUME, {"sv", "setv"}},
    {GET_VOLUME, {"gv", "getv"}},
    {MUTE, {"m"}},
    {SET_MODE, {"sm", "setm"}},
    {GET_MODE, {"gm", "getm"}},
    {GET_LIST, {"list", "ls"}},
    {QUIT, {"q"}},
    {PRINT_VERSION, {"-v"}},
    {PRINT_HELP, {"-h"}},
    {ADD_MUSIC_TO_LIST, {"aml"}},
    {FUZZY_PLAY, {"fp"}},
    {FUZZY_PLAY_LATER, {"fpl"}}
};