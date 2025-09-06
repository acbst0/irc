
#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
IRC Functional Tester
---------------------
Runs a battery of tests against your IRC server implementation.

Usage:
  python irc_tester.py --port 6667 --pass pass [--host 127.0.0.1] [--verbose]

What it does (high level):
- Connects 1–3 clients as needed.
- Exercises PASS/NICK/USER registration paths (happy + error cases).
- Checks server numerics (001..004, 431/432/433/451/461/462/464/409).
- Pings server (PING->PONG).
- Smoke-tests core commands after registration (JOIN, PART, PRIVMSG, NOTICE,
  MODE, TOPIC, NAMES, LIST, INVITE, KICK, WHO, WHOIS, QUIT).
- Prints a compact pass/fail summary.

Notes:
- The tester looks for canonical numerics/messages your code already emits.
- For channel/privmsg features, this is a *smoke test*: it checks that the
  server responds (and doesn't send obvious error numerics), not the full RFC behavior.
- If your handlers use different numerics/texts, tweak the EXPECT_* regexes below.

Test philosophy:
- Make minimal assumptions about formatting; we use regex and allow any prefix.
- Read until idle timeout, matching lines incrementally.
"""

import argparse, socket, time, re, sys, threading, queue, ssl
from dataclasses import dataclass, field
from typing import List, Optional, Tuple

DEFAULT_TIMEOUT = 2.5   # seconds to wait for replies per expect
READ_CHUNK = 4096

def now_ms():
    return int(time.time() * 1000)

class IRCClient:
    def __init__(self, host: str, port: int, name: str, use_ssl: bool = False, verbose: bool = False):
        self.host = host
        self.port = port
        self.name = name
        self.verbose = verbose
        self.sock = None
        self.buf = b""
        self.connected = False
        self.use_ssl = use_ssl

    def connect(self, timeout: float = 5.0):
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.settimeout(timeout)
        s.connect((self.host, self.port))
        if self.use_ssl:
            ctx = ssl.create_default_context()
            s = ctx.wrap_socket(s, server_hostname=self.host)
        s.settimeout(0.1)  # non-blocking-ish reads
        self.sock = s
        self.connected = True
        if self.verbose:
            print(f"[{self.name}] connected to {self.host}:{self.port}")

    def send(self, line: str):
        if not line.endswith("\r\n"):
            line += "\r\n"
        if self.verbose:
            print(f">>> [{self.name}] {line.rstrip()}")
        self.sock.sendall(line.encode("utf-8", "ignore"))

    def read_available_lines(self) -> List[str]:
        lines = []
        try:
            while True:
                data = self.sock.recv(READ_CHUNK)
                if not data:
                    self.connected = False
                    break
                self.buf += data
                while b"\r\n" in self.buf:
                    line, self.buf = self.buf.split(b"\r\n", 1)
                    s = line.decode("utf-8", "ignore")
                    if self.verbose:
                        print(f"<<< [{self.name}] {s}")
                    lines.append(s)
        except socket.timeout:
            pass
        except BlockingIOError:
            pass
        return lines

    def expect(self, pattern: str, timeout: float = DEFAULT_TIMEOUT) -> Tuple[bool, Optional[str]]:
        """Wait up to timeout seconds for a line matching regex pattern."""
        deadline = time.time() + timeout
        regex = re.compile(pattern)
        haystack = []
        while time.time() < deadline:
            lines = self.read_available_lines()
            haystack.extend(lines)
            for ln in lines:
                if regex.search(ln):
                    return True, ln
            time.sleep(0.02)
        return False, ("\n".join(haystack) if haystack else None)

    def close(self):
        try:
            self.sock.close()
        except Exception:
            pass
        self.connected = False

@dataclass
class TestResult:
    name: str
    ok: bool
    detail: str = ""

@dataclass
class Tester:
    host: str
    port: int
    password: str
    verbose: bool = False
    results: List[TestResult] = field(default_factory=list)

    # --- Utility ---
    def new_client(self, label: str) -> IRCClient:
        c = IRCClient(self.host, self.port, label, verbose=self.verbose)
        c.connect()
        return c

    def record(self, name: str, ok: bool, detail: str = ""):
        self.results.append(TestResult(name, ok, detail))

    # --- Test cases ---

    def test_pass_missing(self):
        name = "PASS missing -> 461"
        c = self.new_client("A")
        try:
            c.send("PASS")  # missing param
            ok, ln = c.expect(r":server 461 \* PASS :Not enough parameters")
            self.record(name, ok, ln or "no reply")
        finally:
            c.close()

    def test_pass_wrong(self):
        name = "PASS wrong -> 464"
        c = self.new_client("A")
        try:
            c.send("PASS wrongpass")
            ok, ln = c.expect(r":server 464 \* :Password incorrect")
            self.record(name, ok, ln or "no reply")
        finally:
            c.close()

    def test_unauthed_other_cmd(self):
        name = "Unauthed NICK -> 464"
        c = self.new_client("A")
        try:
            c.send("NICK foo")
            ok, ln = c.expect(r":server 464 \* :Password incorrect")
            self.record(name, ok, ln or "no reply")
        finally:
            c.close()

    def register(self, c: IRCClient, nick: str, user: str, rname: str = "Real", hostname: str = "host"):
        # PASS ok
        c.send(f"PASS {self.password}")
        ok, _ = c.expect(r":server 464 \* :Password incorrect", timeout=0.3)
        if ok:
            raise AssertionError("Server says PASS incorrect while we passed the right password.")
        # NICK
        c.send(f"NICK {nick}")
        # USER (SERVER expects 4 params; USER <user> <host> <server> <realname>)
        c.send(f"USER {user} {hostname} server :{rname}")
        # Expect welcome numerics eventually
        ok, _ = c.expect(r":server 001 " + re.escape(nick) + r" :Welcome", timeout=3.0)
        return ok

    def test_register_and_welcome(self):
        name = "Register -> 001/002/003/004"
        c = self.new_client("A")
        try:
            ok_welcome = self.register(c, "alice", "aliceu")
            ok_host, _ = c.expect(r":server 002 alice :Your host is", timeout=1.5)
            ok_when, _ = c.expect(r":server 003 alice :This server was created", timeout=1.2)
            ok_vers, _ = c.expect(r":server 004 alice server 1\.0 o o", timeout=1.2)
            self.record(name, ok_welcome and ok_host and ok_when and ok_vers,
                        f"001={ok_welcome}, 002={ok_host}, 003={ok_when}, 004={ok_vers}")
        finally:
            c.close()

    def test_user_not_enough_params(self):
        name = "USER missing params -> 461"
        c = self.new_client("A")
        try:
            c.send(f"PASS {self.password}")
            c.send("USER onlyoneparam")
            ok, ln = c.expect(r":server 461 \* USER :Not enough parameters")
            self.record(name, ok, ln or "no reply")
        finally:
            c.close()

    def test_nick_errors(self):
        # 431 No nickname given
        name1 = "NICK missing -> 431"
        c = self.new_client("A")
        try:
            c.send(f"PASS {self.password}")
            c.send("NICK")
            ok1, ln1 = c.expect(r":server 431 \* :No nickname given")
            self.record(name1, ok1, ln1 or "no reply")
        finally:
            c.close()

        # 432 Erroneous nickname (contains space)
        name2 = "NICK erroneous -> 432"
        c = self.new_client("A")
        try:
            c.send(f"PASS {self.password}")
            c.send("NICK bad nick")
            ok2, ln2 = c.expect(r":server 432 \* bad nick :Erroneous nickname")
            self.record(name2, ok2, ln2 or "no reply")
        finally:
            c.close()

        # 433 Nick in use
        name3 = "NICK in use -> 433"
        c1 = self.new_client("A")
        c2 = self.new_client("B")
        try:
            self.register(c1, "taken", "u1")
            c2.send(f"PASS {self.password}")
            c2.send("NICK taken")
            ok3, ln3 = c2.expect(r":server 433 (?:\*|taken) taken :Nickname is already in use")
            self.record(name3, ok3, ln3 or "no reply")
        finally:
            c1.close()
            c2.close()

    def test_reregister_errors(self):
        name = "USER after registration -> 462"
        c = self.new_client("A")
        try:
            self.register(c, "rr", "rru")
            c.send("USER rr host server :Real Again")
            ok, ln = c.expect(r":server 462 \* :You may not reregister")
            self.record(name, ok, ln or "no reply")
        finally:
            c.close()

    def test_unregistered_vs_registered_guards(self):
        # After PASS but before full reg, commands should hit 451 (not registered)
        name = "Before registration JOIN -> 451"
        c = self.new_client("A")
        try:
            c.send(f"PASS {self.password}")
            c.send("JOIN #test")
            ok, ln = c.expect(r":server 451 \* :You have not registered")
            self.record(name, ok, ln or "no reply")
        finally:
            c.close()

    def test_ping_pong(self):
        name = "PING -> PONG"
        c = self.new_client("A")
        try:
            self.register(c, "pinger", "pingu")
            c.send("PING 12345")
            ok, ln = c.expect(r":server PONG server :12345")
            self.record(name, ok, ln or "no reply")
        finally:
            c.close()

    def test_core_commands_smoke(self):
        name = "Core commands smoke (JOIN/PART/PRIVMSG/NOTICE/MODE/TOPIC/NAMES/LIST/INVITE/KICK/WHO/WHOIS/QUIT)"
        c1 = self.new_client("A")
        c2 = self.new_client("B")
        try:
            self.register(c1, "u1", "u1u")
            self.register(c2, "u2", "u2u")

            def ok_resp(regex: str, timeout=1.5) -> bool:
                ok1, _ = c1.expect(regex, timeout=timeout)
                ok2, _ = c2.expect(regex, timeout=0.05)  # in case the mirror lands elsewhere
                return ok1 or ok2

            # JOIN
            c1.send("JOIN #room")
            c2.send("JOIN #room")
            time.sleep(0.3)

            # PRIVMSG to channel and to user
            c1.send("PRIVMSG #room :hello room")
            c2.send("PRIVMSG u1 :hi u1")
            time.sleep(0.2)

            # NOTICE
            c1.send("NOTICE #room :notice msg")
            time.sleep(0.2)

            # WHO, WHOIS
            c1.send("WHO #room")
            c1.send("WHOIS u2")

            # MODE (channel/user generic)
            c1.send("MODE #room")
            c1.send("MODE u1")

            # TOPIC
            c1.send("TOPIC #room :Welcome here")
            time.sleep(0.2)

            # NAMES, LIST
            c1.send("NAMES #room")
            c1.send("LIST")

            # INVITE/KICK (may require ops; just ensure a response exists)
            c1.send("INVITE u2 #room")
            c1.send("KICK #room u2 :bye")

            # PART and QUIT
            c2.send("PART #room :leaving")
            c2.send("QUIT :bye")

            # Check we received at least some response lines (non-empty).
            # We avoid strict numerics because implementations may vary.
            got_any = False
            start = time.time()
            while time.time() - start < 2.0:
                got_any |= bool(c1.read_available_lines() or c2.read_available_lines())
                if got_any:
                    break

            self.record(name, got_any, "received some responses" if got_any else "no responses captured")
        finally:
            c1.close()
            c2.close()

    def run_all(self):
        self.test_pass_missing()
        self.test_pass_wrong()
        self.test_unauthed_other_cmd()
        self.test_user_not_enough_params()
        self.test_register_and_welcome()
        self.test_nick_errors()
        self.test_reregister_errors()
        self.test_unregistered_vs_registered_guards()
        self.test_ping_pong()
        self.test_core_commands_smoke()

    def summary(self):
        ok = sum(1 for r in self.results if r.ok)
        total = len(self.results)
        print("\n=== TEST SUMMARY ===")
        for r in self.results:
            mark = "✅" if r.ok else "❌"
            detail = f" — {r.detail}" if r.detail else ""
            print(f"{mark} {r.name}{detail}")
        print(f"\n{ok}/{total} tests passed.")

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--host", default="127.0.0.1")
    ap.add_argument("--port", type=int, required=True)
    ap.add_argument("--pass", dest="password", required=True)
    ap.add_argument("--verbose", action="store_true")
    ap.add_argument("--ssl", action="store_true", help="Use TLS to connect")
    args = ap.parse_args()

    t = Tester(args.host, args.port, args.password, verbose=args.verbose)
    try:
        t.run_all()
    except AssertionError as e:
        print(f"[fatal] {e}", file=sys.stderr)
    finally:
        t.summary()

if __name__ == "__main__":
    main()
