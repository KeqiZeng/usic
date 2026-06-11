use anyhow::{bail, Result};

pub fn parse_time(value: &str) -> Result<u64> {
    if let Some((min, sec)) = value.split_once(':') {
        let min: u64 = min.parse()?;
        let sec: u64 = sec.parse()?;
        if sec >= 60 {
            bail!("seconds must be below 60");
        }
        return Ok(min * 60 + sec);
    }
    Ok(value.parse()?)
}

pub fn format_time(seconds: u64) -> String {
    format!("{}:{:02}", seconds / 60, seconds % 60)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn parses_seconds_and_mmss() {
        assert_eq!(parse_time("75").unwrap(), 75);
        assert_eq!(parse_time("1:15").unwrap(), 75);
        assert!(parse_time("1:60").is_err());
    }

    #[test]
    fn formats_mmss() {
        assert_eq!(format_time(75), "1:15");
    }
}
