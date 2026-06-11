use crate::config::{Config, PlayMode};
use crate::fuzzy;
use crate::ipc::{self, Request, Response, ResponseData, Status};
use crate::library;
use crate::playlist::{self, Playlist};
use crate::server_control;
use anyhow::Result;
use crossterm::event::{self, Event, KeyEventKind};
use ratatui::text::Line;
use ratatui::widgets::ListState;
use std::collections::BTreeSet;
use std::fs;
use std::time::{Duration, Instant};

mod input;
mod list;
mod render;

use list::{highlighted_line, marked_line};

pub fn run() -> Result<()> {
    if std::env::args().any(|arg| arg == "--help" || arg == "-h") {
        println!("Usage: usic tui\n\nTerminal client for usic.");
        return Ok(());
    }
    let cfg = Config::load_or_create()?;
    cfg.ensure_dirs()?;
    server_control::ensure_server_running(&cfg)?;
    let terminal = ratatui::init();
    let result = App::new(cfg)?.run(terminal);
    ratatui::restore();
    result
}

struct App {
    cfg: Config,
    status: Option<Status>,
    status_updated_at: Option<Instant>,
    playlist: Vec<String>,
    queue: Vec<String>,
    playlists: Vec<String>,
    library: Vec<String>,
    visible: Vec<VisibleItem>,
    list_state: ListState,
    playlist_state: ListState,
    marked: BTreeSet<String>,
    selected: usize,
    playlist_selected: usize,
    mode: UiMode,
    input: String,
    message: String,
    pending_delete_playlist: Option<String>,
    pending_queue_top: bool,
}

#[derive(Clone)]
struct VisibleItem {
    value: String,
    line: Line<'static>,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
enum UiMode {
    Normal,
    SearchPlay,
    PlaylistSelect,
    AddSelect,
    RemoveSelect,
    CreateSelect,
    CreatePlaylistName,
}

impl App {
    fn new(cfg: Config) -> Result<Self> {
        let mut app = Self {
            library: library::scan(&cfg)?,
            cfg,
            status: None,
            status_updated_at: None,
            playlist: Vec::new(),
            queue: Vec::new(),
            playlists: Vec::new(),
            visible: Vec::new(),
            list_state: ListState::default(),
            playlist_state: ListState::default(),
            marked: BTreeSet::new(),
            selected: 0,
            playlist_selected: 0,
            mode: UiMode::Normal,
            input: String::new(),
            message: String::new(),
            pending_delete_playlist: None,
            pending_queue_top: false,
        };
        app.rebuild_visible();
        Ok(app)
    }

    fn run(&mut self, mut terminal: ratatui::DefaultTerminal) -> Result<()> {
        self.refresh();
        let mut next_status_refresh = Instant::now() + Duration::from_secs(1);
        let mut next_playlist_refresh = Instant::now() + Duration::from_secs(2);
        let mut next_draw = Instant::now() + Duration::from_secs(1);
        let mut dirty = true;

        loop {
            if dirty {
                terminal.draw(|frame| self.render(frame))?;
                dirty = false;
            }

            let now = Instant::now();
            let timeout = [next_status_refresh, next_playlist_refresh, next_draw]
                .into_iter()
                .map(|deadline| deadline.saturating_duration_since(now))
                .min()
                .unwrap_or_else(|| Duration::from_millis(250));

            if event::poll(timeout)? {
                if let Event::Key(key) = event::read()? {
                    if key.kind == KeyEventKind::Press {
                        if self.handle_key(key)? {
                            break;
                        }
                        dirty = true;
                    }
                }
            }

            let now = Instant::now();
            if now >= next_status_refresh {
                dirty |= self.refresh_status();
                next_status_refresh = now + Duration::from_secs(1);
            }
            if now >= next_playlist_refresh {
                dirty |= self.refresh_playlists();
                dirty |= self.refresh_queue();
                next_playlist_refresh = now + Duration::from_secs(2);
            }
            if now >= next_draw {
                dirty |= self.should_redraw_progress();
                next_draw = now + Duration::from_secs(1);
            }
        }
        Ok(())
    }

