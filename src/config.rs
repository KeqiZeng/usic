use anyhow::{Context, Result};
use serde::{Deserialize, Serialize};
use std::env;
use std::fs;
use std::path::{Path, PathBuf};

pub const ALL_PLAYLIST: &str = "All";

#[derive(Debug, Clone, Copy, Serialize, Deserialize, PartialEq, Eq)]
#[serde(rename_all = "snake_case")]
pub enum PlayMode {
    Sequence,
    Shuffle,
    Single,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Config {
    pub music_dir: PathBuf,
    #[serde(skip)]
    pub playlist_dir: PathBuf,
    pub default_playlist: String,
    pub default_mode: PlayMode,
    pub volume_step: f32,
    pub seek_step_secs: u64,
    pub scan_extensions: Vec<String>,
    pub socket_path: PathBuf,
}

impl Config {
    pub fn load_or_create() -> Result<Self> {
        let path = config_path()?;
        if !path.exists() {
            let cfg = Self::default_paths()?;
            if let Some(parent) = path.parent() {
                fs::create_dir_all(parent)
                    .with_context(|| format!("failed to create {}", parent.display()))?;
            }
            fs::write(&path, toml::to_string_pretty(&cfg)?)
                .with_context(|| format!("failed to write {}", path.display()))?;
            return Ok(cfg);
        }

        let data = fs::read_to_string(&path)
            .with_context(|| format!("failed to read {}", path.display()))?;
        let mut cfg: Config =
            toml::from_str(&data).with_context(|| format!("failed to parse {}", path.display()))?;
        cfg.normalize();
        Ok(cfg)
    }

    pub fn default_paths() -> Result<Self> {
        let state = state_dir()?;
        let music_dir = dirs::audio_dir()
            .or_else(dirs::home_dir)
            .unwrap_or_else(|| PathBuf::from("."));
        let playlist_dir = music_dir.join("playlists");
        Ok(Self {
            music_dir,
            playlist_dir,
            default_playlist: ALL_PLAYLIST.to_string(),
            default_mode: PlayMode::Sequence,
            volume_step: 0.1,
            seek_step_secs: 10,
            scan_extensions: ["mp3", "flac", "wav", "ogg", "m4a"]
                .iter()
                .map(|s| s.to_string())
                .collect(),
            socket_path: state.join("usic.sock"),
        })
    }

    pub fn ensure_dirs(&self) -> Result<()> {
        fs::create_dir_all(&self.playlist_dir)
            .with_context(|| format!("failed to create {}", self.playlist_dir.display()))?;
        if let Some(parent) = self.socket_path.parent() {
            fs::create_dir_all(parent)
                .with_context(|| format!("failed to create {}", parent.display()))?;
        }
        Ok(())
    }

    pub fn config_path() -> Result<PathBuf> {
        config_path()
    }

    fn normalize(&mut self) {
        self.music_dir = expand_tilde(&self.music_dir);
        self.playlist_dir = self.music_dir.join("playlists");
        if self.default_playlist == "default.txt" {
            self.default_playlist = ALL_PLAYLIST.to_string();
        }
        self.socket_path = expand_tilde(&self.socket_path);
        self.scan_extensions = self
            .scan_extensions
            .iter()
            .map(|ext| ext.trim_start_matches('.').to_ascii_lowercase())
            .filter(|ext| !ext.is_empty())
            .collect();
    }
}

pub fn config_path() -> Result<PathBuf> {
    let base = match env::var_os("XDG_CONFIG_HOME") {
        Some(path) => PathBuf::from(path),
        None => dirs::home_dir()
            .map(|home| home.join(".config"))
            .context("failed to resolve config directory")?,
    };
    Ok(base.join("usic").join("config.toml"))
}

pub fn state_dir() -> Result<PathBuf> {
    let base = match env::var_os("XDG_STATE_HOME") {
        Some(path) => PathBuf::from(path),
        None => dirs::home_dir()
            .map(|home| home.join(".local").join("state"))
            .context("failed to resolve state directory")?,
    };
    Ok(base.join("usic"))
}

fn expand_tilde(path: &Path) -> PathBuf {
    let s = path.to_string_lossy();
    if s == "~" {
        return dirs::home_dir().unwrap_or_else(|| path.to_path_buf());
    }
    if let Some(rest) = s.strip_prefix("~/") {
        if let Some(home) = dirs::home_dir() {
            return home.join(rest);
        }
    }
    path.to_path_buf()
}
