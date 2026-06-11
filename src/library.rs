use crate::config::Config;
use anyhow::{Context, Result};
use std::path::{Path, PathBuf};
use walkdir::WalkDir;

pub fn scan(cfg: &Config) -> Result<Vec<String>> {
    if !cfg.music_dir.exists() {
        return Ok(Vec::new());
    }

    let mut tracks = Vec::new();
    for entry in WalkDir::new(&cfg.music_dir).follow_links(true) {
        let entry = entry.with_context(|| format!("failed to scan {}", cfg.music_dir.display()))?;
        if !entry.file_type().is_file() {
            continue;
        }
        let path = entry.path();
        let Some(ext) = path.extension().and_then(|ext| ext.to_str()) else {
            continue;
        };
        if !cfg
            .scan_extensions
            .iter()
            .any(|allowed| allowed.eq_ignore_ascii_case(ext))
        {
            continue;
        }
        tracks.push(relative_track(&cfg.music_dir, path));
    }
    tracks.sort_by_key(|track| track.to_ascii_lowercase());
    Ok(tracks)
}

pub fn resolve_track(cfg: &Config, track: &str) -> PathBuf {
    cfg.music_dir.join(track)
}

fn relative_track(root: &Path, path: &Path) -> String {
    path.strip_prefix(root)
        .unwrap_or(path)
        .to_string_lossy()
        .replace('\\', "/")
}
