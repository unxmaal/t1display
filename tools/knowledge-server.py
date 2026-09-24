#!/usr/bin/env python3
"""Knowledge MCP Server for t1display.

External knowledge store + behavioral guardrails.
Keeps context free for real-time work. Nudges Claude
to store findings and avoid hyper-focusing.
"""
import json
import sqlite3
import time
from pathlib import Path

from mcp.server.fastmcp import FastMCP

# --- Paths ---
PROJECT_ROOT = Path(__file__).resolve().parent.parent
DB_PATH = PROJECT_ROOT / ".claude" / "knowledge.db"
LOG_PATH = Path("/tmp/nightscout-knowledge-mcp.log")

# --- Logging (file only, never stderr) ---
_log_file = None


def log(msg: str) -> None:
    global _log_file
    try:
        if _log_file is None:
            _log_file = open(LOG_PATH, "a")
        _log_file.write(f"[{time.strftime('%Y-%m-%d %H:%M:%S')}] {msg}\n")
        _log_file.flush()
    except Exception:
        pass


# --- Database ---
_db: sqlite3.Connection | None = None

SCHEMA = """
CREATE TABLE IF NOT EXISTS knowledge (
    id INTEGER PRIMARY KEY,
    topic TEXT NOT NULL,
    summary TEXT NOT NULL,
    detail TEXT,
    source TEXT,
    tags TEXT,
    hit_count INTEGER DEFAULT 0,
    created_at TEXT DEFAULT (datetime('now')),
    updated_at TEXT DEFAULT (datetime('now'))
);

CREATE TABLE IF NOT EXISTS negative_knowledge (
    id INTEGER PRIMARY KEY,
    category TEXT NOT NULL,
    what_failed TEXT NOT NULL,
    why_failed TEXT,
    correct_approach TEXT,
    severity TEXT DEFAULT 'normal',
    created_at TEXT DEFAULT (datetime('now'))
);

CREATE TABLE IF NOT EXISTS project_data (
    id INTEGER PRIMARY KEY,
    category TEXT NOT NULL,
    key TEXT NOT NULL,
    value TEXT NOT NULL,
    metadata TEXT,
    updated_at TEXT DEFAULT (datetime('now'))
);

CREATE TABLE IF NOT EXISTS handoffs (
    id INTEGER PRIMARY KEY,
    status TEXT NOT NULL,
    current_task TEXT,
    findings TEXT,
    next_steps TEXT,
    blockers TEXT,
    context_snapshot TEXT,
    created_at TEXT DEFAULT (datetime('now'))
);

CREATE TABLE IF NOT EXISTS notes (
    id INTEGER PRIMARY KEY,
    topic TEXT NOT NULL,
    content TEXT NOT NULL,
    created_at TEXT DEFAULT (datetime('now'))
);

CREATE VIRTUAL TABLE IF NOT EXISTS knowledge_fts USING fts5(
    topic, summary, detail, tags,
    content='knowledge',
    content_rowid='id'
);

CREATE VIRTUAL TABLE IF NOT EXISTS negative_fts USING fts5(
    category, what_failed, why_failed, correct_approach,
    content='negative_knowledge',
    content_rowid='id'
);

-- Triggers to keep FTS in sync
CREATE TRIGGER IF NOT EXISTS knowledge_ai AFTER INSERT ON knowledge BEGIN
    INSERT INTO knowledge_fts(rowid, topic, summary, detail, tags)
    VALUES (new.id, new.topic, new.summary, new.detail, new.tags);
END;

CREATE TRIGGER IF NOT EXISTS knowledge_ad AFTER DELETE ON knowledge BEGIN
    INSERT INTO knowledge_fts(knowledge_fts, rowid, topic, summary, detail, tags)
    VALUES ('delete', old.id, old.topic, old.summary, old.detail, old.tags);
END;

CREATE TRIGGER IF NOT EXISTS knowledge_au AFTER UPDATE ON knowledge BEGIN
    INSERT INTO knowledge_fts(knowledge_fts, rowid, topic, summary, detail, tags)
    VALUES ('delete', old.id, old.topic, old.summary, old.detail, old.tags);
    INSERT INTO knowledge_fts(rowid, topic, summary, detail, tags)
    VALUES (new.id, new.topic, new.summary, new.detail, new.tags);
END;

CREATE TRIGGER IF NOT EXISTS negative_ai AFTER INSERT ON negative_knowledge BEGIN
    INSERT INTO negative_fts(rowid, category, what_failed, why_failed, correct_approach)
    VALUES (new.id, new.category, new.what_failed, new.why_failed, new.correct_approach);
END;

CREATE TRIGGER IF NOT EXISTS negative_ad AFTER DELETE ON negative_knowledge BEGIN
    INSERT INTO negative_fts(negative_fts, rowid, category, what_failed, why_failed, correct_approach)
    VALUES ('delete', old.id, old.category, old.what_failed, old.why_failed, old.correct_approach);
END;

CREATE TRIGGER IF NOT EXISTS negative_au AFTER UPDATE ON negative_knowledge BEGIN
    INSERT INTO negative_fts(negative_fts, rowid, category, what_failed, why_failed, correct_approach)
    VALUES ('delete', old.id, old.category, old.what_failed, old.why_failed, old.correct_approach);
    INSERT INTO negative_fts(rowid, category, what_failed, why_failed, correct_approach)
    VALUES (new.id, new.category, new.what_failed, new.why_failed, new.correct_approach);
END;
"""


