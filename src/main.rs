use anyhow::{bail, Context, Result};
use clap::{Parser, Subcommand};
use std::io::{self, Write};
use std::process::{Command, Stdio};
use usic::config::{Config, PlayMode};
use usic::fuzzy;
use usic::ipc::{self, Request, Response, ResponseData, SeekTarget};
use usic::library;
use usic::time;

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
        CommandKind::Pause => send_and_print(&cfg, Request::TogglePause),
        CommandKind::Next => send_and_print(&cfg, Request::Next),
        CommandKind::Prev => send_and_print(&cfg, Request::Prev),
        CommandKind::Later { query } => handle_later(&cfg, query),
        CommandKind::Seek { target } => send_and_print(
            &cfg,
            Request::Seek {
                target: parse_seek(&target)?,
            },
        ),
        CommandKind::Volume { value } => handle_volume(&cfg, &value),
        CommandKind::Mode { mode } => send_and_print(&cfg, Request::SetMode { mode: mode.into() }),
        CommandKind::Tui => run_tui(),
        CommandKind::Status => send_and_print(&cfg, Request::GetStatus),
    }
}

fn handle_server(cfg: &Config, command: ServerCommand) -> Result<()> {
    match command {
        ServerCommand::Start => {
            if cfg.socket_path.exists()
                && std::os::unix::net::UnixStream::connect(&cfg.socket_path).is_ok()
            {
                println!("server already running");
                return Ok(());
            }
            let mut exe = std::env::current_exe()?;
            exe.set_file_name("usic-server");
            Command::new(exe)
                .stdin(Stdio::null())
                .stdout(Stdio::null())
                .stderr(Stdio::null())
                .spawn()
                .context("failed to start usic-server")?;
            println!("server started");
            Ok(())
        }
        ServerCommand::Stop => send_and_print(cfg, Request::Quit),
        ServerCommand::Status => send_and_print(cfg, Request::GetStatus),
    }
}

fn handle_volume(cfg: &Config, value: &str) -> Result<()> {
    match value {
        "up" => send_and_print(cfg, Request::VolumeUp),
        "down" => send_and_print(cfg, Request::VolumeDown),
        "mute" => send_and_print(cfg, Request::ToggleMute),
        value => send_and_print(
            cfg,
            Request::SetVolume {
                value: value.parse()?,
            },
        ),
    }
}

fn handle_play(cfg: &Config, query: Option<String>) -> Result<()> {
    let tracks = library::scan(cfg)?;
    let track = choose_track(query, &tracks)?;
    send_and_print(cfg, Request::Play { track: Some(track) })
}

fn handle_later(cfg: &Config, query: Option<String>) -> Result<()> {
    let tracks = library::scan(cfg)?;
    let track = choose_track(query, &tracks)?;
    send_and_print(cfg, Request::PlayLater { track })
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

fn send_and_print(cfg: &Config, request: Request) -> Result<()> {
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

fn run_tui() -> Result<()> {
    let mut exe = std::env::current_exe()?;
    exe.set_file_name("usic-tui");
    let status = Command::new(exe)
        .status()
        .context("failed to start usic-tui")?;
    if !status.success() {
        bail!("usic-tui exited with {status}");
    }
    Ok(())
}
