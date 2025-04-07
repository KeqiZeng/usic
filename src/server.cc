#include "server.h"
#include "commands.h"
#include "core.h"
#include "log.h"
#include "named_pipe.h"
#include "runtime_path.h"
#include "utils.h"
#include <filesystem>
#include <format>
#include <optional>
#include <vector>

Server::Server()
    : pipe_to_client_(NamedPipe(RuntimePath::SERVER_TO_CLIENT_PIPE))
    , pipe_from_client_(NamedPipe(RuntimePath::CLIENT_TO_SERVER_PIPE))
{
}

void Server::run()
{
    try {
        setupRuntime();
    }
    catch (std::exception& e) {
        LOG(std::format("failed to setup runtime: {}", e.what()), LogType::ERROR);
        throw std::runtime_error(std::format("failed to setup runtime: {}", e.what()));
    }

    try {
        CoreComponents& core = CoreComponents::getInstance();
        while (!core.shouldQuit()) {
            const std::string& cmd = getMessageFromClient();
            handleCommand(cmd);
        }
    }
    catch (std::exception& e) {
        LOG(std::format("error occurred while running server: {}", e.what()), LogType::ERROR);
        throw std::runtime_error(std::format("error occurred while running server: {}", e.what()));
    }

    pipe_from_client_.closePipe();
    pipe_to_client_.closePipe();
    pipe_from_client_.deletePipe();
    pipe_to_client_.deletePipe();
}

void Server::setupRuntime()
{
    // create runtime path
    if (!std::filesystem::exists(RuntimePath::RUNTIME_PATH)) {
        std::filesystem::create_directory(RuntimePath::RUNTIME_PATH);
    }

    // remove log files for initialization
    removeLogFiles();

    // setup named pipes
    try {
        pipe_to_client_.setup();
    }
    catch (std::exception& e) {
        LOG(std::format("failed to setup {}: {}", RuntimePath::SERVER_TO_CLIENT_PIPE, e.what()), LogType::ERROR);
        throw std::runtime_error(std::format("failed to setup named pipe: {}", e.what()));
    }

    try {
        pipe_from_client_.setup();
    }
    catch (std::exception& e) {
        LOG(std::format("failed to setup {}: {}", RuntimePath::CLIENT_TO_SERVER_PIPE, e.what()), LogType::ERROR);
        throw std::runtime_error(std::format("failed to setup named pipe: {}", e.what()));
    }
}

std::string Server::getMessageFromClient() noexcept
{
    pipe_from_client_.openPipe(OpenMode::RD_ONLY_BLOCK);
    pipe_to_client_.openPipe(OpenMode::WR_ONLY_BLOCK);

    std::string msg = pipe_from_client_.readOut();
    pipe_from_client_.closePipe();
    pipe_to_client_.closePipe();
    return msg;
}

static std::vector<std::string> parseCommand(const std::string& cmd)
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

    if (!current.empty()) {
        args.push_back(current);
    }

    return args;
}

void Server::sendMsgToClient(std::string_view msg) noexcept
{
    pipe_from_client_.openPipe(OpenMode::RD_ONLY_BLOCK);
    pipe_to_client_.openPipe(OpenMode::WR_ONLY_BLOCK);
    pipe_to_client_.writeIn(msg);
    if (pipe_from_client_.readOut() == SpecialMessages::GOT) {
        pipe_to_client_.writeIn(SpecialMessages::OVER);
    }
    pipe_from_client_.closePipe();
    pipe_to_client_.closePipe();
}

void Server::sendMsgToClient(const std::vector<std::string>* msg)
{
    if (msg->size() == 0) {
        LOG("got an empty message", LogType::ERROR);
        return;
    }
    pipe_from_client_.openPipe(OpenMode::RD_ONLY_BLOCK);
    pipe_to_client_.openPipe(OpenMode::WR_ONLY_BLOCK);
    pipe_to_client_.writeIn(msg->at(0));
    std::string response = pipe_from_client_.readOut();
    if (response == SpecialMessages::GOT) {
        for (size_t i = 1; i < msg->size(); i++) {
            pipe_to_client_.writeIn(msg->at(i));
            response = pipe_from_client_.readOut();
        }
    }
    pipe_to_client_.writeIn(SpecialMessages::OVER);
    pipe_from_client_.closePipe();
    pipe_to_client_.closePipe();
}