def get_db() -> sqlite3.Connection:
    global _db
    if _db is None:
        DB_PATH.parent.mkdir(parents=True, exist_ok=True)
        _db = sqlite3.connect(str(DB_PATH))
        _db.row_factory = sqlite3.Row
        _db.execute("PRAGMA journal_mode=WAL")
        _db.execute("PRAGMA busy_timeout=5000")
        _db.executescript(SCHEMA)
        log(f"DB initialized at {DB_PATH} (WAL mode)")
    return _db


# --- Behavioral Tracking ---
class SessionTracker:
    def __init__(self) -> None:
        self.turn_count: int = 0
        self.last_store_turn: int = 0
        self.topic_freq: dict[str, int] = {}
        self.session_start: float = time.time()

    def record_call(self, tool_name: str, topic: str | None = None) -> None:
        self.turn_count += 1
        if topic:
            key = topic.lower().strip()
            self.topic_freq[key] = self.topic_freq.get(key, 0) + 1
        log(f"Turn {self.turn_count}: {tool_name} topic={topic}")

    def record_store(self) -> None:
        self.last_store_turn = self.turn_count

    def reset_all(self) -> None:
        self.last_store_turn = self.turn_count
        self.topic_freq.clear()

    def get_nudge(self, topic: str | None = None) -> str | None:
        messages: list[str] = []
        turns_since_store = self.turn_count - self.last_store_turn

        # Mandatory stop at 50+
        if self.turn_count >= 50:
            messages.append(
                f"MANDATORY STOP: {self.turn_count} tool calls. You MUST call "
                "`session_handoff` NOW before continuing. Do NOT say "
                "'but first let me...' — that is the failure mode this "
                "checkpoint prevents."
            )
        # Long session checkpoint at 25+
        elif self.turn_count >= 25:
            messages.append(
                f"CHECKPOINT: {self.turn_count} tool calls this session. "
                "Call `session_handoff` to snapshot your current state. "
                "If debugging the same issue for >3 attempts, delegate "
                "to a sub-agent."
            )

        # Periodic capture reminder
        if turns_since_store >= 10:
            messages.append(
                f"REMINDER: You've made {turns_since_store} tool calls "
                "without storing findings. Use `add_knowledge` or "
                "`add_negative` to externalize what you've learned "
                "before it compacts out of context."
            )

        # Hyper-focus detection
        if topic:
            key = topic.lower().strip()
            freq = self.topic_freq.get(key, 0)
            if freq >= 3:
                messages.append(
                    f"WARNING: You've queried '{topic}' {freq} times. "
                    "Either store your findings and move on, or delegate "
                    "to a sub-agent with Task()."
                )

        # Rules check every 15 turns
        if self.turn_count > 0 and self.turn_count % 15 == 0:
            messages.append(
                "RULES CHECK: (1) Are you following CLAUDE.md? "
                "(2) Have you stored findings? "
                "(3) Are you hyper-focused on one approach? "
                "(4) Should you delegate?"
            )

        return "\n\n".join(messages) if messages else None


