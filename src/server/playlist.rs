use crate::config::Config;
use crate::library;
use crate::playlist::Playlist;
use anyhow::Result;

pub(super) fn load_runtime_playlist(
    cfg: &Config,
    name: &str,
    rebuild_if_empty: bool,
) -> Result<Playlist> {
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
