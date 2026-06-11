use super::{App, UiMode};
use crate::ipc::{Request, SeekTarget};
use anyhow::Result;
use crossterm::event::{KeyCode, KeyEvent, KeyModifiers};

impl App {
    pub(super) fn handle_key(&mut self, key: KeyEvent) -> Result<bool> {
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
        if self.pending_queue_top {
            self.pending_queue_top = false;
            if code == KeyCode::Char('g') {
                self.select_first();
                return Ok(false);
            }
        }

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
            KeyCode::Char('g') => self.pending_queue_top = true,
            KeyCode::Char('G') => self.select_last(),
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
}
