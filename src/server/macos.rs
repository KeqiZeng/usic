use std::path::Path;

#[cfg(target_os = "macos")]
use std::process::{Child, Command, Stdio};

#[cfg(target_os = "macos")]
pub(super) struct MacosMediaBridge {
    child: Child,
}

#[cfg(target_os = "macos")]
impl Drop for MacosMediaBridge {
    fn drop(&mut self) {
        if matches!(self.child.try_wait(), Ok(Some(_))) {
            return;
        }
        let _ = self.child.kill();
        let _ = self.child.wait();
    }
}

#[cfg(target_os = "macos")]
pub(super) fn start_macos_media_bridge(socket_path: &Path) -> Option<MacosMediaBridge> {
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
        Ok(child) => Some(MacosMediaBridge { child }),
        Err(err) => {
            eprintln!("failed to start usic-macos-media: {err}");
            None
        }
    }
}

#[cfg(not(target_os = "macos"))]
pub(super) fn start_macos_media_bridge(_socket_path: &Path) -> Option<()> {
    None
}
