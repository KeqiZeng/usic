#include "server.h"
#include "commands.h"
#include "config.h"
#include "core.h"
#include "fmt/core.h"
#include "music_list.h"
#include "runtime.h"
#include "utils.h"

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <memory>
#include <string>
#include <thread>
#include <vector>

void initMusicList(MusicList* music_list, Config* config)
{
    if (DEFAULT_PLAY_LIST.empty()) {
        const std::string DEFAULT_LIST = utils::createTmpDefaultList();
        music_list->load(DEFAULT_LIST);
        if (music_list->isEmpty()) {
            LOG(fmt::format("failed to load default music list while user defined default music list is empty"),
                LogType::ERROR);
            throw std::runtime_error("failed to load default music list");
        }
        utils::deleteTmpFiles();
    }
    else {
        music_list->load(fmt::format("{}{}", config->getPlayListPath(), DEFAULT_PLAY_LIST));
        if (music_list->isEmpty()) {
            LOG(fmt::format("failed to load music list {}{}", config->getPlayListPath(), DEFAULT_PLAY_LIST),
                LogType::ERROR);
            throw std::runtime_error("failed to load default music list");
        }
    }
}

std::string getCommand(NamedPipe* pipe_to_server, NamedPipe* pipe_to_client)
{
    pipe_to_server->openPipe(OpenMode::RD_ONLY_BLOCK);
    pipe_to_client->openPipe(OpenMode::WR_ONLY_BLOCK);

    std::string cmd = pipe_to_server->readOut();
    pipe_to_server->closePipe();
    pipe_to_client->closePipe();
    return cmd;
}

void sendMsgToClient(std::string_view msg, NamedPipe* pipe_to_server, NamedPipe* pipe_to_client)
{
    pipe_to_server->openPipe(OpenMode::RD_ONLY_BLOCK);
    pipe_to_client->openPipe(OpenMode::WR_ONLY_BLOCK);
    pipe_to_client->writeIn(msg);
    if (pipe_to_server->readOut() == GOT) { pipe_to_client->writeIn(OVER); }
    pipe_to_server->closePipe();
    pipe_to_client->closePipe();
}

void sendMsgToClient(const std::vector<std::string>* msg, NamedPipe* pipe_to_server, NamedPipe* pipe_to_client)
{
    if (msg->size() == 0) {
        LOG("got an empty message", LogType::ERROR);
        return;
    }
    pipe_to_server->openPipe(OpenMode::RD_ONLY_BLOCK);
    pipe_to_client->openPipe(OpenMode::WR_ONLY_BLOCK);
    pipe_to_client->writeIn(msg->at(0));
    std::string response = pipe_to_server->readOut();
    if (response == GOT) {
        for (size_t i = 1; i < msg->size(); i++) {
            pipe_to_client->writeIn(msg->at(i));
            response = pipe_to_server->readOut();
        }
    }
    pipe_to_client->writeIn(OVER);
    pipe_to_server->closePipe();
    pipe_to_client->closePipe();
}

/* Split command into individual arguments, by space but skip the space inside
 * quotes */
static std::vector<std::string> parseCommand(std::string& cmd)
{
    std::vector<std::string> args;
    std::string current;
    bool is_in_quotes = false;

    for (char c : cmd) {
        if (c == '\"') {
            // skip space inside quotes
            is_in_quotes = !is_in_quotes;
        }
        else if (c == ' ' && !is_in_quotes) {
            if (!current.empty()) {
                args.push_back(current);
                current.clear();
            }
        }
        else {
            current += c;
        }
    }

    if (!current.empty()) { args.push_back(current); }

    return args;
}

static void logErrorAndSendToClient(std::string_view err, NamedPipe* pipe_to_server, NamedPipe* pipe_to_client)
{
    LOG(err, LogType::ERROR);
    sendMsgToClient(err, pipe_to_server, pipe_to_client);
}

