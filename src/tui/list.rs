use ratatui::style::{Color, Modifier, Style};
use ratatui::text::{Line, Span};

pub(super) fn highlighted_line(value: &str, query: &str) -> Line<'static> {
    if query.trim().is_empty() {
        return Line::from(value.to_string());
    }

    let mut spans = Vec::new();
    let mut query_chars = query
        .chars()
        .filter(|c| !c.is_whitespace())
        .map(|c| c.to_ascii_lowercase());
    let mut target = query_chars.next();
    for ch in value.chars() {
        if target.is_some_and(|target| ch.to_ascii_lowercase() == target) {
            spans.push(Span::styled(
                ch.to_string(),
                Style::default()
                    .fg(Color::LightYellow)
                    .add_modifier(Modifier::BOLD),
            ));
            target = query_chars.next();
        } else {
            spans.push(Span::styled(
                ch.to_string(),
                Style::default().fg(Color::Gray),
            ));
        }
    }
    Line::from(spans)
}

pub(super) fn marked_line(value: &str, query: &str, marked: bool) -> Line<'static> {
    let mut line = highlighted_line(value, query);
    let marker = if marked { "[x] " } else { "[ ] " };
    line.spans.insert(
        0,
        Span::styled(
            marker,
            Style::default()
                .fg(if marked {
                    Color::LightGreen
                } else {
                    Color::DarkGray
                })
                .add_modifier(Modifier::BOLD),
        ),
    );
    line
}
