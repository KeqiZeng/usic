#pragma once

#include "miniaudio.h"
#include <string>
#include <string_view>

namespace Utils
{

/**
 * Reinitializes the given decoder with a new file and configuration.
 *
 * This function first uninitializes the existing decoder and then initializes
 * it with the specified file and configuration. If any step fails, an error
 * result is returned.
 *
 * @param filename The name of the file to initialize the decoder with.
 * @param config A pointer to the decoder configuration.
 * @param decoder A pointer to the decoder to be reinitialized.
 * @return MA_SUCCESS on success, or an error code on failure.
 */
ma_result reinitDecoder(std::string_view filename, ma_decoder_config* config, ma_decoder* decoder);

/**
 * Seeks to the start of the audio data in the given decoder.
 *
 * @param decoder A pointer to the decoder to seek to the start of.
 * @return MA_SUCCESS on success, or an error code on failure.
 */
ma_result seekToStart(ma_decoder* decoder) noexcept;

/**
 * Seeks to the end of the audio data in the given decoder.
 *
 * @param decoder A pointer to the decoder to seek to the end of.
 * @return MA_SUCCESS on success, or an error code on failure.
 */
ma_result seekToEnd(ma_decoder* decoder);

/**
 * Gets the corresponding name of the WAV file from the given filename.
 *
 * @param filename The name of the file to get the WAV name from.
 * @return The name of the WAV file.
 */
std::string getWAVFileName(std::string_view filename);

/**
 * Creates an empty file with the given name.
 *
 * @param filename The full path and name of the file to create.
 */
void createFile(std::string_view filename);

} // namespace Utils