    fn submit_input(&mut self) -> Result<()> {
        let result: Result<bool> = match self.mode {
            UiMode::SearchPlay => {
                if let Some(track) = self.selected_value() {
                    self.send(Request::Play { track: Some(track) });
                }
                Ok(true)
            }
            UiMode::PlaylistSelect => {
                if let Some(name) = self.selected_playlist_name() {
                    self.load_playlist(name);
                }
                Ok(true)
            }
            UiMode::AddSelect | UiMode::RemoveSelect => {
                if self.marked.is_empty() {
                    self.message = "no tracks selected".to_string();
                    Ok(false)
                } else {
                    self.save_marked_to_current_playlist()
                }
            }
            UiMode::CreateSelect => {
                if self.marked.is_empty() {
                    self.message = "no tracks selected".to_string();
                    Ok(false)
                } else {
                    self.input.clear();
                    self.mode = UiMode::CreatePlaylistName;
                    self.rebuild_visible();
                    Ok(false)
                }
            }
            UiMode::CreatePlaylistName => self.save_marked_to_playlist(),
            UiMode::Normal => Ok(false),
        };

        match result {
            Ok(true) => {
                self.enter_mode(UiMode::Normal);
                self.refresh();
            }
            Ok(false) => {}
            Err(err) => self.message = err.to_string(),
        }
        Ok(())
    }

    fn save_marked_to_playlist(&mut self) -> Result<bool> {
        let name = self.input.trim();
        if name.is_empty() {
            self.message = "playlist name is required".to_string();
            return Ok(false);
        }
        if name == self.cfg.default_playlist {
            self.message = format!("{} is managed from music_dir; choose another name", name);
            return Ok(false);
        }

        let mut new_playlist = Playlist {
            name: name.to_string(),
            tracks: Vec::new(),
        };
        for track in &self.marked {
            new_playlist.add(track.clone());
        }
        new_playlist.save(&self.cfg)?;
        self.message = format!("created {} with {} tracks", name, self.marked.len());
        self.marked.clear();
        self.refresh_playlists();
        Ok(true)
    }

    fn save_marked_to_current_playlist(&mut self) -> Result<bool> {
        let Some(name) = self.current_playlist_name() else {
            self.message = "no playlist loaded".to_string();
            return Ok(false);
        };
        if name == self.cfg.default_playlist {
            self.message = "All is managed from music_dir".to_string();
            return Ok(false);
        }

        let mut playlist = Playlist::load(&self.cfg, &name)?;
        let count = self.marked.len();
        let message = match self.mode {
            UiMode::AddSelect => {
                for track in &self.marked {
                    playlist.add(track.clone());
                }
                format!("added {} tracks to {}", count, name)
            }
            UiMode::RemoveSelect => {
                for track in &self.marked {
                    playlist.remove(track);
                }
                format!("removed {} tracks from {}", count, name)
            }
            _ => return Ok(false),
        };
        playlist.save(&self.cfg)?;
        self.marked.clear();
        self.enter_mode(UiMode::Normal);
        self.load_playlist(name.clone());
        self.message = message;
        Ok(false)
    }

    fn send(&mut self, request: Request) {
        let refresh_status = !matches!(request, Request::Quit);
        let refresh_queue = matches!(
            request,
            Request::Play { .. }
                | Request::Next
                | Request::Prev
                | Request::PlayLater { .. }
                | Request::SetMode { .. }
                | Request::LoadPlaylist { .. }
        );
        match ipc::send(&self.cfg.socket_path, &request) {
            Ok(Response::Ok(ResponseData::Message(message))) => {
                self.message = message;
                if refresh_status {
                    self.refresh_status();
                }
                if refresh_queue {
                    self.refresh_queue();
                }
            }
            Ok(Response::Ok(_)) => {
                self.message.clear();
                if refresh_status {
                    self.refresh_status();
                }
                if refresh_queue {
                    self.refresh_queue();
                }
            }
            Ok(Response::Err { message }) => self.message = message,
            Err(err) => self.message = err.to_string(),
        }
    }

    fn load_playlist(&mut self, name: String) {
        self.send(Request::LoadPlaylist { name });
    }

    fn play_selected_later(&mut self) {
        if let Some(track) = self.selected_value() {
            self.send(Request::PlayLater { track });
        } else {
            self.message = "no track selected".to_string();
        }
    }

    fn submit_search_later(&mut self) {
        if let Some(track) = self.selected_value() {
            self.send(Request::PlayLater { track });
            self.enter_mode(UiMode::Normal);
            self.refresh();
        } else {
            self.message = "no track selected".to_string();
        }
    }

