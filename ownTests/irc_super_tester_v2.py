#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Ultra‑Comprehensive IRC Server Tester (RFC 1459‑ish) — v2

• Async, multi‑client, multi‑channel scenario tests
• Covers 42 ft_irc evaluation checklist items as much as possible via black‑box testing
• Validates error numerics, registration flow, modes, permissions, robustness (partial cmds, slow readers)
• Prints a MANUAL CHECKLIST for items that require source inspection (single poll, fcntl, leaks)

USAGE
-----
python3 irc_super_tester_v2.py --host 127.0.0.1 --port 6667 --password pass \
    --verbose

Run a subset:
python3 irc_super_tester_v2.py --only "Registration: basic errors" "Modes: +o/+t topic protection"

Exit code is non‑zero if any test fails.

AUTHOR
------
Upgraded by ChatGPT for Ahmet (ft_irc project).
"""
from __future__ import annotations
import asyncio
import argparse
import re
import sys
import time
from dataclasses import dataclass, field
from typing import Callable, Awaitable, List, Optional, Tuple, Dict, Any

# ----------------------------- Utility -------------------------------------
ANSI = {
    "RESET": "\033[0m",
    "GREEN": "\033[32m",
    "RED": "\033[31m",
    "YELLOW": "\033[33m",
    "CYAN": "\033[36m",
    "BOLD": "\033[1m",
}

def c(s, color):
    return f"{ANSI[color]}{s}{ANSI['RESET']}"

# --------------------------- IRC Client ------------------------------------
class IRCClient:
    def __init__(self, name: str, host: str, port: int, password: Optional[str],
                 verbose: bool = False):
        self.name = name
        self.host = host
        self.port = port
        self.password = password
        self.verbose = verbose
        self.reader: asyncio.StreamReader
        self.writer: asyncio.StreamWriter
        self.recv_log: List[str] = []
        self.connected = False

    async def connect(self):
        self.reader, self.writer = await asyncio.open_connection(self.host, self.port)
        self.connected = True
        if self.verbose:
            print(c(f"[{self.name}] CONNECTED {self.host}:{self.port}", "CYAN"))

    async def close(self):
        if self.connected:
            try:
                self.writer.close()
                await self.writer.wait_closed()
            except Exception:
                pass
            self.connected = False
            if self.verbose:
                print(c(f"[{self.name}] CLOSED", "CYAN"))

    # ------------ sending -------------
    def _send_raw(self, line: str):
        if not line.endswith("\r\n"):
            line += "\r\n"
        if self.verbose:
            print(c(f"[{self.name}] >>> {line.rstrip()}", "YELLOW"))
        self.writer.write(line.encode("utf-8", errors="ignore"))

    def send_raw_bytes_no_crlf(self, data: bytes):
        """Send raw bytes WITHOUT adding CRLF (to simulate partial commands)."""
        if self.verbose:
            preview = data.decode('utf-8', errors='ignore').replace('\r', '␍').replace('\n', '␊')
            print(c(f"[{self.name}] >>>(partial) {preview}", "YELLOW"))
        self.writer.write(data)

    async def drain(self):
        await self.writer.drain()

    def send(self, cmd: str, *params: str, trailing: Optional[str] = None):
        parts = [cmd]
        for p in params:
            if ' ' in p or p == '' or p.startswith(':'):
                if trailing is None:
                    trailing = p
                else:
                    trailing += f" {p}"
            else:
                parts.append(p)
        if trailing is not None:
            parts.append(':' + trailing)
        self._send_raw(' '.join(parts))

    def send_raw(self, raw: str):
        self._send_raw(raw)

    # ------------ receiving ------------
    async def recv_line(self, timeout: float = 2.0) -> Optional[str]:
        try:
            line = await asyncio.wait_for(self.reader.readline(), timeout=timeout)
        except asyncio.TimeoutError:
            return None
        if not line:
            return None
        s = line.decode("utf-8", errors="ignore").rstrip("\r\n")
        self.recv_log.append(s)
        if self.verbose:
            print(c(f"[{self.name}] <<< {s}", "GREEN"))
        return s

    async def expect_regex(self, pattern: str, timeout: float = 2.0) -> bool:
        """Read lines until regex matches (within timeout window)."""
        deadline = asyncio.get_event_loop().time() + timeout
        regex = re.compile(pattern)
        while asyncio.get_event_loop().time() < deadline:
            rem = max(0.02, deadline - asyncio.get_event_loop().time())
            line = await self.recv_line(timeout=rem)
            if line is None:
                await asyncio.sleep(0.01)
                continue
            if regex.search(line):
                return True
        return False

    async def expect_numeric(self, code: str, contains: Optional[str] = None, timeout: float = 2.0) -> bool:
        pat = fr"\s{re.escape(code)}\s"
        if contains:
            pat = fr"{pat}.*{re.escape(contains)}"
        return await self.expect_regex(pat, timeout=timeout)

    async def expect_command(self, cmd: str, timeout: float = 2.0) -> bool:
        pat = fr"\s{re.escape(cmd)}\b"
        return await self.expect_regex(pat, timeout=timeout)

# --------------------------- Test Framework --------------------------------
@dataclass
class TestResult:
    name: str
    ok: bool
    details: str = ""

@dataclass
class TestCase:
    name: str
    func: Callable[["TestContext"], Awaitable[None]]

@dataclass
class TestContext:
    host: str
    port: int
    password: Optional[str]
    verbose: bool
    clients: Dict[str, IRCClient] = field(default_factory=dict)

    async def new_client(self, name: str) -> IRCClient:
        c = IRCClient(name, self.host, self.port, self.password, self.verbose)
        await c.connect()
        self.clients[name] = c
        return c

    async def close_all(self):
        for c in list(self.clients.values()):
            await c.close()
        self.clients.clear()

# --------------------------- Helpers ---------------------------------------
async def register_minimal(c: IRCClient, nick: str, user: Optional[str] = None, real: str = "Real"):
    if c.password is not None:
        c.send("PASS", c.password)
        await c.drain()
    c.send("NICK", nick)
    await c.drain()
    c.send("USER", user or (nick + "u"), "host", "server", trailing=real)
    await c.drain()
    await c.expect_numeric("001", timeout=2.0)

async def drain_until_silent(client: IRCClient, quiet_ms: int = 150):
    deadline = asyncio.get_event_loop().time() + (quiet_ms / 1000.0)
    while True:
        rem = deadline - asyncio.get_event_loop().time()
        if rem <= 0:
            break
        line = await client.recv_line(timeout=rem)
        if line is None:
            break
        deadline = asyncio.get_event_loop().time() + (quiet_ms / 1000.0)

# --------------------------- Individual Tests ------------------------------
# === Registration & basic numerics ===
async def test_registration_basic_errors(ctx: TestContext):
    a = await ctx.new_client("R1")
    a.send("PASS")
    await a.drain()
    assert await a.expect_numeric("461", contains="PASS", timeout=1.0), "Expected 461 for PASS missing parameter"

    # Unknown command => 421
    a.send("FOOBAR", "x")
    await a.drain()
    assert await a.expect_numeric("421", timeout=1.0), "Expected 421 ERR_UNKNOWNCOMMAND"

    # NICK missing => 431, bad nick => 432
    a.send("NICK")
    await a.drain()
    assert await a.expect_numeric("431", timeout=1.0), "Expected 431 ERR_NONICKNAMEGIVEN"

    a.send("NICK", "bad nick")
    await a.drain()
    assert await a.expect_numeric("432", timeout=1.0), "Expected 432 ERR_ERRONEUSNICKNAME"

    # USER without enough params => 461
    a.send("USER", "justone")
    await a.drain()
    assert await a.expect_regex(r"\s(461|451|464)\s.*USER", timeout=1.0), "Expected error for USER (missing params or not registered)"

    # PRIVMSG before register => 451
    a.send("PRIVMSG", "nobody", trailing="hi")
    await a.drain()
    assert await a.expect_numeric("451", timeout=1.0), "Expected 451 You have not registered"
    await a.close()

async def test_successful_registration(ctx: TestContext):
    a = await ctx.new_client("R2")
    await register_minimal(a, "alice")
    # Repeat USER/PASS after registration => 462
    a.send("USER", "aliceu", "h", "s", trailing="Real")
    await a.drain()
    assert await a.expect_numeric("462", timeout=1.0), "Expected 462 ERR_ALREADYREGISTRED on USER after register"

    if ctx.password:
        a.send("PASS", ctx.password)
        await a.drain()
        assert await a.expect_numeric("462", timeout=1.0), "Expected 462 on PASS after register"

    # NICK change to same nick should not break things
    a.send("NICK", "alice")
    await a.drain()
    await drain_until_silent(a)
    await a.close()

async def test_double_nick_collision(ctx: TestContext):
    a = await ctx.new_client("N1")
    b = await ctx.new_client("N2")
    await register_minimal(a, "bob")
    await register_minimal(b, "charlie")
    b.send("NICK", "bob")
    await b.drain()
    assert await b.expect_numeric("433", timeout=1.0), "Expected 433 ERR_NICKNAMEINUSE"
    await a.close(); await b.close()

# === Basic channel ops ===
async def test_join_and_names(ctx: TestContext):
    a = await ctx.new_client("C1")
    await register_minimal(a, "dana")
    a.send("JOIN", "#room1")
    await a.drain()
    assert await a.expect_numeric("353", timeout=1.0), "Expected 353 RPL_NAMREPLY"
    assert await a.expect_numeric("366", timeout=1.0), "Expected 366 RPL_ENDOFNAMES"

    # Query topic before set => 331 (no topic)
    a.send("TOPIC", "#room1")
    await a.drain()
    assert await a.expect_numeric("331", timeout=1.0), "Expected 331 RPL_NOTOPIC"

    # MODE query => 324
    a.send("MODE", "#room1")
    await a.drain()
    assert await a.expect_numeric("324", timeout=1.0), "Expected 324 RPL_CHANNELMODEIS"
    await a.close()

# === Messaging & common errors ===
async def test_privmsg_channel_and_user(ctx: TestContext):
    a = await ctx.new_client("P1")
    b = await ctx.new_client("P2")
    await register_minimal(a, "erin")
    await register_minimal(b, "frank")

    # Errors: missing recipient 411 and missing text 412
    a.send("PRIVMSG")
    await a.drain()
    assert await a.expect_numeric("411", timeout=1.0), "Expected 411 ERR_NORECIPIENT"
    a.send("PRIVMSG", "frank")
    await a.drain()
    assert await a.expect_numeric("412", timeout=1.0), "Expected 412 ERR_NOTEXTTOSEND"

    # To unknown nick => 401
    a.send("PRIVMSG", "idontexist", trailing="hi")
    await a.drain()
    assert await a.expect_numeric("401", timeout=1.0), "Expected 401 ERR_NOSUCHNICK"

    # Direct user message
    a.send("PRIVMSG", "frank", trailing="hello user")
    await a.drain()
    assert await b.expect_command("PRIVMSG", timeout=1.0), "frank should receive PRIVMSG"

    # Channel: ensure 404 when not in channel
    a.send("PRIVMSG", "#c1", trailing="hello?")
    await a.drain()
    assert await a.expect_numeric("404", timeout=1.0), "Expected 404 Cannot send to channel (not joined)"

    # Join both and message
    a.send("JOIN", "#c1")
    b.send("JOIN", "#c1")
    await a.drain(); await b.drain()
    await a.expect_numeric("353", timeout=1.0)
    await b.expect_numeric("353", timeout=1.0)

    a.send("PRIVMSG", "#c1", trailing="hello channel")
    await a.drain()
    assert await b.expect_command("PRIVMSG", timeout=1.0), "channel msg should reach other member"

    # NOTICE should also reach
    a.send("NOTICE", "#c1", trailing="notice msg")
    await a.drain()
    assert await b.expect_command("NOTICE", timeout=1.0), "NOTICE should reach other member"

    await a.close(); await b.close()

# === Modes & permissions ===
async def test_modes_topic_protection(ctx: TestContext):
    a = await ctx.new_client("M1")
    b = await ctx.new_client("M2")
    await register_minimal(a, "ops")
    await register_minimal(b, "user")

    a.send("JOIN", "#m")
    await a.drain(); await a.expect_numeric("353", timeout=1.0)

    # +t topic protection
    a.send("MODE", "#m", "+t")
    await a.drain(); await drain_until_silent(a)

    # set topic by op OK
    a.send("TOPIC", "#m", trailing="welcome")
    await a.drain()
    assert await a.expect_command("TOPIC", timeout=1.0) or await a.expect_numeric("332", timeout=1.0), "Topic should be set/echoed"

    # join normal user and try setting topic => 482
    b.send("JOIN", "#m")
    await b.drain(); await b.expect_numeric("353", timeout=1.0)
    b.send("TOPIC", "#m", trailing="hack")
    await b.drain()
    assert await b.expect_numeric("482", timeout=1.0), "Expected 482 for non‑op topic set when +t"

    # give op to user, then topic OK
    a.send("MODE", "#m", "+o", "user")
    await a.drain(); await drain_until_silent(a)
    b.send("TOPIC", "#m", trailing="allowed now")
    await b.drain()
    assert await a.expect_command("TOPIC", timeout=1.0) or await b.expect_numeric("332", timeout=1.0), "Op can set topic"

    # remove op and ensure blocked again
    a.send("MODE", "#m", "-o", "user")
    await a.drain(); await drain_until_silent(a)
    b.send("TOPIC", "#m", trailing="blocked again")
    await b.drain()
    assert await b.expect_numeric("482", timeout=1.0), "Removing +o should block topic"

    await a.close(); await b.close()

async def test_modes_unknown_and_errors(ctx: TestContext):
    a = await ctx.new_client("MZ")
    await register_minimal(a, "moz")
    a.send("JOIN", "#z")
    await a.drain(); await a.expect_numeric("353", timeout=1.0)

    # Unknown mode flag => 472
    a.send("MODE", "#z", "+z")
    await a.drain()
    assert await a.expect_numeric("472", timeout=1.0), "Expected 472 ERR_UNKNOWNMODE for +z (not supported)"

    # MODE +k without key => 461
    a.send("MODE", "#z", "+k")
    await a.drain()
    assert await a.expect_numeric("461", timeout=1.0), "Expected 461 Need more params for +k without key"

    # PART not on channel => 442
    a.send("PART", "#idontexist", trailing="bye")
    await a.drain()
    assert await a.expect_numeric("442", timeout=1.0), "Expected 442 You're not on that channel"

    await a.close()

async def test_key_limit_invite(ctx: TestContext):
    a = await ctx.new_client("K1")
    b = await ctx.new_client("K2")
    c = await ctx.new_client("K3")
    await register_minimal(a, "keeper")
    await register_minimal(b, "u1")
    await register_minimal(c, "u2")

    a.send("JOIN", "#key")
    await a.drain(); await a.expect_numeric("353", timeout=1.0)
    a.send("MODE", "#key", "+k", "sekret")
    a.send("MODE", "#key", "+l", "2")
    await a.drain(); await drain_until_silent(a)

    b.send("JOIN", "#key", "wrong")
    await b.drain(); assert await b.expect_numeric("475", timeout=1.0), "Expected 475 bad key"
    b.send("JOIN", "#key", "sekret")
    await b.drain(); await b.expect_numeric("353", timeout=1.0)

    c.send("JOIN", "#key", "sekret")
    await c.drain(); assert await c.expect_numeric("471", timeout=1.0), "Expected 471 channel full"

    # invite‑only
    a.send("MODE", "#key", "+i")
    await a.drain(); await drain_until_silent(a)
    c.send("JOIN", "#key", "sekret")
    await c.drain(); assert await c.expect_numeric("473", timeout=1.0), "Expected 473 invite‑only"
    a.send("INVITE", "u2", "#key")
    await a.drain(); await drain_until_silent(a)
    c.send("JOIN", "#key", "sekret")
    await c.drain(); await c.expect_numeric("353", timeout=1.0)

    # non‑op KICK should fail with 482
    b.send("KICK", "#key", "u2", trailing="bye")
    await b.drain(); assert await b.expect_numeric("482", timeout=1.0), "Non‑op KICK must fail"

    # op KICK works
    a.send("KICK", "#key", "u2", trailing="bye")
    await a.drain(); assert await c.expect_command("KICK", timeout=1.0), "Kicked user should see KICK"

    await a.close(); await b.close(); await c.close()

# === PART / QUIT visibility ===
async def test_part_quit_visibility(ctx: TestContext):
    a = await ctx.new_client("PQ1")
    b = await ctx.new_client("PQ2")
    await register_minimal(a, "sam")
    await register_minimal(b, "tom")

    a.send("JOIN", "#pq")
    b.send("JOIN", "#pq")
    await a.drain(); await b.drain()
    await a.expect_numeric("353", timeout=1.0); await b.expect_numeric("353", timeout=1.0)

    # PART should be seen by the other user
    b.send("PART", "#pq", trailing="see ya")
    await b.drain()
    assert await a.expect_command("PART", timeout=1.0), "PART should be broadcast to channel"

    # QUIT with message should be seen by peers in same channel
    b = await ctx.new_client("PQ3")
    await register_minimal(b, "tim")
    b.send("JOIN", "#pq")
    await b.drain(); await b.expect_numeric("353", timeout=1.0)
    b.send("QUIT", trailing="bye")
    await b.drain()
    assert await a.expect_command("QUIT", timeout=1.0), "QUIT should be seen by shared channel peers"

    await a.close()

# === WHOIS, PING/PONG ===
async def test_whois_ping(ctx: TestContext):
    a = await ctx.new_client("W1")
    await register_minimal(a, "zara")
    a.send("WHOIS", "zara")
    await a.drain()
    assert await a.expect_regex(r"\s(311|318)\s|RPL_WHOIS", timeout=1.5), "Expected WHOIS reply (311 + 318 typical)"

    a.send("PING", "server")
    await a.drain()
    assert await a.expect_command("PONG", timeout=1.0), "Expected PONG"
    await a.close()

# === Robustness: partial commands, slow readers, abrupt close ===
async def test_partial_command_assembly(ctx: TestContext):
    a = await ctx.new_client("PC1")
    b = await ctx.new_client("PC2")
    await register_minimal(a, "pa")
    await register_minimal(b, "pb")

    # join both to the same channel
    a.send("JOIN", "#p")
    b.send("JOIN", "#p")
    await a.drain(); await b.drain()
    await a.expect_numeric("353", timeout=1.0); await b.expect_numeric("353", timeout=1.0)

    # Send PRIVMSG split across multiple writes without CRLF until the end
    a.send_raw_bytes_no_crlf(b"PRIV")
    await a.drain(); await asyncio.sleep(0.02)
    a.send_raw_bytes_no_crlf(b"MSG #p :hello ")
    await a.drain(); await asyncio.sleep(0.02)
    a.send_raw_bytes_no_crlf(b"world\r\n")
    await a.drain()

    assert await b.expect_command("PRIVMSG", timeout=1.0), "Server must assemble fragmented lines"
    await a.close(); await b.close()

async def test_slow_reader_does_not_block(ctx: TestContext):
    # One client will not read for a while. Another will flood. Server must not hang.
    slow = await ctx.new_client("SLOW")
    fast = await ctx.new_client("FAST")
    ctrl = await ctx.new_client("CTRL")
    await register_minimal(slow, "slow")
    await register_minimal(fast, "fast")
    await register_minimal(ctrl, "ctrl")

    slow.send("JOIN", "#f")
    fast.send("JOIN", "#f")
    await slow.drain(); await fast.drain()
    await slow.expect_numeric("353", timeout=1.0); await fast.expect_numeric("353", timeout=1.0)

    # Flood channel from fast while we do a control PING to ensure server remains responsive
    for i in range(50):
        fast.send("PRIVMSG", "#f", trailing=f"msg {i}")
        if i % 10 == 0:
            ctrl.send("PING", f"tick-{i}")
        await fast.drain()
        await ctrl.drain()

    # ctrl should receive PONGs quickly despite slow not reading
    ok = await ctrl.expect_command("PONG", timeout=1.5)
    assert ok, "Server appears blocked by a slow consumer"

    # Now let slow start reading and ensure it still stays connected (no crash/hang assumed)
    await drain_until_silent(slow, 200)

    await slow.close(); await fast.close(); await ctrl.close()

async def test_abrupt_disconnect_no_global_hang(ctx: TestContext):
    a = await ctx.new_client("AB1")
    b = await ctx.new_client("AB2")
    await register_minimal(a, "alive")
    await register_minimal(b, "boom")

    # b joins and then we close b abruptly
    b.send("JOIN", "#ab")
    await b.drain(); await b.expect_numeric("353", timeout=1.0)

    # Close socket immediately (simulating kill -9 / cable pull)
    try:
        b.writer.transport.close()  # force close
    except Exception:
        pass

    # a should still be able to ping and receive pong
    a.send("PING", "server")
    await a.drain()
    assert await a.expect_command("PONG", timeout=1.0), "Server should remain responsive after peer abort"
    await a.close()

# --------------------------- Test Suite ------------------------------------
ALL_TESTS: List[TestCase] = [
    # Registration & basic numerics
    TestCase("Registration: basic errors", test_registration_basic_errors),
    TestCase("Registration: success & already registered", test_successful_registration),
    TestCase("Nick collision (433)", test_double_nick_collision),

    # Channels & messaging
    TestCase("JOIN + NAMES (353/366/331/324)", test_join_and_names),
    TestCase("PRIVMSG/NOTICE + common errors (401/404/411/412)", test_privmsg_channel_and_user),

    # Modes & permissions
    TestCase("Modes: +o/+t topic protection", test_modes_topic_protection),
    TestCase("Modes: unknown flag & param errors (472/461/442)", test_modes_unknown_and_errors),
    TestCase("Modes: +k/+l/+i with JOIN/INVITE/KICK (475/471/473/482)", test_key_limit_invite),

    # PART/QUIT visibility
    TestCase("PART + QUIT visibility to peers", test_part_quit_visibility),

    # WHOIS + PING
    TestCase("WHOIS + PING/PONG", test_whois_ping),

    # Robustness
    TestCase("Robustness: partial command assembly", test_partial_command_assembly),
    TestCase("Robustness: slow reader doesn't block others", test_slow_reader_does_not_block),
    TestCase("Robustness: abrupt disconnect doesn't hang server", test_abrupt_disconnect_no_global_hang),
]

# --------------------------- Runner ----------------------------------------
async def run_tests(ctx: TestContext, selected: Optional[List[str]] = None) -> Tuple[int, List[TestResult]]:
    results: List[TestResult] = []
    failed = 0
    tests = [t for t in ALL_TESTS if (not selected or t.name in selected)]
    for t in tests:
        if ctx.verbose:
            print(c(f"\n=== RUN: {t.name} ===", "BOLD"))
        try:
            await t.func(ctx)
            await ctx.close_all()
            results.append(TestResult(t.name, True))
            if ctx.verbose:
                print(c(f"=== OK: {t.name} ===", "GREEN"))
        except AssertionError as e:
            failed += 1
            results.append(TestResult(t.name, False, str(e)))
            print(c(f"=== FAIL: {t.name}: {e} ===", "RED"))
            await ctx.close_all()
        except Exception as e:
            failed += 1
            results.append(TestResult(t.name, False, f"Unhandled: {e}"))
            print(c(f"=== ERROR: {t.name}: {e} ===", "RED"))
            await ctx.close_all()
    return failed, results

# --------------------------- Manual Checklist ------------------------------
MANUAL_CHECKLIST = """
MANUAL CHECKLIST (ft_irc scale form)
------------------------------------
The following require human/source inspection and/or external tools:

