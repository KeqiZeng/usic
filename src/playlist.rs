use crate::config::Config;
use anyhow::{Context, Result};
use std::fs;
use std::path::PathBuf;

#[derive(Debug, Clone)]
pub struct Playlist {
    pub name: String,
    pub tracks: Vec<String>,
}

impl Playlist {
    pub fn load(cfg: &Config, name: &str) -> Result<Self> {
        let path = playlist_path(cfg, name);
        if !path.exists() {
            return Ok(Self {
                name: name.to_string(),
                tracks: Vec::new(),
            });
        }
        let data = fs::read_to_string(&path)
            .with_context(|| format!("failed to read {}", path.display()))?;
        let tracks = data
            .lines()
            .map(str::trim)
            .filter(|line| !line.is_empty() && !line.starts_with('#'))
            .map(ToOwned::to_owned)
            .collect();
        Ok(Self {
            name: name.to_string(),
            tracks,
        })
    }

    pub fn save(&self, cfg: &Config) -> Result<()> {
        fs::create_dir_all(&cfg.playlist_dir)
            .with_context(|| format!("failed to create {}", cfg.playlist_dir.display()))?;
        let path = playlist_path(cfg, &self.name);
        let data = self.tracks.join("\n") + if self.tracks.is_empty() { "" } else { "\n" };
        fs::write(&path, data).with_context(|| format!("failed to write {}", path.display()))
    }

    pub fn add(&mut self, track: String) {
        if !self.tracks.iter().any(|existing| existing == &track) {
            self.tracks.push(track);
        }
    }

    pub fn remove(&mut self, track: &str) -> bool {
        let original_len = self.tracks.len();
        self.tracks.retain(|existing| existing != track);
        self.tracks.len() != original_len
    }
}

pub fn playlist_path(cfg: &Config, name: &str) -> PathBuf {
    cfg.playlist_dir.join(name)
}

pub fn list_names(cfg: &Config) -> Result<Vec<String>> {
    let mut names = vec![cfg.default_playlist.clone()];
    if !cfg.playlist_dir.exists() {
        return Ok(names);
    }

    for entry in fs::read_dir(&cfg.playlist_dir)
        .with_context(|| format!("failed to read {}", cfg.playlist_dir.display()))?
    {
        let entry = entry?;
        if !entry.file_type()?.is_file() {
            continue;
        }
        let name = entry.file_name().to_string_lossy().into_owned();
        if name != cfg.default_playlist {
            names.push(name);
        }
    }
    names[1..].sort();
    Ok(names)
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::config::PlayMode;

    #[test]
    fn saves_loads_and_removes_tracks() {
        let root = std::env::temp_dir().join(format!("usic_playlist_test_{}", std::process::id()));
        let _ = fs::remove_dir_all(&root);
        let cfg = Config {
            music_dir: root.join("music"),
            playlist_dir: root.join("playlists"),
            default_playlist: "default.txt".to_string(),
            default_mode: PlayMode::Sequence,
            volume_step: 0.1,
            seek_step_secs: 10,
            scan_extensions: vec!["mp3".to_string()],
            socket_path: root.join("usic.sock"),
        };

        let mut playlist = Playlist {
            name: "test.txt".to_string(),
            tracks: Vec::new(),
        };
        playlist.add("a.mp3".to_string());
        playlist.add("a.mp3".to_string());
        playlist.add("b.mp3".to_string());
        playlist.save(&cfg).unwrap();

        let mut loaded = Playlist::load(&cfg, "test.txt").unwrap();
        assert_eq!(loaded.tracks, vec!["a.mp3", "b.mp3"]);
        assert!(loaded.remove("a.mp3"));
        assert!(!loaded.remove("missing.mp3"));

        let _ = fs::remove_dir_all(&root);
    }
}
