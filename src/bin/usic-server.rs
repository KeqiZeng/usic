use anyhow::{bail, Context, Result};
use rand::Rng;
use rodio::{Decoder, OutputStream, OutputStreamBuilder, Sink, Source};
use std::fs::{self, File};
use std::io::{BufRead, BufReader, Write};
use std::os::unix::net::{UnixListener, UnixStream};
use std::path::Path;
use std::sync::mpsc::{self, Sender};
use std::sync::Arc;
use std::time::Duration;
use usic::config::{Config, PlayMode};
use usic::ipc::{self, Request, Response, ResponseData, SeekTarget, Status};
use usic::library;
use usic::playlist::Playlist;

fn main() -> Result<()> {
    let cfg = Config::load_or_create()?;
    cfg.ensure_dirs()?;
    run(cfg)
}

fn run(cfg: Config) -> Result<()> {
    bind_guard(&cfg.socket_path)?;
    let listener = UnixListener::bind(&cfg.socket_path)
        .with_context(|| format!("failed to bind {}", cfg.socket_path.display()))?;
    let (event_tx, event_rx) = mpsc::channel();
    spawn_accept_thread(listener, event_tx.clone());

    let mut player = Player::new(cfg.clone(), event_tx)?;
    eprintln!("usic-server listening on {}", cfg.socket_path.display());

    let mut quitting = false;
    while !quitting {
        match event_rx.recv().context("server event channel closed")? {
            ServerEvent::Client(stream) => {
                let response =
                    match read_request(&stream).and_then(|request| player.handle(request)) {
                        Ok((response, should_quit)) => {
                            quitting = should_quit;
                            response
                        }
                        Err(err) => ipc::err(err.to_string()),
                    };
                write_response(stream, &response)?;
            }
            ServerEvent::TrackFinished(token) => {
                player.advance_if_finished(token);
            }
        }
    }

    let _ = fs::remove_file(&cfg.socket_path);
    Ok(())
}

enum ServerEvent {
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
    serde_json::from_str(line.trim_end()).context("failed to parse request")
}

fn write_response(mut stream: UnixStream, response: &Response) -> Result<()> {
    serde_json::to_writer(&mut stream, response)?;
    stream.write_all(b"\n")?;
    stream.flush()?;
    Ok(())
}

struct Player {
    cfg: Config,
    event_tx: Sender<ServerEvent>,
    _stream: OutputStream,
    sink: Arc<Sink>,
    playlist: Playlist,
    current_index: Option<usize>,
    current_track: Option<String>,
    current_duration: Option<u64>,
    playback_token: u64,
    mode: PlayMode,
    volume: f32,
    last_volume: f32,
    paused: bool,
}

impl Player {
    fn new(cfg: Config, event_tx: Sender<ServerEvent>) -> Result<Self> {
        let stream = OutputStreamBuilder::open_default_stream()
            .context("failed to open default audio output")?;
        let sink = Arc::new(Sink::connect_new(stream.mixer()));
        let playlist = load_runtime_playlist(&cfg, &cfg.default_playlist, true)?;
        sink.set_volume(1.0);
        Ok(Self {
            cfg: cfg.clone(),
            event_tx,
            _stream: stream,
            sink,
            playlist,
            current_index: None,
            current_track: None,
            current_duration: None,
            playback_token: 0,
            mode: cfg.default_mode,
            volume: 1.0,
            last_volume: 1.0,
            paused: false,
        })
    }

    fn handle(&mut self, request: Request) -> Result<(Response, bool)> {
        let response = match request {
            Request::Play { track } => {
                self.play_requested(track)?;
                ipc::ok_empty()
            }
            Request::TogglePause => {
                self.toggle_pause();
                ipc::ok_empty()
            }
            Request::Next => {
                self.play_next()?;
                ipc::ok_empty()
            }
            Request::Prev => {
                self.play_prev()?;
                ipc::ok_empty()
            }
            Request::PlayLater { track } => {
                self.play_later(track);
                ipc::ok_empty()
            }
            Request::Seek { target } => {
                self.seek(target)?;
                ipc::ok_empty()
            }
            Request::SetVolume { value } => {
                self.set_volume(value);
                ipc::ok_empty()
            }
            Request::VolumeUp => {
                self.set_volume(self.volume + self.cfg.volume_step);
                ipc::ok_empty()
            }
            Request::VolumeDown => {
                self.set_volume(self.volume - self.cfg.volume_step);
                ipc::ok_empty()
            }
            Request::ToggleMute => {
                self.toggle_mute();
                ipc::ok_empty()
            }
            Request::SetMode { mode } => {
                self.mode = mode;
                ipc::ok_empty()
            }
            Request::LoadPlaylist { name } => {
                self.playlist =
                    load_runtime_playlist(&self.cfg, &name, name == self.cfg.default_playlist)?;
                self.current_index = None;
                ipc::ok_empty()
            }
            Request::GetStatus => Response::Ok(ResponseData::Status(self.status())),
            Request::GetPlaylist => {
                Response::Ok(ResponseData::Playlist(self.playlist.tracks.clone()))
            }
            Request::Quit => return Ok((ipc::ok_message("server quit successfully"), true)),
        };
        Ok((response, false))
    }

    fn play_requested(&mut self, track: Option<String>) -> Result<()> {
        let track = match track {
            Some(track) => track,
            None => self
                .current_index
                .and_then(|idx| self.playlist.tracks.get(idx).cloned())
                .or_else(|| self.playlist.tracks.first().cloned())
                .context("playlist is empty")?,
        };
        self.play_track(track)
    }