1) Build & binary name: Makefile produces C++ binary named `ircserv`.
2) Exactly one poll()/ppoll()/epoll_wait()/kqueue loop in the code path.
3) fcntl() usage ONLY as: fcntl(fd, F_SETFL, O_NONBLOCK).
4) poll() (or equivalent) is called before each accept/read/recv/write/send; errno is NOT abused for retries.
5) No crashes/segfaults during defense; use appropriate flag if any occurs.
6) Memory leaks: run with valgrind/leaks/e_fence and ensure no reachable/definitely lost blocks.
7) Reference IRC client connectivity and (if applicable) DCC/file transfer via the chosen client.
8) Optional extra: a small bot available and connectable.

Tip: run this tester concurrently with a GUI client (irssi/weechat/hexchat) while observing server logs.
"""

# --------------------------- Main ------------------------------------------
def parse_args():
    p = argparse.ArgumentParser(description="Ultra IRC tester (ft_irc‑ready)")
    p.add_argument("--host", default="127.0.0.1")
    p.add_argument("--port", type=int, default=6667)
    p.add_argument("--password", default=None, help="PASS password if required")
    p.add_argument("--verbose", action="store_true")
    p.add_argument("--only", nargs="*", help="Run only the tests with exact names")
    p.add_argument("--print-manual-checklist", action="store_true", help="Print the manual evaluator checklist and exit")
    return p.parse_args()

async def amain():
    args = parse_args()
    if args.print_manual_checklist:
        print(MANUAL_CHECKLIST)
        return
    ctx = TestContext(host=args.host, port=args.port, password=args.password, verbose=args.verbose)
    failed, results = await run_tests(ctx, selected=args.only)

    # Summary
    total = len(results)
    ok = sum(1 for r in results if r.ok)
    print()
    if failed == 0:
        print(c(f"ALL TESTS PASSED ({ok}/{total})", "GREEN"))
    else:
        print(c(f"{failed} TEST(S) FAILED ({ok}/{total})", "RED"))
        for r in results:
            if not r.ok:
                print(c(f" - {r.name}: {r.details}", "RED"))
    sys.exit(1 if failed else 0)

if __name__ == "__main__":
    try:
        asyncio.run(amain())
    except KeyboardInterrupt:
        print("Interrupted.")
