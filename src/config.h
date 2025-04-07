#pragma once
#include <string_view>
#include <unordered_map>
#include <vector>

namespace Config
{

/* =========== usic library ========== */
// The path to the usic library, where music files are stored
constexpr std::string_view USIC_LIBRARY = "/Users/ketch/Music/usic";

// The directory in usic library to store playlist files
constexpr std::string_view PLAYLISTS_DIR = "playlists/";

// The default play list, if none is specified, all the music in the usic library will be loaded
constexpr std::string_view DEFAULT_PLAYLIST;

/* =========== playmode ========== */
// 0 - sequence
// 1 - shuffle
// 2 - single
constexpr int DEFAULT_PLAY_MODE = 0;

/* ====== number of cached wav files ====== */
constexpr int NUM_CACHED_WAV_FILES = 10;

/* =========== move cursor ========== */
// The step length in seconds for seek_forward and seek_backward
constexpr int STEP_SECONDS = 10;

/* =========== volume ========== */
// The step size for volume up and down
constexpr float VOLUME_STEP = 0.1F;

/* =========== progress bar ========== */
// The length of the progress bar in characters
constexpr int PROGRESS_BAR_LENGTH = 50;

// The character used for the progress bar
constexpr char BAR_ELEMENT = '-';

// The character used for the cursor
constexpr char CURSOR_ELEMENT = 'o';

/* =========== commands ========== */
constexpr std::string_view LOAD              = "load";
constexpr std::string_view PLAY              = "play";
constexpr std::string_view PLAY_LATER        = "play_later";
constexpr std::string_view PLAY_NEXT         = "play_next";
constexpr std::string_view PLAY_PREV         = "play_prev";
constexpr std::string_view PAUSE             = "pause";
constexpr std::string_view SEEK_FORWARD      = "seek_forward";
constexpr std::string_view SEEK_BACKWARD     = "seek_backward";
constexpr std::string_view SEEK_TO           = "seek_to";
constexpr std::string_view GET_PROGRESS      = "get_progress";
constexpr std::string_view VOLUME_UP         = "volume_up";
constexpr std::string_view VOLUME_DOWN       = "volume_down";
constexpr std::string_view SET_VOLUME        = "set_volume";
constexpr std::string_view GET_VOLUME        = "get_volume";
constexpr std::string_view MUTE              = "mute";
constexpr std::string_view SET_MODE          = "set_mode";
constexpr std::string_view GET_MODE          = "get_mode";
constexpr std::string_view GET_LIST          = "get_list";
constexpr std::string_view QUIT              = "quit";
constexpr std::string_view PRINT_VERSION     = "--version";
constexpr std::string_view PRINT_HELP        = "--help";
constexpr std::string_view ADD_MUSIC_TO_LIST = "add_music_to_list";
constexpr std::string_view FUZZY_PLAY        = "fuzzy_play";
constexpr std::string_view FUZZY_PLAY_LATER  = "fuzzy_play_later";

/* =========== command aliases ========== */
const std::unordered_map<std::string_view, std::vector<std::string_view>> COMMANDS = {
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

} // namespace Config