    fn delete_selected_playlist(&mut self) {
        let Some(name) = self.selected_playlist_name() else {
            return;
        };
        if name == self.cfg.default_playlist {
            self.pending_delete_playlist = None;
            self.message = "All cannot be deleted".to_string();
            return;
        }

        if self.pending_delete_playlist.as_deref() != Some(name.as_str()) {
            self.pending_delete_playlist = Some(name.clone());
            self.message = format!("press d again to delete {}", name);
            return;
        }

        self.pending_delete_playlist = None;
        let path = playlist::playlist_path(&self.cfg, &name);
        match fs::remove_file(&path) {
            Ok(()) => {
                let was_current = self.current_playlist_name().as_deref() == Some(name.as_str());
                self.refresh_playlists();
                if was_current {
                    self.load_playlist(self.cfg.default_playlist.clone());
                }
                self.message = format!("deleted {}", name);
            }
            Err(err) => self.message = format!("failed to delete {}: {}", name, err),
        }
    }

    fn refresh(&mut self) {
        self.refresh_status();
        self.refresh_playlists();
        self.refresh_queue();
    }

    fn refresh_status(&mut self) -> bool {
        match ipc::send(&self.cfg.socket_path, &Request::GetStatus) {
            Ok(Response::Ok(ResponseData::Status(status))) => {
                let changed = self.status.as_ref() != Some(&status);
                self.status = Some(status);
                self.status_updated_at = Some(Instant::now());
                changed
            }
            Ok(Response::Err { message }) => {
                let changed = self.message != message;
                self.message = message;
                changed
            }
            Err(err) => {
                let message = err.to_string();
                let changed = self.message != message;
                self.message = message;
                changed
            }
            _ => false,
        }
    }

    fn refresh_queue(&mut self) -> bool {
        if let Ok(Response::Ok(ResponseData::Playlist(queue))) =
            ipc::send(&self.cfg.socket_path, &Request::GetPlaylist)
        {
            if self.queue != queue {
                self.queue = queue;
                self.rebuild_visible();
                return true;
            }
        }
        false
    }

    fn refresh_current_playlist(&mut self) -> bool {
        match self.current_playlist_tracks() {
            Ok(playlist) => {
                if self.playlist != playlist {
                    self.playlist = playlist;
                    return true;
                }
                false
            }
            Err(err) => {
                let message = err.to_string();
                let changed = self.message != message;
                self.message = message;
                changed
            }
        }
    }

    fn refresh_playlists(&mut self) -> bool {
        match playlist::list_names(&self.cfg) {
            Ok(names) => {
                let changed = self.playlists != names;
                self.playlists = names;
                let visible_len = self.visible_playlists().len();
                if self.playlist_selected >= visible_len {
                    self.playlist_selected = visible_len.saturating_sub(1);
                }
                if self
                    .pending_delete_playlist
                    .as_ref()
                    .is_some_and(|name| !self.playlists.contains(name))
                {
                    self.pending_delete_playlist = None;
                }
                changed
            }
            Err(err) => {
                let message = err.to_string();
                let changed = self.message != message;
                self.message = message;
                changed
            }
        }
    }

    fn enter_mode(&mut self, mode: UiMode) {
        self.mode = mode;
        self.input.clear();
        self.message.clear();
        self.pending_delete_playlist = None;
        self.pending_queue_top = false;
        self.selected = 0;
        if mode == UiMode::PlaylistSelect {
            self.refresh_playlists();
            self.playlist_selected = 0;
        }
        if matches!(
            mode,
            UiMode::AddSelect | UiMode::RemoveSelect | UiMode::CreateSelect
        ) {
            self.marked.clear();
            self.refresh_current_playlist();
        }
        self.rebuild_visible();
    }

    fn rebuild_visible(&mut self) {
        let values: Vec<String> = match self.mode {
            UiMode::SearchPlay => fuzzy::rank(&self.input, &self.library, 100)
                .into_iter()
                .map(ToOwned::to_owned)
                .collect(),
            UiMode::AddSelect => {
                let existing: BTreeSet<&str> = self.playlist.iter().map(String::as_str).collect();
                let candidates: Vec<String> = self
                    .library
                    .iter()
                    .filter(|track| !existing.contains(track.as_str()))
                    .cloned()
                    .collect();
                fuzzy::rank(&self.input, &candidates, 100)
                    .into_iter()
                    .map(ToOwned::to_owned)
                    .collect()
            }
            UiMode::RemoveSelect => fuzzy::rank(&self.input, &self.playlist, 100)
                .into_iter()
                .map(ToOwned::to_owned)
                .collect(),
            UiMode::CreateSelect => fuzzy::rank(&self.input, &self.playlist, 100)
                .into_iter()
                .map(ToOwned::to_owned)
                .collect(),
            UiMode::CreatePlaylistName => self.marked.iter().cloned().collect(),
            UiMode::Normal | UiMode::PlaylistSelect => self.queue.clone(),
        };

        self.visible = values
            .into_iter()
            .map(|value| {
                let line = match self.mode {
                    UiMode::SearchPlay => highlighted_line(&value, &self.input),
                    UiMode::AddSelect | UiMode::RemoveSelect | UiMode::CreateSelect => {
                        marked_line(&value, &self.input, self.marked.contains(&value))
                    }
                    UiMode::Normal | UiMode::PlaylistSelect => Line::from(value.clone()),
                    UiMode::CreatePlaylistName => marked_line(&value, "", true),
                };
                VisibleItem { value, line }
            })
            .collect();
        if self.selected >= self.visible.len() {
            self.selected = self.visible.len().saturating_sub(1);
        }
        self.list_state.select(if self.visible.is_empty() {
            None
        } else {
            Some(self.selected)
        });
    }

