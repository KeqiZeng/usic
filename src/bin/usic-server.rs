use anyhow::{bail, Context, Result};
use rand::seq::SliceRandom;
use rodio::{Decoder, OutputStream, OutputStreamBuilder, Sink, Source};
use std::fs::{self, File};
use std::io::{BufRead, BufReader, Write};
use std::os::unix::net::{UnixListener, UnixStream};
use std::path::Path;
#[cfg(target_os = "macos")]
use std::process::{Child, Command, Stdio};
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
    let _macos_media = start_macos_media_bridge(&cfg.socket_path);

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
    play_order: Vec<String>,
    current_order_index: Option<usize>,
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
        let play_order = build_play_order(&playlist.tracks, cfg.default_mode);
        sink.set_volume(1.0);
        Ok(Self {
            cfg: cfg.clone(),
            event_tx,
            _stream: stream,
            sink,
            playlist,
            play_order,
            current_order_index: None,
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
            Request::SetPaused { paused } => {
                self.set_paused(paused);
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
                self.rebuild_play_order();
                ipc::ok_empty()
            }
            Request::LoadPlaylist { name } => {
                self.playlist =
                    load_runtime_playlist(&self.cfg, &name, name == self.cfg.default_playlist)?;
                self.rebuild_play_order();
                ipc::ok_empty()
            }
            Request::GetStatus => Response::Ok(ResponseData::Status(self.status())),
            Request::GetPlaylist => Response::Ok(ResponseData::Playlist(self.queue_view())),
            Request::Quit => return Ok((ipc::ok_message("server quit successfully"), true)),
        };
        Ok((response, false))
    }

    fn play_requested(&mut self, track: Option<String>) -> Result<()> {
        let track = match track {
            Some(track) => track,
            None => self
                .current_order_index
                .and_then(|idx| self.play_order.get(idx).cloned())
                .or_else(|| self.play_order.first().cloned())
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
        self.current_order_index = self.ensure_in_play_order(&track);
        self.current_track = Some(track);
        Ok(())
    }

    fn play_next(&mut self) -> Result<()> {
        let Some(index) = self.next_index() else {
            bail!("queue is empty");
        };
        let track = self.play_order[index].clone();
        self.current_order_index = Some(index);
        self.play_track(track)
    }

    fn play_prev(&mut self) -> Result<()> {
        if self.play_order.is_empty() {
            bail!("queue is empty");
        }
        let index = match self.current_order_index {
            Some(0) | None => self.play_order.len() - 1,
            Some(index) => index - 1,
        };
        let track = self.play_order[index].clone();
        self.current_order_index = Some(index);
        self.play_track(track)
    }

    fn play_later(&mut self, track: String) {
        if self.current_track.as_deref() == Some(track.as_str()) {
            return;
        }

        let mut current = self.current_order_index;
        if let Some(existing) = self
            .play_order
            .iter()
            .position(|candidate| candidate == &track)
        {
            self.play_order.remove(existing);
            if let Some(index) = current {
                if existing < index {
                    current = Some(index - 1);
                }
            }
        }

        let insert_at = current
            .map(|idx| idx + 1)
            .unwrap_or(self.play_order.len())
            .min(self.play_order.len());
        self.play_order.insert(insert_at, track);
        self.current_order_index = current;
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
        self.set_paused(!self.paused);
    }

    fn set_paused(&mut self, paused: bool) {
        self.paused = paused;
        if paused {
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
        if self.play_order.is_empty() {
            return None;
        }
        match self.mode {
            PlayMode::Single => Some(self.current_order_index.unwrap_or(0)),
            PlayMode::Sequence => Some(
                self.current_order_index
                    .map(|idx| (idx + 1) % self.play_order.len())
                    .unwrap_or(0),
            ),
            PlayMode::Shuffle => Some(
                self.current_order_index
                    .map(|idx| (idx + 1) % self.play_order.len())
                    .unwrap_or(0),
            ),
        }
    }

    fn rebuild_play_order(&mut self) {
        self.play_order = build_play_order(&self.playlist.tracks, self.mode);
        self.current_order_index = self.current_track.as_ref().and_then(|track| {
            self.play_order
                .iter()
                .position(|candidate| candidate == track)
        });
        if self.current_order_index.is_none() {
            if let Some(track) = &self.current_track {
                self.play_order.insert(0, track.clone());
                self.current_order_index = Some(0);
            }
        }
    }

    fn ensure_in_play_order(&mut self, track: &str) -> Option<usize> {
        if let Some(index) = self
            .play_order
            .iter()
            .position(|candidate| candidate == track)
        {
            return Some(index);
        }

        let insert_at = self
            .current_order_index
            .map(|idx| idx + 1)
            .unwrap_or(self.play_order.len())
            .min(self.play_order.len());
        self.play_order.insert(insert_at, track.to_string());
        Some(insert_at)
    }

    fn queue_view(&self) -> Vec<String> {
        let Some(index) = self
            .current_order_index
            .filter(|index| *index < self.play_order.len())
        else {
            return self.play_order.clone();
        };

        self.play_order[index..]
            .iter()
            .chain(self.play_order[..index].iter())
            .cloned()
            .collect()
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
            queue_len: self.play_order.len(),
        }
    }
}

fn build_play_order(tracks: &[String], mode: PlayMode) -> Vec<String> {
    let mut order = tracks.to_vec();
    if mode == PlayMode::Shuffle {
        order.shuffle(&mut rand::thread_rng());
    }
    order
}

#[cfg(target_os = "macos")]
fn start_macos_media_bridge(socket_path: &Path) -> Option<Child> {
    let helper = std::env::current_exe()
        .ok()
        .map(|path| path.with_file_name("usic-macos-media"))
        .filter(|path| path.is_file());
    let mut command = match helper {
        Some(path) => Command::new(path),
        None => Command::new("usic-macos-media"),
    };
    match command
        .arg(socket_path)
        .stdin(Stdio::null())
        .stdout(Stdio::null())
        .stderr(Stdio::null())
        .spawn()
    {
        Ok(child) => Some(child),
        Err(err) => {
            eprintln!("failed to start usic-macos-media: {err}");
            None
        }
    }
}

#[cfg(not(target_os = "macos"))]
fn start_macos_media_bridge(_socket_path: &Path) -> Option<()> {
    None
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