void handleCommand(
    std::string& cmd,
    NamedPipe* pipe_to_server,
    NamedPipe* pipe_to_client,
    MaComponents* ma_comp,
    std::string* music_playing,
    MusicList* music_list,
    Controller* controller,
    Config* config
)
{
    std::vector<std::string> args = parseCommand(cmd);
    const std::string& sub_cmd    = args.at(0);
    if (sub_cmd == LOAD || std::ranges::find(COMMANDS.at(LOAD), sub_cmd) != COMMANDS.at(LOAD).end()) {
        if (args.size() >= 2) {
            commands::load(args.at(1), music_list, config);
            sendMsgToClient(NO_MESSAGE, pipe_to_server, pipe_to_client);
        }
        else {
            logErrorAndSendToClient(fmt::format("{}: not enough arguments.", LOAD), pipe_to_server, pipe_to_client);
        }
    }
    else if (sub_cmd == PLAY || std::ranges::find(COMMANDS.at(PLAY), sub_cmd) != COMMANDS.at(PLAY).end()) {
        if (args.size() >= 2) {
            const std::string& music_to_play = args.at(1);
            ma_result result = commands::play(ma_comp, music_to_play, music_playing, music_list, controller, config);
            if (result != MA_SUCCESS) {
                logErrorAndSendToClient(
                    fmt::format("Failed to play music: {}", music_to_play),
                    pipe_to_server,
                    pipe_to_client
                );
            }
            else {
                sendMsgToClient(NO_MESSAGE, pipe_to_server, pipe_to_client);
            }
        }
        else {
            const std::string& music_to_play = music_list->headOut()->getMusic();
            ma_result result = commands::play(ma_comp, music_to_play, music_playing, music_list, controller, config);
            if (result != MA_SUCCESS) {
                logErrorAndSendToClient(
                    fmt::format("Failed to play music: {}", music_to_play),
                    pipe_to_server,
                    pipe_to_client
                );
            }
            else {
                sendMsgToClient(NO_MESSAGE, pipe_to_server, pipe_to_client);
            }
        }
    }
    else if (sub_cmd == PLAY_LATER ||
             std::ranges::find(COMMANDS.at(PLAY_LATER), sub_cmd) != COMMANDS.at(PLAY_LATER).end()) {
        if (args.size() >= 2) {
            const std::string& music_to_play = args.at(1);
            commands::playLater(config, music_to_play, music_list);
            sendMsgToClient(NO_MESSAGE, pipe_to_server, pipe_to_client);
        }
        else {
            logErrorAndSendToClient(
                fmt::format("{}: not enough arguments.", PLAY_LATER),
                pipe_to_server,
                pipe_to_client
            );
        }
    }
    else if (sub_cmd == PLAY_NEXT ||
             std::ranges::find(COMMANDS.at(PLAY_NEXT), sub_cmd) != COMMANDS.at(PLAY_NEXT).end()) {
        ma_result result = commands::playNext(ma_comp);
        if (result != MA_SUCCESS) {
            logErrorAndSendToClient("Failed to play next music", pipe_to_server, pipe_to_client);
        }
        else {
            sendMsgToClient(NO_MESSAGE, pipe_to_server, pipe_to_client);
        }
    }
    else if (sub_cmd == PLAY_PREV ||
             std::ranges::find(COMMANDS.at(PLAY_PREV), sub_cmd) != COMMANDS.at(PLAY_PREV).end()) {
        ma_result result = commands::playPrev(ma_comp, music_list);
        if (result != MA_SUCCESS) {
            logErrorAndSendToClient("Failed to play prev music", pipe_to_server, pipe_to_client);
        }
        else {
            sendMsgToClient(NO_MESSAGE, pipe_to_server, pipe_to_client);
        }
    }
    else if (sub_cmd == PAUSE || std::ranges::find(COMMANDS.at(PAUSE), sub_cmd) != COMMANDS.at(PAUSE).end()) {
        ma_result result = commands::pause(ma_comp);
        if (result != MA_SUCCESS) {
            logErrorAndSendToClient("Failed to pause/resume music", pipe_to_server, pipe_to_client);
        }
        else {
            sendMsgToClient(NO_MESSAGE, pipe_to_server, pipe_to_client);
        }
    }
    else if (sub_cmd == CURSOR_FORWARD ||
             std::ranges::find(COMMANDS.at(CURSOR_FORWARD), sub_cmd) != COMMANDS.at(CURSOR_FORWARD).end()) {
        ma_result result = commands::cursorForward(ma_comp);
        if (result != MA_SUCCESS) {
            logErrorAndSendToClient("Failed to move cursor forward", pipe_to_server, pipe_to_client);
        }
        else {
            sendMsgToClient(NO_MESSAGE, pipe_to_server, pipe_to_client);
        }
    }
    else if (sub_cmd == CURSOR_BACKWARD ||
             std::ranges::find(COMMANDS.at(CURSOR_BACKWARD), sub_cmd) != COMMANDS.at(CURSOR_BACKWARD).end()) {
        ma_result result = commands::cursorBackward(ma_comp);
        if (result != MA_SUCCESS) {
            logErrorAndSendToClient("Failed to move cursor backward", pipe_to_server, pipe_to_client);
        }
        else {
            sendMsgToClient(NO_MESSAGE, pipe_to_server, pipe_to_client);
        }
    }
    else if (sub_cmd == SET_CURSOR ||
             std::ranges::find(COMMANDS.at(SET_CURSOR), sub_cmd) != COMMANDS.at(SET_CURSOR).end()) {
        if (args.size() >= 2) {
            const std::string& time = args.at(1);
            ma_result result        = commands::setCursor(ma_comp, time);
            if (result != MA_SUCCESS) {
                logErrorAndSendToClient("Failed to set cursor", pipe_to_server, pipe_to_client);
            }
            else {
                sendMsgToClient(NO_MESSAGE, pipe_to_server, pipe_to_client);
            }
        }
    }
    else if (sub_cmd == GET_PROGRESS ||
             std::ranges::find(COMMANDS.at(GET_PROGRESS), sub_cmd) != COMMANDS.at(GET_PROGRESS).end()) {
        auto progress    = std::make_unique<Progress>();
        ma_result result = commands::getProgress(ma_comp, *music_playing, progress.get());
        if (result != MA_SUCCESS) { logErrorAndSendToClient("Failed to get progress", pipe_to_server, pipe_to_client); }
        else {
            sendMsgToClient(progress->makeBar(), pipe_to_server, pipe_to_client);
        }
    }
    else if (sub_cmd == VOLUME_UP ||
             std::ranges::find(COMMANDS.at(VOLUME_UP), sub_cmd) != COMMANDS.at(VOLUME_UP).end()) {
        ma_result result = commands::volumeUp(ma_comp->engine_.get());
        if (result != MA_SUCCESS) {
            logErrorAndSendToClient("Failed to turn the volume up", pipe_to_server, pipe_to_client);
        }
        else {
            sendMsgToClient(NO_MESSAGE, pipe_to_server, pipe_to_client);
        }
    }
    else if (sub_cmd == VOLUME_DOWN ||
             std::ranges::find(COMMANDS.at(VOLUME_DOWN), sub_cmd) != COMMANDS.at(VOLUME_DOWN).end()) {
        ma_result result = commands::volumeDown(ma_comp->engine_.get());
        if (result != MA_SUCCESS) {
            logErrorAndSendToClient("Failed to turn the volume down", pipe_to_server, pipe_to_client);
        }
        else {
            sendMsgToClient(NO_MESSAGE, pipe_to_server, pipe_to_client);
        }
    }
    else if (sub_cmd == SET_VOLUME ||
             std::ranges::find(COMMANDS.at(SET_VOLUME), sub_cmd) != COMMANDS.at(SET_VOLUME).end()) {
        if (args.size() >= 2) {
            const std::string& volume = args.at(1);
            ma_result result          = commands::setVolume(ma_comp->engine_.get(), volume);
            if (result != MA_SUCCESS) {
                logErrorAndSendToClient("Failed to set volume", pipe_to_server, pipe_to_client);
            }
            else {
                sendMsgToClient(NO_MESSAGE, pipe_to_server, pipe_to_client);
            }
        }
        else {
            logErrorAndSendToClient(
                fmt::format("{}, not enough arguments.", SET_VOLUME),
                pipe_to_server,
                pipe_to_client
            );
        }
    }
    else if (sub_cmd == GET_VOLUME ||
             std::ranges::find(COMMANDS.at(GET_VOLUME), sub_cmd) != COMMANDS.at(GET_VOLUME).end()) {
        float volume     = -1;
        ma_result result = commands::getVolume(ma_comp->engine_.get(), &volume);
        if (result != MA_SUCCESS) { logErrorAndSendToClient("Failed to get volume", pipe_to_server, pipe_to_client); }
        else {
            sendMsgToClient(fmt::format("Volume: {}", volume), pipe_to_server, pipe_to_client);
        }
    }
    else if (sub_cmd == MUTE || std::ranges::find(COMMANDS.at(MUTE), sub_cmd) != COMMANDS.at(MUTE).end()) {
        ma_result result = commands::mute(ma_comp->engine_.get());
        if (result != MA_SUCCESS) { logErrorAndSendToClient("Failed to (un)mute", pipe_to_server, pipe_to_client); }
        else {
            sendMsgToClient(NO_MESSAGE, pipe_to_server, pipe_to_client);
        }
    }
    else if (sub_cmd == SET_RANDOM ||
             std::ranges::find(COMMANDS.at(SET_RANDOM), sub_cmd) != COMMANDS.at(SET_RANDOM).end()) {
        commands::setRandom(config);
        sendMsgToClient(
            fmt::format("random: {}", config->isRandom() ? "enabled" : "disabled"),
            pipe_to_server,
            pipe_to_client
        );
    }
    else if (sub_cmd == SET_REPETITIVE ||
             std::ranges::find(COMMANDS.at(SET_REPETITIVE), sub_cmd) != COMMANDS.at(SET_REPETITIVE).end()) {
        commands::setRepetitive(config);
        sendMsgToClient(
            fmt::format("repetitive: {}", config->isRepetitive() ? "enabled" : "disabled"),
            pipe_to_server,
            pipe_to_client
        );
    }
    else if (sub_cmd == GET_LIST || std::ranges::find(COMMANDS.at(GET_LIST), sub_cmd) != COMMANDS.at(GET_LIST).end()) {
        std::vector<std::string> list = commands::getList(music_list);
        sendMsgToClient(&list, pipe_to_server, pipe_to_client);
    }
    else if (sub_cmd == QUIT || std::ranges::find(COMMANDS.at(QUIT), sub_cmd) != COMMANDS.at(QUIT).end()) {
        commands::quit(ma_comp, controller);
        sendMsgToClient("Server quit successfully", pipe_to_server, pipe_to_client);
    }
    else {
        logErrorAndSendToClient(fmt::format("unknown command: {}", sub_cmd), pipe_to_server, pipe_to_client);
    }
}