    fn play_track(&mut self, track: String) -> Result<()> {
        let path = library::resolve_track(&self.cfg, &track);
        let file =
            File::open(&path).with_context(|| format!("failed to open {}", path.display()))?;
        let source = Decoder::try_from(file)
            .with_context(|| format!("failed to decode {}", path.display()))?;
        self.current_duration = source.total_duration().map(|duration| duration.as_secs());
        self.sink.stop();
        self.sink = Arc::new(Sink::connect_new(self._stream.mixer()));
        self.sink.set_volume(self.volume);
        if self.paused {
            self.sink.pause();
        }
        self.sink.append(source);
        self.playback_token = self.playback_token.wrapping_add(1);
        self.spawn_completion_waiter(self.playback_token);
        self.current_index = self
            .playlist
            .tracks
            .iter()
            .position(|candidate| candidate == &track);
        self.current_track = Some(track);
        Ok(())
    }

    fn play_next(&mut self) -> Result<()> {
        let Some(index) = self.next_index() else {
            bail!("playlist is empty");
        };
        let track = self.playlist.tracks[index].clone();
        self.current_index = Some(index);
        self.play_track(track)
    }

    fn play_prev(&mut self) -> Result<()> {
        if self.playlist.tracks.is_empty() {
            bail!("playlist is empty");
        }
        let index = match self.current_index {
            Some(0) | None => self.playlist.tracks.len() - 1,
            Some(index) => index - 1,
        };
        let track = self.playlist.tracks[index].clone();
        self.current_index = Some(index);
        self.play_track(track)
    }

    fn play_later(&mut self, track: String) {
        let insert_at = self
            .current_index
            .map(|idx| idx + 1)
            .unwrap_or(self.playlist.tracks.len());
        if let Some(existing) = self
            .playlist
            .tracks
            .iter()
            .position(|candidate| candidate == &track)
        {
            self.playlist.tracks.remove(existing);
        }
        self.playlist
            .tracks
            .insert(insert_at.min(self.playlist.tracks.len()), track);
    }

    fn seek(&mut self, target: SeekTarget) -> Result<()> {
        let current = self.sink.get_pos().as_secs();
        let seconds = match target {
            SeekTarget::Absolute { seconds } => seconds,
            SeekTarget::Relative { seconds } if seconds.is_negative() => {
                current.saturating_sub(seconds.unsigned_abs())
            }
            SeekTarget::Relative { seconds } => current.saturating_add(seconds as u64),
        };
        self.sink
            .try_seek(Duration::from_secs(seconds))
            .map_err(|err| anyhow::anyhow!("failed to seek: {err}"))?;
        Ok(())
    }

    fn toggle_pause(&mut self) {
        self.paused = !self.paused;
        if self.paused {
            self.sink.pause();
        } else {
            self.sink.play();
        }
    }

    fn set_volume(&mut self, volume: f32) {
        let volume = volume.clamp(0.0, 1.0);
        if volume > 0.0 {
            self.last_volume = volume;
        }
        self.volume = volume;
        self.sink.set_volume(volume);
    }

    fn toggle_mute(&mut self) {
        if self.volume == 0.0 {
            self.set_volume(self.last_volume);
        } else {
            self.last_volume = self.volume;
            self.set_volume(0.0);
        }
    }

    fn advance_if_finished(&mut self, token: u64) {
        if token == self.playback_token
            && self.current_track.is_some()
            && !self.paused
            && self.sink.empty()
        {
            if self.play_next().is_err() {
                self.current_track = None;
                self.current_duration = None;
            }
        }
    }

    fn spawn_completion_waiter(&self, token: u64) {
        let sink = Arc::clone(&self.sink);
        let event_tx = self.event_tx.clone();
        std::thread::spawn(move || {
            sink.sleep_until_end();
            let _ = event_tx.send(ServerEvent::TrackFinished(token));
        });
    }

    fn next_index(&self) -> Option<usize> {
        if self.playlist.tracks.is_empty() {
            return None;
        }
        match self.mode {
            PlayMode::Single => Some(self.current_index.unwrap_or(0)),
            PlayMode::Sequence => Some(
                self.current_index
                    .map(|idx| (idx + 1) % self.playlist.tracks.len())
                    .unwrap_or(0),
            ),
            PlayMode::Shuffle => Some(rand::thread_rng().gen_range(0..self.playlist.tracks.len())),
        }
    }

    fn status(&self) -> Status {
        Status {
            current_track: self.current_track.clone(),
            position_secs: self.sink.get_pos().as_secs(),
            duration_secs: self.current_duration,
            volume: self.volume,
            muted: self.volume == 0.0,
            paused: self.paused,
            mode: self.mode,
            playlist_name: Some(self.playlist.name.clone()),
            queue_len: self.playlist.tracks.len(),
        }
    }
}

fn load_runtime_playlist(cfg: &Config, name: &str, rebuild_if_empty: bool) -> Result<Playlist> {
    if name == cfg.default_playlist {
        return Ok(Playlist {
            name: cfg.default_playlist.clone(),
            tracks: library::scan(cfg)?,
        });
    }

    let mut playlist = Playlist::load(cfg, name)?;
    let original_len = playlist.tracks.len();
    playlist
        .tracks
        .retain(|track| library::resolve_track(cfg, track).is_file());

    if playlist.tracks.len() != original_len && rebuild_if_empty {
        playlist.save(cfg)?;
    }

    Ok(playlist)
}
