use crate::config::Config;
use crate::ipc::{self, Request, Response};
use anyhow::{bail, Context, Result};
use macos::start_macos_media_bridge;
use player::Player;
use std::fs;
use std::io::{BufRead, BufReader, Write};
use std::os::unix::net::{UnixListener, UnixStream};
use std::path::Path;
use std::sync::mpsc::{self, Sender};

mod macos;
mod player;
mod playlist;

pub fn run() -> Result<()> {
    let cfg = Config::load_or_create()?;
    cfg.ensure_dirs()?;
    run_server(cfg)
}

fn run_server(cfg: Config) -> Result<()> {
    bind_guard(&cfg.socket_path)?;
    let listener = UnixListener::bind(&cfg.socket_path)
        .with_context(|| format!("failed to bind {}", cfg.socket_path.display()))?;
    let (event_tx, event_rx) = mpsc::channel();
    spawn_accept_thread(listener, event_tx.clone());
    let _macos_media = start_macos_media_bridge(&cfg.socket_path);

    let mut player = Player::new(cfg.clone(), event_tx)?;
    eprintln!("usic server listening on {}", cfg.socket_path.display());

    let mut quitting = false;
    while !quitting {
        match event_rx.recv().context("server event channel closed")? {
            ServerEvent::Client(stream) => match read_request(&stream) {
                Ok(request) => {
                    let response = match player.handle(request) {
                        Ok((response, should_quit)) => {
                            quitting = should_quit;
                            response
                        }
                        Err(err) => ipc::err(err.to_string()),
                    };
                    if let Err(err) = write_response(stream, &response) {
                        eprintln!("failed to write client response: {err}");
                    }
                }
                Err(err) => {
                    eprintln!("failed to read client request: {err}");
                }
            },
            ServerEvent::TrackFinished(token) => {
                player.advance_if_finished(token);
            }
        }
    }

    let _ = fs::remove_file(&cfg.socket_path);
    Ok(())
}

pub(super) enum ServerEvent {
    Client(UnixStream),
    TrackFinished(u64),
}

fn spawn_accept_thread(listener: UnixListener, event_tx: Sender<ServerEvent>) {
    std::thread::spawn(move || {
        for stream in listener.incoming() {
            let Ok(stream) = stream else {
                break;
            };
            if event_tx.send(ServerEvent::Client(stream)).is_err() {
                break;
            }
        }
    });
}

fn bind_guard(socket_path: &Path) -> Result<()> {
    if !socket_path.exists() {
        return Ok(());
    }
    if UnixStream::connect(socket_path).is_ok() {
        bail!("server already running at {}", socket_path.display());
    }
    fs::remove_file(socket_path)
        .with_context(|| format!("failed to remove stale {}", socket_path.display()))
}

fn read_request(stream: &UnixStream) -> Result<Request> {
    let mut line = String::new();
    let mut reader = BufReader::new(stream);
    reader.read_line(&mut line)?;
    if line.is_empty() {
        bail!("client disconnected before sending a request");
    }
    serde_json::from_str(line.trim_end()).context("failed to parse request")
}

fn write_response(mut stream: UnixStream, response: &Response) -> Result<()> {
    serde_json::to_writer(&mut stream, response)?;
    stream.write_all(b"\n")?;
    stream.flush()?;
    Ok(())
}
