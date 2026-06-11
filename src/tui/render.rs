use super::{list::highlighted_line, App, UiMode};
use crate::config::PlayMode;
use crate::time;
use ratatui::layout::{Constraint, Direction, Layout};
use ratatui::style::{Color, Modifier, Style, Stylize};
use ratatui::text::{Line, Span};
use ratatui::widgets::{Block, Borders, Gauge, List, ListItem, Paragraph};
use ratatui::Frame;

impl App {
    pub(super) fn render(&mut self, frame: &mut Frame) {
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
                UiMode::Normal => "Queue",
            })),
            chunks[3],
            &mut self.list_state,
        );

        let prompt = match self.mode {
            UiMode::Normal if self.is_all_playlist() => {
                "[space] pause | [n/p] next/prev | [gg/G] top/bottom | [/] search | [l] later | [c] create | [o] playlists | [s] mode | [q] quit"
            }
            UiMode::Normal => {
                "[space] pause | [n/p] next/prev | [gg/G] top/bottom | [/] search | [a/r] add/remove | [l] later | [c] create | [o] playlists | [s] mode | [q] quit"
            }
            UiMode::SearchPlay => "search: type to filter, [enter] play, [tab] later",
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