void Server::logErrorAndSendToClient(std::string_view err)
{
    LOG(err, LogType::ERROR);
    sendMsgToClient(err);
}

void Server::handleCommand(const std::string& cmd)
{
    std::vector<std::string> args = parseCommand(cmd);
    if (args.empty()) {
        return;
    }
    const std::string& sub_cmd = args.at(0);
    if (Utils::commandEq(sub_cmd, Config::LOAD)) {
        if (args.size() >= 2) {
            Commands::load(args.at(1));
            sendMsgToClient(SpecialMessages::NO_MESSAGE);
        }
        else {
            logErrorAndSendToClient(std::format("{}: arguments are not enough.", Config::LOAD));
        }
    }
    else if (Utils::commandEq(sub_cmd, Config::PLAY)) {
        if (args.size() >= 2) {
            const std::string& music_to_play = args.at(1);
            ma_result result                 = Commands::play(music_to_play);
            if (result != MA_SUCCESS) {
                logErrorAndSendToClient(std::format("failed to play music: {}", music_to_play));
            }
            else {
                sendMsgToClient(SpecialMessages::NO_MESSAGE);
            }
        }
        else {
            const std::optional<std::string>& music_to_play = CoreComponents::getInstance().getAudio();
            if (!music_to_play.has_value()) {
                logErrorAndSendToClient("failed to get music to play");
            }

            ma_result result = Commands::play(music_to_play.value());
            if (result != MA_SUCCESS) {
                logErrorAndSendToClient(std::format("failed to play music: {}", music_to_play.value()));
            }
            else {
                sendMsgToClient(SpecialMessages::NO_MESSAGE);
            }
        }
    }
    else if (Utils::commandEq(sub_cmd, Config::PLAY_LATER)) {
        if (args.size() >= 2) {
            const std::string& music_to_play = args.at(1);
            Commands::playLater(music_to_play);
            sendMsgToClient(SpecialMessages::NO_MESSAGE);
        }
        else {
            logErrorAndSendToClient(std::format("{}: arguments are not enough.", Config::PLAY_LATER));
        }
    }
    else if (Utils::commandEq(sub_cmd, Config::PLAY_NEXT)) {
        ma_result result = Commands::playNext();
        if (result != MA_SUCCESS) {
            logErrorAndSendToClient("failed to play next music");
        }
        else {
            sendMsgToClient(SpecialMessages::NO_MESSAGE);
        }
    }
    else if (Utils::commandEq(sub_cmd, Config::PLAY_PREV)) {
        ma_result result = Commands::playPrev();
        if (result != MA_SUCCESS) {
            logErrorAndSendToClient("failed to play prev music");
        }
        else {
            sendMsgToClient(SpecialMessages::NO_MESSAGE);
        }
    }
    else if (Utils::commandEq(sub_cmd, Config::PAUSE)) {
        Commands::pause();
        sendMsgToClient(SpecialMessages::NO_MESSAGE);
    }
    else if (Utils::commandEq(sub_cmd, Config::SEEK_FORWARD)) {
        ma_result result = Commands::seekForward();
        if (result != MA_SUCCESS) {
            logErrorAndSendToClient("failed to move cursor forward");
        }
        else {
            sendMsgToClient(SpecialMessages::NO_MESSAGE);
        }
    }
    else if (Utils::commandEq(sub_cmd, Config::SEEK_BACKWARD)) {
        ma_result result = Commands::seekBackward();
        if (result != MA_SUCCESS) {
            logErrorAndSendToClient("failed to move cursor backward");
        }
        else {
            sendMsgToClient(SpecialMessages::NO_MESSAGE);
        }
    }
    else if (Utils::commandEq(sub_cmd, Config::SEEK_TO)) {
        if (args.size() >= 2) {
            const std::string& time = args.at(1);
            ma_result result        = Commands::seekTo(time);
            if (result != MA_SUCCESS) {
                logErrorAndSendToClient("failed to set cursor");
            }
            else {
                sendMsgToClient(SpecialMessages::NO_MESSAGE);
            }
        }
        else {
            logErrorAndSendToClient(std::format("{}: arguments are not enough.", Config::SEEK_TO));
        }
    }
    else if (Utils::commandEq(sub_cmd, Config::GET_PROGRESS)) {
        std::optional<const std::string> bar = Commands::getProgressBar();
        if (bar.has_value()) {
            sendMsgToClient(bar.value());
        }
        else {
            logErrorAndSendToClient("failed to get progress");
        }
    }
    else if (Utils::commandEq(sub_cmd, Config::VOLUME_UP)) {
        ma_result result = Commands::volumeUp();
        if (result != MA_SUCCESS) {
            logErrorAndSendToClient("failed to turn the volume up");
        }
        else {
            sendMsgToClient(SpecialMessages::NO_MESSAGE);
        }
    }
    else if (Utils::commandEq(sub_cmd, Config::VOLUME_DOWN)) {
        ma_result result = Commands::volumeDown();
        if (result != MA_SUCCESS) {
            logErrorAndSendToClient("failed to turn the volume down");
        }
        else {
            sendMsgToClient(SpecialMessages::NO_MESSAGE);
        }
    }
    else if (Utils::commandEq(sub_cmd, Config::SET_VOLUME)) {
        if (args.size() >= 2) {
            const std::string& volume = args.at(1);
            ma_result result          = Commands::setVolume(volume);
            if (result != MA_SUCCESS) {
                logErrorAndSendToClient("failed to set volume");
            }
            else {
                sendMsgToClient(SpecialMessages::NO_MESSAGE);
            }
        }
        else {
            logErrorAndSendToClient(std::format("{}, arguments are not enough.", Config::SET_VOLUME));
        }
    }
    else if (Utils::commandEq(sub_cmd, Config::GET_VOLUME)) {
        float volume = Commands::getVolume();
        sendMsgToClient(std::format("volume: {}", volume));
    }
    else if (Utils::commandEq(sub_cmd, Config::MUTE)) {
        ma_result result = Commands::mute();
        if (result != MA_SUCCESS) {
            logErrorAndSendToClient("failed to (un)mute");
        }
        else {
            sendMsgToClient(SpecialMessages::NO_MESSAGE);
        }
    }
    else if (Utils::commandEq(sub_cmd, Config::SET_MODE)) {
        if (args.size() >= 2) {
            const std::string& mode_str = args.at(1);
            PlayMode mode{};
            if (mode_str == "sequence") {
                mode = PlayMode::SEQUENCE;
            }
            else if (mode_str == "shuffle") {
                mode = PlayMode::SHUFFLE;
            }
            else if (mode_str == "single") {
                mode = PlayMode::SINGLE;
            }
            else {
                logErrorAndSendToClient(std::format("{}: invalid mode: {}", Config::SET_MODE, mode_str));
                return;
            }
            Commands::setMode(mode);
            sendMsgToClient(SpecialMessages::NO_MESSAGE);
        }
        else {
            logErrorAndSendToClient(std::format("{}: arguments are not enough.", Config::SET_MODE));
        }
    }
    else if (Utils::commandEq(sub_cmd, Config::GET_MODE)) {
        PlayMode mode = Commands::getMode();
        std::string mode_str;
        switch (mode) {
        case PlayMode::SEQUENCE:
            mode_str = "sequence";
            break;
        case PlayMode::SHUFFLE:
            mode_str = "shuffle";
            break;
        case PlayMode::SINGLE:
            mode_str = "single";
            break;
        default:
            mode_str = "unknown";
            break;
        }
        sendMsgToClient(std::format("mode: {}", mode_str));
    }
    else if (Utils::commandEq(sub_cmd, Config::GET_LIST)) {
        const std::vector<std::string> LIST = Commands::getList();
        sendMsgToClient(&LIST);
    }
    else if (Utils::commandEq(sub_cmd, Config::QUIT)) {
        Commands::quit();
        sendMsgToClient("server quit successfully");
    }
    else {
        logErrorAndSendToClient(std::format("unknown command: {}", sub_cmd));
    }
}

Server& Server::getInstance() noexcept
{
    static Server instance;
    return instance;
}