tracker = SessionTracker()


# --- Helper ---
def with_nudge(result: str, topic: str | None = None) -> str:
    nudge = tracker.get_nudge(topic)
    if nudge:
        return f"{result}\n\n---\n{nudge}"
    return result


# --- FastMCP Server ---
mcp = FastMCP("nightscout-knowledge")


def _sanitize_fts(query: str) -> str:
    """Quote each term so FTS5 special chars (hyphens, colons) don't break queries."""
    terms = query.split()
    return " ".join(f'"{t}"' for t in terms)


@mcp.tool()
def search_knowledge(query: str) -> str:
    """Search across all knowledge, negative knowledge, and project data.

    Uses FTS5 full-text search for fast, relevant results.
    """
    tracker.record_call("search_knowledge", topic=query)
    db = get_db()
    results: list[str] = []
    fts_query = _sanitize_fts(query)

    # Search knowledge FTS
    rows = db.execute(
        "SELECT k.id, k.topic, k.summary, k.detail, k.tags, k.source "
        "FROM knowledge_fts f JOIN knowledge k ON f.rowid = k.id "
        "WHERE knowledge_fts MATCH ? ORDER BY rank LIMIT 10",
        (fts_query,),
    ).fetchall()
    for r in rows:
        db.execute(
            "UPDATE knowledge SET hit_count = hit_count + 1 WHERE id = ?",
            (r["id"],),
        )
        tags = r["tags"] or ""
        if "rules, pointer" in tags:
            # Extract filename from source field
            src = r["source"] or ""
            fname = src.replace("rules/", "") if src.startswith("rules/") else src
            results.append(
                f'[RULE] {src} — matches "{r["topic"]}". '
                f'Read with get_rule("{fname}")'
            )
        else:
            results.append(
                f"[KNOWLEDGE] {r['topic']}: {r['summary']}"
                + (f"\n  Detail: {r['detail']}" if r["detail"] else "")
                + (f"\n  Tags: {tags}" if tags else "")
            )

    # Search negative knowledge FTS
    rows = db.execute(
        "SELECT n.id, n.category, n.what_failed, n.why_failed, n.correct_approach "
        "FROM negative_fts f JOIN negative_knowledge n ON f.rowid = n.id "
        "WHERE negative_fts MATCH ? ORDER BY rank LIMIT 10",
        (fts_query,),
    ).fetchall()
    for r in rows:
        results.append(
            f"[NEGATIVE] {r['category']}: {r['what_failed']}"
            + (f"\n  Why: {r['why_failed']}" if r["why_failed"] else "")
            + (f"\n  Fix: {r['correct_approach']}" if r["correct_approach"] else "")
        )

    # Search project_data with LIKE fallback (no FTS for project_data)
    like = f"%{query}%"
    rows = db.execute(
        "SELECT category, key, value FROM project_data "
        "WHERE key LIKE ? OR value LIKE ? OR category LIKE ? LIMIT 10",
        (like, like, like),
    ).fetchall()
    for r in rows:
        results.append(f"[PROJECT] {r['category']}/{r['key']}: {r['value']}")

    db.commit()

    if not results:
        return with_nudge(f"No results for '{query}'.", topic=query)
    return with_nudge("\n\n".join(results), topic=query)


@mcp.tool()
def add_knowledge(
    topic: str,
    summary: str,
    detail: str = "",
    source: str = "",
    tags: str = "",
) -> str:
    """Store a learning about this project — patterns, decisions, architecture.

    Resets the behavioral store counter.
    """
    tracker.record_call("add_knowledge", topic=topic)
    tracker.record_store()
    db = get_db()
    db.execute(
        "INSERT INTO knowledge (topic, summary, detail, source, tags) "
        "VALUES (?, ?, ?, ?, ?)",
        (topic, summary, detail, source, tags),
    )
    db.commit()
    return with_nudge(f"Stored knowledge: {topic}")


