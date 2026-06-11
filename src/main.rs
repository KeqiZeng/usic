use anyhow::{bail, Context, Result};
use clap::{Parser, Subcommand};
use std::io::{self, Write};
use usic::config::{Config, PlayMode};
use usic::fuzzy;
use usic::ipc::{self, Request, Response, ResponseData, SeekTarget};
use usic::library;
use usic::server;
use usic::server_control;
use usic::time;
use usic::tui;

#[derive(Debug, Parser)]
#[command(version, about = "A minimal local music player")]
struct Cli {
    #[command(subcommand)]
    command: CommandKind,
}

#[derive(Debug, Subcommand)]
enum CommandKind {
    Server {
        #[command(subcommand)]
        command: ServerCommand,
    },
    Play {
        query: Option<String>,
    },
    Pause,
    Next,
    Prev,
    Later {
        query: Option<String>,
    },
    Seek {
        target: String,
    },
    Volume {
        value: String,
    },
    Mode {
        mode: PlayModeArg,
    },
    Tui,
    Status,
    #[command(name = "__server", hide = true)]
    InternalServer,
}

#[derive(Debug, Subcommand)]
enum ServerCommand {
    Start,
    Stop,
    Status,
}

#[derive(Debug, Clone, Copy, clap::ValueEnum)]
enum PlayModeArg {
    Sequence,
    Shuffle,
    Single,
}

impl From<PlayModeArg> for PlayMode {
    fn from(value: PlayModeArg) -> Self {
        match value {
            PlayModeArg::Sequence => PlayMode::Sequence,
            PlayModeArg::Shuffle => PlayMode::Shuffle,
            PlayModeArg::Single => PlayMode::Single,
        }
    }
}

fn main() -> Result<()> {
    let cli = Cli::parse();
    let cfg = Config::load_or_create()?;
    cfg.ensure_dirs()?;

    match cli.command {
        CommandKind::Server { command } => handle_server(&cfg, command),
        CommandKind::Play { query } => handle_play(&cfg, query),
        CommandKind::Pause => send_and_print(&cfg, Request::TogglePause, true),
        CommandKind::Next => send_and_print(&cfg, Request::Next, true),
        CommandKind::Prev => send_and_print(&cfg, Request::Prev, true),
        CommandKind::Later { query } => handle_later(&cfg, query),
        CommandKind::Seek { target } => send_and_print(
            &cfg,
            Request::Seek {
                target: parse_seek(&target)?,
            },
            true,
        ),
        CommandKind::Volume { value } => handle_volume(&cfg, &value),
        CommandKind::Mode { mode } => {
            send_and_print(&cfg, Request::SetMode { mode: mode.into() }, true)
        }
        CommandKind::Tui => {
            server_control::ensure_server_running(&cfg)?;
            tui::run()
        }
        CommandKind::Status => send_and_print(&cfg, Request::GetStatus, true),
        CommandKind::InternalServer => server::run(),
    }
}

fn handle_server(cfg: &Config, command: ServerCommand) -> Result<()> {
    match command {
        ServerCommand::Start => {
            if server_control::start_server(cfg)? {
                println!("server started");
            } else {
                println!("server already running");
            }
            Ok(())
        }
        ServerCommand::Stop => send_and_print(cfg, Request::Quit, false),
        ServerCommand::Status => send_and_print(cfg, Request::GetStatus, false),
    }
}

fn handle_volume(cfg: &Config, value: &str) -> Result<()> {
    match value {
        "up" => send_and_print(cfg, Request::VolumeUp, true),
        "down" => send_and_print(cfg, Request::VolumeDown, true),
        "mute" => send_and_print(cfg, Request::ToggleMute, true),
        value => send_and_print(
            cfg,
            Request::SetVolume {
                value: value.parse()?,
            },
            true,
        ),
    }
}

fn handle_play(cfg: &Config, query: Option<String>) -> Result<()> {
    let tracks = library::scan(cfg)?;
    let track = choose_track(query, &tracks)?;
    send_and_print(cfg, Request::Play { track: Some(track) }, true)
}

fn handle_later(cfg: &Config, query: Option<String>) -> Result<()> {
    let tracks = library::scan(cfg)?;
    let track = choose_track(query, &tracks)?;
    send_and_print(cfg, Request::PlayLater { track }, true)
}

fn choose_track(query: Option<String>, tracks: &[String]) -> Result<String> {
    if tracks.is_empty() {
        bail!("music library is empty");
    }
    let query = match query {
        Some(query) => query,
        None => {
            print!("search: ");
            io::stdout().flush()?;
            let mut query = String::new();
            io::stdin().read_line(&mut query)?;
            query.trim().to_string()
        }
    };
    let matches = fuzzy::rank(&query, tracks, 10);
    if matches.is_empty() {
        bail!("no match for query: {query}");
    }
    for (index, track) in matches.iter().enumerate() {
        println!("{:>2}. {}", index + 1, track);
    }
    print!("select [1]: ");
    io::stdout().flush()?;
    let mut selected = String::new();
    io::stdin().read_line(&mut selected)?;
    let index = if selected.trim().is_empty() {
        0
    } else {
        selected.trim().parse::<usize>()?.saturating_sub(1)
    };
    matches
        .get(index)
        .map(|track| (*track).to_string())
        .context("selection out of range")
}

fn parse_seek(value: &str) -> Result<SeekTarget> {
    if let Some(rest) = value.strip_prefix('+') {
        return Ok(SeekTarget::Relative {
            seconds: time::parse_time(rest)? as i64,
        });
    }
    if let Some(rest) = value.strip_prefix('-') {
        return Ok(SeekTarget::Relative {
            seconds: -(time::parse_time(rest)? as i64),
        });
    }
    Ok(SeekTarget::Absolute {
        seconds: time::parse_time(value)?,
    })
}

fn send_and_print(cfg: &Config, request: Request, auto_start: bool) -> Result<()> {
    if auto_start {
        server_control::ensure_server_running(cfg)?;
    }
    match ipc::send(&cfg.socket_path, &request)? {
        Response::Ok(ResponseData::Empty) => Ok(()),
        Response::Ok(ResponseData::Message(message)) => {
            println!("{message}");
            Ok(())
        }
        Response::Ok(ResponseData::Status(status)) => {
            let current = status.current_track.unwrap_or_else(|| "-".to_string());
            let duration = status
                .duration_secs
                .map(time::format_time)
                .unwrap_or_else(|| "--:--".to_string());
            println!("track: {current}");
            println!(
                "time: {}/{}",
                time::format_time(status.position_secs),
                duration
            );
            println!(
                "volume: {:.2}{}",
                status.volume,
                if status.muted { " muted" } else { "" }
            );
            println!("mode: {:?}", status.mode);
            println!("paused: {}", status.paused);
            println!(
                "playlist: {}",
                status.playlist_name.unwrap_or_else(|| "-".to_string())
            );
            Ok(())
        }
        Response::Ok(ResponseData::Playlist(tracks)) => {
            for track in tracks {
                println!("{track}");
            }
            Ok(())
        }
        Response::Err { message } => bail!(message),
    }
}