void server()
{
    // setup runtime
    auto pipe_to_server = std::make_unique<NamedPipe>(PIPE_TO_SERVER);
    auto pipe_to_client = std::make_unique<NamedPipe>(PIPE_TO_CLIENT);
    setupRuntime(pipe_to_server.get(), pipe_to_client.get()); // throw fatal error

    auto config  = std::make_unique<Config>(REPETITIVE, RANDOM, USIC_LIBRARY, PLAY_LISTS_PATH); // throw fatal error
    auto ma_comp = std::make_unique<MaComponents>();
    ma_comp->maCompInitEngine(); // throw fatal error

    auto music_list = std::make_unique<MusicList>();
    initMusicList(music_list.get(), config.get()); // throw fatal error

    auto music_playing = std::make_unique<std::string>();
    auto controller    = std::make_unique<Controller>();

    std::thread cleaner(cleanFunc, ma_comp.get(), controller.get());
    cleaner.detach();

    std::string cmd;
    std::vector<std::string> args;
    while (!controller->getQuitFlag()) {
        args.clear();
        cmd = getCommand(pipe_to_server.get(), pipe_to_client.get()); // throw fatal error
        if (cmd == "") {
            LOG("failed to get command", LogType::ERROR);
            continue;
        }

        handleCommand(
            cmd,
            pipe_to_server.get(),
            pipe_to_client.get(),
            ma_comp.get(),
            music_playing.get(),
            music_list.get(),
            controller.get(),
            config.get()
        ); // throw fatal error
    }
    pipe_to_server->deletePipe();
    pipe_to_client->deletePipe();
}