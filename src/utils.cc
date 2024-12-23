#include "utils.h"
#include "log.h"
#include "miniaudio.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>

namespace Utils
{

ma_result reinitDecoder(std::string_view filename, ma_decoder_config* config, ma_decoder* decoder) noexcept
{
    ma_result result = ma_decoder_uninit(decoder);
    result           = ma_decoder_init_file(filename.data(), config, decoder);
    return result;
}

ma_result seekToStart(ma_decoder* decoder)
{
    return ma_decoder_seek_to_pcm_frame(decoder, 0);
}

ma_result seekToEnd(ma_decoder* decoder)
{
    ma_uint64 length = 0;
    ma_result result = ma_decoder_get_length_in_pcm_frames(decoder, &length);
    if (result != MA_SUCCESS) {
        LOG("failed to get length in PCM frames", LogType::ERROR, result);
        return result;
    }
    result = ma_decoder_seek_to_pcm_frame(decoder, length);
    if (result != MA_SUCCESS) {
        LOG("failed to seek to PCM frame with length", LogType::ERROR, result);
        return result;
    }
    return result;
}

std::string getWAVFileName(std::string_view filename)
{
    // create a file in temp directory with same name but .wav extension
    std::filesystem::path file_path(filename);
    std::string base_name          = file_path.stem().string();
    std::string temp_dir           = std::filesystem::temp_directory_path().string();
    std::filesystem::path wav_path = std::filesystem::path(temp_dir) / "usic";

    // create the usic directory in temp if it doesn't exist
    if (!std::filesystem::exists(wav_path)) {
        std::filesystem::create_directories(wav_path);
    }

    return (wav_path / (base_name + ".wav")).string();
}

void createFile(std::string_view filename)
{
    // Create parent directory if it doesn't exist
    std::filesystem::path file_path(filename);
    if (!std::filesystem::exists(file_path.parent_path())) {
        std::filesystem::create_directories(file_path.parent_path());
    }

    // Create file if it doesn't exist
    std::ofstream out_file(filename.data(), std::ios::app);
    if (!out_file) {
        throw std::ios_base::failure("failed to create file");
    }
    out_file.close();
}

} // namespace Utils