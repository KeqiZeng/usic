use nucleo_matcher::pattern::{CaseMatching, Normalization, Pattern};
use nucleo_matcher::{Config, Matcher};

pub fn rank<'a>(query: &str, candidates: &'a [String], limit: usize) -> Vec<&'a str> {
    if query.trim().is_empty() {
        return candidates.iter().take(limit).map(String::as_str).collect();
    }

    let mut matcher = Matcher::new(Config::DEFAULT.match_paths());
    let pattern = Pattern::parse(query, CaseMatching::Ignore, Normalization::Smart);
    let mut matches = pattern.match_list(candidates.iter().map(String::as_str), &mut matcher);
    matches.sort_by(|a, b| b.1.cmp(&a.1).then_with(|| a.0.cmp(b.0)));
    matches
        .into_iter()
        .take(limit)
        .map(|(candidate, _)| candidate)
        .collect()
}

pub fn best(query: &str, candidates: &[String]) -> Option<String> {
    rank(query, candidates, 1)
        .into_iter()
        .next()
        .map(ToOwned::to_owned)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn returns_ranked_matches() {
        let candidates = vec![
            "artist/foo.mp3".to_string(),
            "bar/song.flac".to_string(),
            "foo/bar.wav".to_string(),
        ];
        let matches = rank("foo", &candidates, 2);
        assert_eq!(matches.len(), 2);
        assert!(matches.iter().all(|track| track.contains("foo")));
    }
}
