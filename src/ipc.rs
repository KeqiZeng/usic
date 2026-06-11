use crate::config::PlayMode;
use anyhow::{Context, Result};
use serde::{Deserialize, Serialize};
use std::io::{BufRead, BufReader, Write};
use std::os::unix::net::UnixStream;
use std::path::Path;

#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(tag = "type", rename_all = "snake_case")]
pub enum SeekTarget {
    Absolute { seconds: u64 },
    Relative { seconds: i64 },
}

#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(tag = "type", rename_all = "snake_case")]
pub enum Request {
    Play { track: Option<String> },
    TogglePause,
    Next,
    Prev,
    PlayLater { track: String },
    Seek { target: SeekTarget },
    SetVolume { value: f32 },
    VolumeUp,
    VolumeDown,
    ToggleMute,
    SetMode { mode: PlayMode },
    LoadPlaylist { name: String },
    GetStatus,
    GetPlaylist,
    Quit,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Status {
    pub current_track: Option<String>,
    pub position_secs: u64,
    pub duration_secs: Option<u64>,
    pub volume: f32,
    pub muted: bool,
    pub paused: bool,
    pub mode: PlayMode,
    pub playlist_name: Option<String>,
    pub queue_len: usize,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(tag = "status", content = "data", rename_all = "snake_case")]
pub enum Response {
    Ok(ResponseData),
    Err { message: String },
}

#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(tag = "type", content = "value", rename_all = "snake_case")]
pub enum ResponseData {
    Empty,
    Message(String),
    Status(Status),
    Playlist(Vec<String>),
}

pub fn send(socket_path: &Path, request: &Request) -> Result<Response> {
    let mut stream = UnixStream::connect(socket_path)
        .with_context(|| format!("failed to connect to {}", socket_path.display()))?;
    serde_json::to_writer(&mut stream, request)?;
    stream.write_all(b"\n")?;
    stream.flush()?;

    let mut line = String::new();
    let mut reader = BufReader::new(stream);
    reader.read_line(&mut line)?;
    serde_json::from_str(line.trim_end()).context("failed to parse server response")
}

pub fn ok_empty() -> Response {
    Response::Ok(ResponseData::Empty)
}

pub fn ok_message(message: impl Into<String>) -> Response {
    Response::Ok(ResponseData::Message(message.into()))
}

pub fn err(message: impl Into<String>) -> Response {
    Response::Err {
        message: message.into(),
    }
}
