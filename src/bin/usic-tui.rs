use anyhow::Result;
use crossterm::event::{self, Event, KeyCode, KeyEvent, KeyEventKind, KeyModifiers};
use ratatui::layout::{Constraint, Direction, Layout};
use ratatui::style::{Color, Modifier, Style, Stylize};
use ratatui::text::{Line, Span};
use ratatui::widgets::{Block, Borders, Gauge, List, ListItem, ListState, Paragraph};
use ratatui::Frame;
use std::collections::BTreeSet;
use std::fs;
use std::time::{Duration, Instant};
use usic::config::{Config, PlayMode};
use usic::fuzzy;
use usic::ipc::{self, Request, Response, ResponseData, SeekTarget, Status};
use usic::library;
use usic::playlist::{self, Playlist};
use usic::time;

fn main() -> Result<()> {
    if std::env::args().any(|arg| arg == "--help" || arg == "-h") {
        println!("Usage: usic-tui\n\nTerminal client for usic-server.");
        return Ok(());
    }
    let cfg = Config::load_or_create()?;
    cfg.ensure_dirs()?;
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
                dirty |= self.refresh_playlist();
                next_playlist_refresh = now + Duration::from_secs(2);
            }
            if now >= next_draw {
                dirty |= self.should_redraw_progress();
                next_draw = now + Duration::from_secs(1);
            }
        }
        Ok(())
    }

    fn render(&mut self, frame: &mut Frame) {
        let area = frame.area();
        let chunks = Layout::default()
            .direction(Direction::Vertical)
            .constraints([
                Constraint::Length(5),
                Constraint::Length(3),
                Constraint::Length(5),
                Constraint::Min(5),
                Constraint::Length(1),
                Constraint::Length(1),
            ])
            .split(area);

        let status_text = if let Some(status) = &self.status {
            let track = status.current_track.as_deref().unwrap_or("-");
            let duration = status
                .duration_secs
                .map(time::format_time)
                .unwrap_or_else(|| "--:--".to_string());
            vec![
                Line::from(vec!["track ".cyan().bold(), Span::raw(track.to_string())]),
                Line::from(vec![
                    "playlist ".cyan().bold(),
                    Span::raw(format!(
                        "{}   ",
                        status.playlist_name.as_deref().unwrap_or("-")
                    )),
                    "time ".cyan().bold(),
                    Span::raw(format!(
                        "{} / {}   ",
                        time::format_time(self.display_position_secs(status)),
                        duration
                    )),
                    "volume ".cyan().bold(),
                    Span::raw(format!(
                        "{:.2}{}   ",
                        status.volume,
                        if status.muted { " muted" } else { "" }
                    )),
                    "mode ".cyan().bold(),
                    mode_span(status.mode),
                    Span::raw("   "),
                    if status.paused {
                        "paused".yellow()
                    } else {
                        "playing".green()
                    },
                ]),
            ]
        } else {
            vec![Line::from("server unavailable".red())]
        };
        frame.render_widget(
            Paragraph::new(status_text)
                .block(styled_block("Status"))
                .style(Style::default().fg(Color::Gray)),
            chunks[0],
        );

        let ratio = self
            .status
            .as_ref()
            .and_then(|status| {
                status
                    .duration_secs
                    .filter(|duration| *duration > 0)
                    .map(|duration| {
                        (self.display_position_secs(status) as f64 / duration as f64)
                            .clamp(0.0, 1.0)
                    })
            })
            .unwrap_or(0.0);
        frame.render_widget(
            Gauge::default()
                .block(styled_block("Progress"))
                .gauge_style(Style::default().fg(Color::LightGreen).bg(Color::DarkGray))
                .ratio(ratio),
            chunks[1],
        );

        let playlist_rows: Vec<ListItem> = self
            .visible_playlists()
            .into_iter()
            .enumerate()
            .map(|(index, name)| {
                let current = self
                    .status
                    .as_ref()
                    .and_then(|status| status.playlist_name.as_deref())
                    == Some(name.as_str());
                let selected =
                    self.mode == UiMode::PlaylistSelect && index == self.playlist_selected;
                let style = if selected {
                    Style::default()
                        .fg(Color::White)
                        .bg(Color::DarkGray)
                        .add_modifier(Modifier::BOLD)
                } else if current {
                    Style::default()
                        .fg(Color::LightGreen)
                        .add_modifier(Modifier::BOLD)
                } else {
                    Style::default().fg(Color::Gray)
                };
                let line = if self.mode == UiMode::PlaylistSelect {
                    highlighted_line(&name, &self.input)
                } else {
                    Line::from(name.clone())
                };
                ListItem::new(line).style(style)
            })
            .collect();
        self.playlist_state.select(
            if self.mode == UiMode::PlaylistSelect && !playlist_rows.is_empty() {
                Some(self.playlist_selected)
            } else {
                None
            },
        );
        frame.render_stateful_widget(
            List::new(playlist_rows).block(styled_block("Playlists")),
            chunks[2],
            &mut self.playlist_state,
        );

        let rows: Vec<ListItem> = self
            .visible
            .iter()
            .enumerate()
            .map(|(index, item)| {
                let style = if index == self.selected {
                    Style::default()
                        .fg(Color::White)
                        .bg(Color::DarkGray)
                        .add_modifier(Modifier::BOLD)
                } else {
                    Style::default().fg(Color::Gray)
                };
                ListItem::new(item.line.clone()).style(style)
            })
            .collect();
        self.list_state.select(if self.visible.is_empty() {
            None
        } else {
            Some(self.selected)
        });
        frame.render_stateful_widget(
            List::new(rows).block(styled_block(match self.mode {
                UiMode::SearchPlay => "Fuzzy Results",
                UiMode::PlaylistSelect => "Playlist",
                UiMode::AddSelect => "Add Tracks",
                UiMode::RemoveSelect => "Remove Tracks",
                UiMode::CreateSelect => "Create Playlist",
                UiMode::CreatePlaylistName => "Selected Tracks",
                UiMode::Normal => "Playlist",
            })),
            chunks[3],
            &mut self.list_state,
        );

        let prompt = match self.mode {
            UiMode::Normal if self.is_all_playlist() => {
                "[space] pause | [n/p] next/prev | [/] search | [l] later | [c] create | [o] playlists | [s] mode | [q] quit"
            }
            UiMode::Normal => {
                "[space] pause | [n/p] next/prev | [/] search | [a/r] add/remove | [l] later | [c] create | [o] playlists | [s] mode | [q] quit"
            }
            UiMode::SearchPlay => {
                "search: type to filter, [enter] play, [tab] later"
            }
            UiMode::PlaylistSelect => {
                "playlists: type to filter, [up/down C-j/C-k] move, [enter] load, [d] delete/confirm"
            }
            UiMode::AddSelect => {
                "add: type to filter, [up/down C-j/C-k] move, [space] mark, [enter] save"
            }
            UiMode::RemoveSelect => {
                "remove: type to filter, [up/down C-j/C-k] move, [space] mark, [enter] save"
            }
            UiMode::CreateSelect => {
                "create: type to filter, [up/down C-j/C-k] move, [space] mark, [enter] name"
            }
            UiMode::CreatePlaylistName => "create playlist",
        };
        let mut detail = if self.mode == UiMode::Normal {
            self.message.clone()
        } else if matches!(
            self.mode,
            UiMode::AddSelect | UiMode::RemoveSelect | UiMode::CreateSelect
        ) {
            format!("{prompt}: {}  ({} selected)", self.input, self.marked.len())
        } else if self.mode == UiMode::CreatePlaylistName {
            format!("{prompt}: {}  ({} selected)", self.input, self.marked.len())
        } else {
            format!("{prompt}: {}", self.input)
        };
        if self.mode != UiMode::Normal && !self.message.is_empty() {
            detail.push_str("  ");
            detail.push_str(&self.message);
        }
        frame.render_widget(
            Paragraph::new(prompt).style(
                Style::default()
                    .fg(Color::LightCyan)
                    .add_modifier(Modifier::BOLD),
            ),
            chunks[4],
        );
        frame.render_widget(
            Paragraph::new(detail).style(Style::default().fg(Color::LightYellow)),
            chunks[5],
        );
    }

    fn handle_key(&mut self, key: KeyEvent) -> Result<bool> {
        match self.mode {
            UiMode::Normal => self.handle_normal_key(key.code),
            UiMode::SearchPlay
            | UiMode::AddSelect
            | UiMode::RemoveSelect
            | UiMode::CreateSelect => self.handle_filter_key(key),
            UiMode::PlaylistSelect => self.handle_playlist_select_key(key),
            UiMode::CreatePlaylistName => self.handle_playlist_name_key(key.code),
        }
    }

    fn handle_normal_key(&mut self, code: KeyCode) -> Result<bool> {
        match code {
            KeyCode::Char('q') => return Ok(true),
            KeyCode::Char(' ') => self.send(Request::TogglePause),
            KeyCode::Char('n') => self.send(Request::Next),
            KeyCode::Char('p') => self.send(Request::Prev),
            KeyCode::Char('+') | KeyCode::Char('=') => self.send(Request::VolumeUp),
            KeyCode::Char('-') => self.send(Request::VolumeDown),
            KeyCode::Char('m') => self.send(Request::ToggleMute),
            KeyCode::Right => self.send(Request::Seek {
                target: SeekTarget::Relative {
                    seconds: self.cfg.seek_step_secs as i64,
                },
            }),
            KeyCode::Left => self.send(Request::Seek {
                target: SeekTarget::Relative {
                    seconds: -(self.cfg.seek_step_secs as i64),
                },
            }),
            KeyCode::Char('s') => self.cycle_mode(),
            KeyCode::Down | KeyCode::Char('j') => self.move_selection(1),
            KeyCode::Up | KeyCode::Char('k') => self.move_selection(-1),
            KeyCode::Enter => {
                if let Some(track) = self.selected_value() {
                    self.send(Request::Play { track: Some(track) });
                }
            }
            KeyCode::Char('/') => self.enter_mode(UiMode::SearchPlay),
            KeyCode::Char('l') => self.play_selected_later(),
            KeyCode::Char('c') => self.enter_mode(UiMode::CreateSelect),
            KeyCode::Char('o') => self.enter_mode(UiMode::PlaylistSelect),
            KeyCode::Char('a') if !self.is_all_playlist() => self.enter_mode(UiMode::AddSelect),
            KeyCode::Char('r') if !self.is_all_playlist() => self.enter_mode(UiMode::RemoveSelect),
            _ => {}
        }
        Ok(false)
    }

    fn handle_filter_key(&mut self, key: KeyEvent) -> Result<bool> {
        let ctrl = key.modifiers.contains(KeyModifiers::CONTROL);
        match key.code {
            KeyCode::Esc => self.enter_mode(UiMode::Normal),
            KeyCode::Backspace => {
                self.message.clear();
                self.input.pop();
                self.selected = 0;
                self.rebuild_visible();
            }
            KeyCode::Tab if self.mode == UiMode::SearchPlay => {
                self.submit_search_later();
            }
            KeyCode::Enter => {
                if let Err(err) = self.submit_input() {
                    self.message = err.to_string();
                }
            }
            KeyCode::Char(' ')
                if matches!(
                    self.mode,
                    UiMode::AddSelect | UiMode::RemoveSelect | UiMode::CreateSelect
                ) =>
            {
                self.toggle_marked();
            }
            KeyCode::Down => self.move_selection(1),
            KeyCode::Up => self.move_selection(-1),
            KeyCode::Char('j') | KeyCode::Char('n') if ctrl => self.move_selection(1),
            KeyCode::Char('k') | KeyCode::Char('p') if ctrl => self.move_selection(-1),
            KeyCode::Char(c) => {
                self.message.clear();
                self.input.push(c);
                self.selected = 0;
                self.rebuild_visible();
            }
            _ => {}
        }
        Ok(false)
    }

    fn handle_playlist_select_key(&mut self, key: KeyEvent) -> Result<bool> {
        let ctrl = key.modifiers.contains(KeyModifiers::CONTROL);
        match key.code {
            KeyCode::Esc => self.enter_mode(UiMode::Normal),
            KeyCode::Backspace => {
                self.pending_delete_playlist = None;
                self.message.clear();
                self.input.pop();
                self.playlist_selected = 0;
            }
            KeyCode::Enter => {
                self.pending_delete_playlist = None;
                if let Some(name) = self.selected_playlist_name() {
                    self.load_playlist(name);
                    self.enter_mode(UiMode::Normal);
                    self.refresh();
                }
            }
            KeyCode::Char('d') => self.delete_selected_playlist(),
            KeyCode::Down => {
                self.pending_delete_playlist = None;
                self.move_playlist_selection(1);
            }
            KeyCode::Up => {
                self.pending_delete_playlist = None;
                self.move_playlist_selection(-1);
            }
            KeyCode::Char('j') | KeyCode::Char('n') if ctrl => {
                self.pending_delete_playlist = None;
                self.move_playlist_selection(1);
            }
            KeyCode::Char('k') | KeyCode::Char('p') if ctrl => {
                self.pending_delete_playlist = None;
                self.move_playlist_selection(-1);
            }
            KeyCode::Char(c) => {
                self.pending_delete_playlist = None;
                self.message.clear();
                self.input.push(c);
                self.playlist_selected = 0;
            }
            _ => {}
        }
        Ok(false)
    }

    fn handle_playlist_name_key(&mut self, code: KeyCode) -> Result<bool> {
        match code {
            KeyCode::Esc => self.enter_mode(UiMode::Normal),
            KeyCode::Backspace => {
                self.message.clear();
                self.input.pop();
            }
            KeyCode::Enter => match self.save_marked_to_playlist() {
                Ok(true) => {
                    self.enter_mode(UiMode::Normal);
                    self.refresh();
                }
                Ok(false) => {}
                Err(err) => self.message = err.to_string(),
            },
            KeyCode::Char(c) => {
                self.message.clear();
                self.input.push(c);
            }
            _ => {}
        }
        Ok(false)
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
        let refresh_playlist = matches!(
            request,
            Request::LoadPlaylist { .. } | Request::PlayLater { .. }
        );
        match ipc::send(&self.cfg.socket_path, &request) {
            Ok(Response::Ok(ResponseData::Message(message))) => {
                self.message = message;
                if refresh_status {
                    self.refresh_status();
                }
                if refresh_playlist {
                    self.refresh_playlist();
                }
            }
            Ok(Response::Ok(_)) => {
                self.message.clear();
                if refresh_status {
                    self.refresh_status();
                }
                if refresh_playlist {
                    self.refresh_playlist();
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
        self.refresh_playlist();
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

    fn refresh_playlist(&mut self) -> bool {
        if let Ok(Response::Ok(ResponseData::Playlist(playlist))) =
            ipc::send(&self.cfg.socket_path, &Request::GetPlaylist)
        {
            if self.playlist != playlist {
                self.playlist = playlist;
                self.rebuild_visible();
                return true;
            }
        }
        false
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
            UiMode::Normal | UiMode::PlaylistSelect => self.playlist.clone(),
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
        self.selected = (current + delta).rem_euclid(self.visible.len() as isize) as usize;
        self.list_state.select(Some(self.selected));
    }

    fn move_playlist_selection(&mut self, delta: isize) {
        let len = self.visible_playlists().len();
        if len == 0 {
            self.playlist_selected = 0;
            return;
        }
        let current = self.playlist_selected as isize;
        self.playlist_selected = (current + delta).rem_euclid(len as isize) as usize;
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

fn styled_block(title: &'static str) -> Block<'static> {
    Block::default()
        .title(title)
        .borders(Borders::ALL)
        .border_style(Style::default().fg(Color::DarkGray))
        .title_style(
            Style::default()
                .fg(Color::LightCyan)
                .add_modifier(Modifier::BOLD),
        )
}

fn mode_span(mode: PlayMode) -> Span<'static> {
    match mode {
        PlayMode::Sequence => "sequence".light_blue(),
        PlayMode::Shuffle => "shuffle".magenta(),
        PlayMode::Single => "single".light_green(),
    }
}

fn highlighted_line(value: &str, query: &str) -> Line<'static> {
    if query.trim().is_empty() {
        return Line::from(value.to_string());
    }

    let mut spans = Vec::new();
    let mut query_chars = query
        .chars()
        .filter(|c| !c.is_whitespace())
        .map(|c| c.to_ascii_lowercase());
    let mut target = query_chars.next();
    for ch in value.chars() {
        if target.is_some_and(|target| ch.to_ascii_lowercase() == target) {
            spans.push(Span::styled(
                ch.to_string(),
                Style::default()
                    .fg(Color::LightYellow)
                    .add_modifier(Modifier::BOLD),
            ));
            target = query_chars.next();
        } else {
            spans.push(Span::styled(
                ch.to_string(),
                Style::default().fg(Color::Gray),
            ));
        }
    }
    Line::from(spans)
}

fn marked_line(value: &str, query: &str, marked: bool) -> Line<'static> {
    let mut line = highlighted_line(value, query);
    let marker = if marked { "[x] " } else { "[ ] " };
    line.spans.insert(
        0,
        Span::styled(
            marker,
            Style::default()
                .fg(if marked {
                    Color::LightGreen
                } else {
                    Color::DarkGray
                })
                .add_modifier(Modifier::BOLD),
        ),
    );
    line
}
