#pragma once
#include <string>

const std::string VERSION = "0.1.0";
const std::string DOC     = "USAGE:\n"
                            "  usic                    Start server\n"
                            "  usic [FLAG]\n"
                            "  usic <COMMAND> [ARGUMENT]\n"
                            "\n"
                            "Available FLAG:\n"
                            "  -h, --help              Print this document\n"
                            "  -v, --version           Print version\n"
                            "\n"
                            "Available COMMANDS:\n"
                            "Server not required:\n"
                            "  add_music_to_list       Add music to playlist                            REQUIRED ARGUMENT: "
                            "The name of playlist\n"
                            "  fuzzy_play              Select a music from default list and play        NO ARGUMENT\n"
                            "  fuzzy_play_later        Select a music from default list and play later  NO ARGUMENT\n"
                            "\n"
                            "Server required:\n"
                            "  play                    Play music                                       OPTIONAL ARGUMENT: "
                            "The name of the music file\n"
                            "  pause                   Pause or resume music                            NO ARGUMENT\n"
                            "  play_next               Play next music in playlist                      NO ARGUMENT\n"
                            "  play_prev               Play previous music                              NO ARGUMENT\n"
                            "  play_later              Play given music next                            REQUIRED ARGUMENT: "
                            "The name of music file\n"
                            "  load                    Load playlist                                    REQUIRED ARGUMENT: "
                            "The name of playlist\n"
                            "  get_list                Print playlist                                   NO ARGUMENT\n"
                            "  seek_forward            Move cursor forward                              NO ARGUMENT\n"
                            "  seek_backward           Move cursor backward                             NO ARGUMENT\n"
                            "  seek_to                 Set cursor to given time                         REQUIRED ARGUMENT: "
                            "Time in MM:SS format\n"
                            "  get_progress            Get the playing progress                         NO ARGUMENT\n"
                            "  volume_up               Increase volume                                  NO ARGUMENT\n"
                            "  volume_down             Decrease volume                                  NO ARGUMENT\n"
                            "  set_volume              Set volume                                       REQUIRED ARGUMENT: "
                            "Volume between 0.0 and 1.0\n"
                            "  get_volume              Get current volume                               NO ARGUMENT\n"
                            "  mute                    Mute or unmute volume                            NO ARGUMENT\n"
                            "  set_mode                Set play mode                                    REQUIRED ARGUMENT: "
                            "normal, shuffle or single\n"
                            "  get_mode                Get current play mode                            NO ARGUMENT\n"
                            "  quit                    Quit server                                      NO ARGUMENT\n";