@mcp.tool()
def add_negative(
    category: str,
    what_failed: str,
    why_failed: str = "",
    correct_approach: str = "",
    severity: str = "normal",
) -> str:
    """Store what doesn't work — mistakes, anti-patterns, dead ends.

    Resets the behavioral store counter.
    """
    tracker.record_call("add_negative", topic=category)
    tracker.record_store()
    db = get_db()
    db.execute(
        "INSERT INTO negative_knowledge "
        "(category, what_failed, why_failed, correct_approach, severity) "
        "VALUES (?, ?, ?, ?, ?)",
        (category, what_failed, why_failed, correct_approach, severity),
    )
    db.commit()
    return with_nudge(f"Stored negative knowledge: {category} — {what_failed}")


@mcp.tool()
def add_project_data(
    category: str,
    key: str,
    value: str,
    metadata: str = "",
) -> str:
    """Store structured project data — pin mappings, config params, hardware specs, build settings.

    Upserts by category+key. Resets the behavioral store counter.
    """
    tracker.record_call("add_project_data", topic=category)
    tracker.record_store()
    db = get_db()
    existing = db.execute(
        "SELECT id FROM project_data WHERE category = ? AND key = ?",
        (category, key),
    ).fetchone()
    if existing:
        db.execute(
            "UPDATE project_data SET value = ?, metadata = ?, "
            "updated_at = datetime('now') WHERE id = ?",
            (value, metadata, existing["id"]),
        )
    else:
        db.execute(
            "INSERT INTO project_data (category, key, value, metadata) "
            "VALUES (?, ?, ?, ?)",
            (category, key, value, metadata),
        )
    db.commit()
    action = "Updated" if existing else "Stored"
    return with_nudge(f"{action} project data: {category}/{key}")


@mcp.tool()
def get_project_data(category: str = "") -> str:
    """Retrieve project data, optionally filtered by category."""
    tracker.record_call("get_project_data", topic=category or None)
    db = get_db()
    if category:
        rows = db.execute(
            "SELECT category, key, value, metadata FROM project_data "
            "WHERE category = ? ORDER BY key",
            (category,),
        ).fetchall()
    else:
        rows = db.execute(
            "SELECT category, key, value, metadata FROM project_data ORDER BY category, key"
        ).fetchall()

    if not rows:
        return with_nudge(
            f"No project data{f' for category {category!r}' if category else ''}.",
            topic=category or None,
        )

    lines = []
    for r in rows:
        line = f"{r['category']}/{r['key']}: {r['value']}"
        if r["metadata"]:
            line += f"  (metadata: {r['metadata']})"
        lines.append(line)
    return with_nudge("\n".join(lines), topic=category or None)


@mcp.tool()
def session_handoff(
    status: str,
    current_task: str = "",
    findings: str = "",
    next_steps: str = "",
    blockers: str = "",
) -> str:
    """Snapshot session state for continuity across sessions.

    Resets ALL behavioral counters.
    """
    tracker.record_call("session_handoff")
    tracker.reset_all()
    db = get_db()
    db.execute(
        "INSERT INTO handoffs "
        "(status, current_task, findings, next_steps, blockers, context_snapshot) "
        "VALUES (?, ?, ?, ?, ?, ?)",
        (
            status,
            current_task,
            findings,
            next_steps,
            blockers,
            json.dumps(
                {
                    "turn_count": tracker.turn_count,
                    "session_duration_s": int(time.time() - tracker.session_start),
                }
            ),
        ),
    )
    db.commit()
    return "Session handoff recorded. Counters reset."


@mcp.tool()
def get_last_handoff() -> str:
    """Retrieve the most recent session handoff for pickup."""
    tracker.record_call("get_last_handoff")
    db = get_db()
    row = db.execute(
        "SELECT * FROM handoffs ORDER BY id DESC LIMIT 1"
    ).fetchone()
    if not row:
        return with_nudge("No previous handoff found.")

    parts = [
        f"Status: {row['status']}",
        f"Task: {row['current_task']}" if row["current_task"] else None,
        f"Findings: {row['findings']}" if row["findings"] else None,
        f"Next steps: {row['next_steps']}" if row["next_steps"] else None,
        f"Blockers: {row['blockers']}" if row["blockers"] else None,
        f"Created: {row['created_at']}",
    ]
    return with_nudge("\n".join(p for p in parts if p))


