use crate::config::Config;
use crate::ipc::{self, Request, Response};
use anyhow::{bail, Context, Result};
use std::process::{Command, Stdio};
use std::thread;
use std::time::{Duration, Instant};

const START_TIMEOUT: Duration = Duration::from_secs(2);
const START_POLL_INTERVAL: Duration = Duration::from_millis(50);

pub fn server_is_running(cfg: &Config) -> bool {
    matches!(
        ipc::send(&cfg.socket_path, &Request::GetStatus),
        Ok(Response::Ok(_))
    )
}

pub fn ensure_server_running(cfg: &Config) -> Result<bool> {
    if server_is_running(cfg) {
        return Ok(false);
    }
    start_server(cfg)
}

pub fn start_server(cfg: &Config) -> Result<bool> {
    if server_is_running(cfg) {
        return Ok(false);
    }

    let exe = std::env::current_exe()?;
    let mut child = Command::new(exe)
        .arg("__server")
        .stdin(Stdio::null())
        .stdout(Stdio::null())
        .stderr(Stdio::null())
        .spawn()
        .context("failed to start usic server")?;

    let deadline = Instant::now() + START_TIMEOUT;
    while Instant::now() < deadline {
        if server_is_running(cfg) {
            return Ok(true);
        }
        if let Some(status) = child.try_wait()? {
            bail!("usic server exited before becoming ready: {status}");
        }
        thread::sleep(START_POLL_INTERVAL);
    }

    bail!("usic server did not become ready within 2 seconds")
}