    fn cycle_mode(&mut self) {
        let next = match self.status.as_ref().map(|status| status.mode) {
            Some(PlayMode::Sequence) => PlayMode::Shuffle,
            Some(PlayMode::Shuffle) => PlayMode::Single,
            Some(PlayMode::Single) | None => PlayMode::Sequence,
        };
        self.send(Request::SetMode { mode: next });
        if let Some(status) = &mut self.status {
            status.mode = next;
        }
    }

    fn display_position_secs(&self, status: &Status) -> u64 {
        if status.paused || status.current_track.is_none() {
            return status.position_secs;
        }
        let elapsed = self
            .status_updated_at
            .map(|updated_at| updated_at.elapsed().as_secs())
            .unwrap_or(0);
        let position = status.position_secs.saturating_add(elapsed);
        status
            .duration_secs
            .map(|duration| position.min(duration))
            .unwrap_or(position)
    }

    fn should_redraw_progress(&self) -> bool {
        self.status
            .as_ref()
            .is_some_and(|status| !status.paused && status.current_track.is_some())
    }

    fn selected_value(&self) -> Option<String> {
        self.visible
            .get(self.selected)
            .map(|item| item.value.clone())
    }

    fn current_playlist_name(&self) -> Option<String> {
        self.status
            .as_ref()
            .and_then(|status| status.playlist_name.clone())
    }

    fn is_all_playlist(&self) -> bool {
        self.current_playlist_name().as_deref() == Some(self.cfg.default_playlist.as_str())
    }

    fn current_playlist_tracks(&self) -> Result<Vec<String>> {
        let name = self
            .current_playlist_name()
            .unwrap_or_else(|| self.cfg.default_playlist.clone());
        if name == self.cfg.default_playlist {
            return Ok(self.library.clone());
        }

        let mut playlist = Playlist::load(&self.cfg, &name)?;
        playlist
            .tracks
            .retain(|track| library::resolve_track(&self.cfg, track).is_file());
        Ok(playlist.tracks)
    }

    fn selected_playlist_name(&self) -> Option<String> {
        self.visible_playlists()
            .get(self.playlist_selected)
            .cloned()
    }

    fn visible_playlists(&self) -> Vec<String> {
        if self.mode == UiMode::PlaylistSelect {
            fuzzy::rank(&self.input, &self.playlists, self.playlists.len())
                .into_iter()
                .map(ToOwned::to_owned)
                .collect()
        } else {
            self.playlists.clone()
        }
    }

    fn move_selection(&mut self, delta: isize) {
        if self.visible.is_empty() {
            self.selected = 0;
            return;
        }
        let current = self.selected as isize;
        self.selected = (current + delta).clamp(0, self.visible.len() as isize - 1) as usize;
        self.list_state.select(Some(self.selected));
    }

    fn select_first(&mut self) {
        if !self.visible.is_empty() {
            self.selected = 0;
            self.list_state.select(Some(self.selected));
        }
    }

    fn select_last(&mut self) {
        if !self.visible.is_empty() {
            self.selected = self.visible.len() - 1;
            self.list_state.select(Some(self.selected));
        }
    }

    fn move_playlist_selection(&mut self, delta: isize) {
        let len = self.visible_playlists().len();
        if len == 0 {
            self.playlist_selected = 0;
            return;
        }
        let current = self.playlist_selected as isize;
        self.playlist_selected = (current + delta).clamp(0, len as isize - 1) as usize;
        self.playlist_state.select(Some(self.playlist_selected));
    }

    fn toggle_marked(&mut self) {
        if let Some(track) = self.selected_value() {
            if !self.marked.remove(&track) {
                self.marked.insert(track);
            }
            self.rebuild_visible();
        }
    }
}