@mcp.tool()
def add_note(topic: str, content: str) -> str:
    """Quick free-form note. Resets the behavioral store counter."""
    tracker.record_call("add_note", topic=topic)
    tracker.record_store()
    db = get_db()
    db.execute(
        "INSERT INTO notes (topic, content) VALUES (?, ?)",
        (topic, content),
    )
    db.commit()
    return with_nudge(f"Note added: {topic}")


@mcp.tool()
def get_notes(topic: str = "", limit: int = 10) -> str:
    """Retrieve recent notes, optionally filtered by topic."""
    tracker.record_call("get_notes", topic=topic or None)
    db = get_db()
    if topic:
        rows = db.execute(
            "SELECT topic, content, created_at FROM notes "
            "WHERE topic LIKE ? ORDER BY id DESC LIMIT ?",
            (f"%{topic}%", limit),
        ).fetchall()
    else:
        rows = db.execute(
            "SELECT topic, content, created_at FROM notes "
            "ORDER BY id DESC LIMIT ?",
            (limit,),
        ).fetchall()

    if not rows:
        return with_nudge("No notes found.", topic=topic or None)

    lines = [
        f"[{r['created_at']}] {r['topic']}: {r['content']}" for r in rows
    ]
    return with_nudge("\n".join(lines), topic=topic or None)


import re


@mcp.tool()
def save_rule(filename: str, content: str, search_terms: str) -> str:
    """Write a rules file and auto-generate knowledge DB pointers.

    Writes rules/<filename>, extracts ## headings, deletes stale DB entries,
    and creates a pointer for each search term + heading. Idempotent.

    Resets the behavioral store counter.
    """
    tracker.record_call("save_rule", topic=filename)
    tracker.record_store()

    # 1. Write the file
    rules_dir = PROJECT_ROOT / "rules"
    rules_dir.mkdir(parents=True, exist_ok=True)
    file_path = rules_dir / filename
    file_path.write_text(content, encoding="utf-8")
    source = f"rules/{filename}"

    # 2. Extract ## headings
    headings = re.findall(r"^## (.+)$", content, re.MULTILINE)

    # 3. Delete existing pointers for this source (idempotent)
    db = get_db()
    db.execute("DELETE FROM knowledge WHERE source = ?", (source,))

    # 4. Insert pointer for each search term
    terms = [t.strip() for t in search_terms.split(",") if t.strip()]
    for term in terms:
        db.execute(
            "INSERT INTO knowledge (topic, summary, source, tags) "
            "VALUES (?, ?, ?, ?)",
            (term, f"See {source}", source, "rules, pointer"),
        )

    # 5. Insert pointer for each heading
    for heading in headings:
        db.execute(
            "INSERT INTO knowledge (topic, summary, source, tags) "
            "VALUES (?, ?, ?, ?)",
            (
                heading,
                f"Documented in {source} § {heading}",
                source,
                "rules, pointer, section",
            ),
        )

    db.commit()
    total = len(terms) + len(headings)
    log(f"save_rule: {source} — {total} pointers ({len(terms)} terms, {len(headings)} headings)")
    return with_nudge(
        f"Saved {source}\n"
        f"Pointers created: {total} ({len(terms)} search terms + {len(headings)} headings)\n"
        f"Sections found: {headings if headings else '(none)'}"
    )


@mcp.tool()
def get_rule(filename: str) -> str:
    """Read and return the content of a rules file.

    Pass just the filename (e.g., '09-testing.md'), not the full path.
    """
    tracker.record_call("get_rule", topic=filename)
    file_path = PROJECT_ROOT / "rules" / filename
    if not file_path.exists():
        return with_nudge(f"Rule file not found: rules/{filename}")
    return with_nudge(file_path.read_text(encoding="utf-8"))


# --- Entry point ---
if __name__ == "__main__":
    log("Starting Nightscout Knowledge MCP server")
    mcp.run()
