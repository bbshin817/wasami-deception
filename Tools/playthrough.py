"""Plays the hospital through in PIE with the keys a player uses, section by section (work list item 27; item 21 plays
it again). Each section goes on from where the one before it stopped, so a run of several is one playthrough.

    python Tools/playthrough.py list                                  the sections in order
    python Tools/playthrough.py status                                level, checkpoint, lives, shards, objective, player
    python Tools/playthrough.py run z1_arrive z1_maze                 run sections one after another
    python Tools/playthrough.py run --from z1_arrive --to z1_ambulance
    python Tools/playthrough.py run z1_parking --setup                first put the game where the section begins (the
                                                                      save's checkpoint, the level opened again)
    python Tools/playthrough.py run ... --record through.mkv          record the viewport while the sections run
    python Tools/playthrough.py run ... --shots                       a screenshot of the viewport at each milestone

Needs PIE running (python Tools/pie.py start) and the desktop agent (python Tools/desktop.py start); the run clicks the
viewport once (--viewport L T R B, where the game shows inside the editor's window) so that the keys reach the game.
Walking holds Shift and W down and turns the view from the editor towards the next point of the navigation path, so
the player goes round corners and through the trigger boxes on foot; long ways between sections are shortened with a
place in front of the next box, never past it (a box passed before the flow binds it is used up: 11 record). Always
finish with python Tools/pie.py stop. Exit code 0 when every section reached its end, 1 otherwise.
"""
import argparse
import json
import math
import os
import sys
import textwrap
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import desktop  # noqa: E402  (same folder)
import pie  # noqa: E402
import ue_remote  # noqa: E402  (puts the engine's remote_execution on the path)
import remote_execution as rx  # noqa: E402

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
SHOTS = os.path.join(ROOT, "Intermediate", "DesktopAgent", "shots")
ALLOW = ["UnrealEditor.exe"]
# The game inside the editor's viewport with the window where it was on 2026-09-19 (screen pixels, even sizes for the
# encoder). Measure again from a shot of the editor's window when the layout changes.
VIEWPORT = (1822, 206, 2862, 858)
WALK_KEYS = ["shift", "w"]


class Failed(Exception):
    pass


class Editor:
    """One remote-execution connection kept open for the whole run (a new one per call costs about a second)."""

    def __init__(self):
        self.ex = rx.RemoteExecution()
        self.ex.start()
        deadline = time.time() + 8
        while time.time() < deadline and not self.ex.remote_nodes:
            time.sleep(0.2)
        if not self.ex.remote_nodes:
            self.ex.stop()
            raise Failed("no editor answered (is it running with remote execution on?)")
        self.ex.open_command_connection(self.ex.remote_nodes[0]["node_id"])

    def close(self):
        self.ex.stop()

    def run(self, body):
        """Runs statements in one function with pie.py's helpers; returns what they printed."""
        code = "def _pt_main():\n" + textwrap.indent(pie.PRELUDE + body, "    ") + "\n_pt_main()\n"
        res = self.ex.run_command(code, unattended=True, exec_mode=rx.MODE_EXEC_FILE)
        out = "".join(entry.get("output", "") for entry in res.get("output", []))
        if not res.get("success"):
            raise Failed("editor python failed: %s\n%s" % (res.get("result"), out))
        return out

    def json(self, body):
        """Runs statements that end by printing one line 'JSON <value>'; returns the value."""
        out = self.run("import json\n" + body)
        for line in out.splitlines():
            if line.startswith("JSON "):
                return json.loads(line[5:])
        raise Failed("no JSON in the editor's answer:\n" + out)


MARK = "PlaythroughSeen"  # a tag on the game mode: a level opened anew has a game mode without it

STATUS = """
MARK = %r
w = _game()
s = {'pie': w is not None}
if w is not None:
    s['world'] = w.get_name()
    s['real'] = unreal.GameplayStatics.get_real_time_seconds(w)
    s['time'] = unreal.GameplayStatics.get_time_seconds(w)
    s['paused'] = unreal.GameplayStatics.is_game_paused(w)
    mode = unreal.GameplayStatics.get_game_mode(w)
    s['marked'] = mode is not None and MARK in [str(t) for t in mode.get_editor_property('tags')]
    if isinstance(mode, unreal.WasamiGameMode):
        save = mode.get_save()
        s['checkpoint'] = save.get_editor_property('hospital').get_editor_property('level_checkpoint') if save else None
        s['objective'] = str(mode.get_editor_property('current_objective'))
    elif unreal.GameplayStatics.does_save_game_exist('structSlot', 0):
        save = unreal.GameplayStatics.load_game_from_slot('structSlot', 0)
        s['checkpoint'] = save.get_editor_property('hospital').get_editor_property('level_checkpoint') if save else None
    titles = unreal.WidgetLibrary.get_all_widgets_of_class(w, unreal.WasamiTitleScreenWidget, False)
    s['title'] = len(titles) > 0
    resume = unreal.find_object(None, titles[0].get_path_name() + '.WidgetTree.Resume') if titles else None
    s['resume'] = resume is not None and resume.get_parent() is not None  # Construct takes it out without a game begun
    s['popup'] = len(unreal.WidgetLibrary.get_all_widgets_of_class(w, unreal.WasamiPopUpWidget, False)) > 0
    instance = unreal.GameplayStatics.get_game_instance(w)
    if isinstance(instance, unreal.WasamiGameInstance):
        s['lives'] = instance.get_lives()
    s['shards'] = len(unreal.GameplayStatics.get_all_actors_of_class(w, unreal.WasamiShard))
    s['captured'] = len(unreal.GameplayStatics.get_all_actors_of_class(w, unreal.WasamiCapture)) > 0
    s['clear'] = len(unreal.WidgetLibrary.get_all_widgets_of_class(w, unreal.WasamiLevelClearWidget, False)) > 0
    player = unreal.GameplayStatics.get_player_character(w, 0)
    if player is not None:
        l = player.get_actor_location()
        r = unreal.GameplayStatics.get_player_controller(w, 0).get_control_rotation()
        s['player'] = [round(l.x, 1), round(l.y, 1), round(l.z, 1), round(r.yaw, 1), round(r.pitch, 1)]
print('JSON ' + json.dumps(s))
""" % MARK

# The middle of a button of the title's menu, of the question over it (UWasamiPopUpWidget) or of the game over's, as
# fractions of the viewport. Python cannot read a widget's geometry (FGeometry has no reflected fields), so this lays the
# button out as the screens place it (14 and 09 records): the title's menu VerticalBox_160 at (7.06, -93.09) from the
# left edge's middle, 421.75 wide, each button its desired height less 10; the question's column 115.2 below the
# middle, the question over YES, 50, NO, 40 below it; the game over's VerticalBox_161 hanging from 358.19 above the
# bottom's middle, 10 around each button. Sizes are in the screen's units; the viewport's DPI scale turns them into
# pixels.
WIDGET_AT = """
w = _need_game()
scale = unreal.WidgetLayoutLibrary.get_viewport_scale(w)
pixels = unreal.WidgetLayoutLibrary.get_viewport_size(w)
width, height = pixels.x / scale, pixels.y / scale
found = unreal.WidgetLibrary.get_all_widgets_of_class(w, unreal.{cls}, False)
part = (lambda n: unreal.find_object(None, found[-1].get_path_name() + '.WidgetTree.' + n)) if found else None
at = None
if found and {cls!r} == 'WasamiTitleScreenWidget':
    top = height / 2 - 93.0933
    for n in ['Resume', 'NewGame', 'Options', 'Quit']:
        button = part(n)
        if button is None or button.get_parent() is None:
            continue
        tall = button.get_desired_size().y
        if n == {name!r}:
            at = [7.0576 + 421.7503 / 2, top + tall / 2]
            break
        top += tall - 10.0
elif found and {cls!r} == 'WasamiPopUpWidget':
    question, yes, no = part('RichTextBlock_374').get_desired_size(), part('Yes').get_desired_size(), part('No').get_desired_size()
    row_wide, row_tall = yes.x + 50.0 + no.x, max(yes.y, no.y)
    top = height / 2 + 115.205 - (question.y + 40.0 + row_tall) / 2
    left = width / 2 - row_wide / 2
    x = left + yes.x / 2 if {name!r} == 'Yes' else left + yes.x + 50.0 + no.x / 2
    at = [x, top + question.y + 40.0 + row_tall / 2]
elif found and {cls!r} == 'WasamiDeathScreenWidget':
    top = height - 358.188
    for n in ['Restart', 'LastCheckpoint', 'QUITTOTITLE']:
        button = part(n)
        if button is None or button.get_visibility() == unreal.SlateVisibility.COLLAPSED:
            continue
        tall = button.get_desired_size().y
        if n == {name!r}:
            at = [width / 2, top + 10.0 + tall / 2]
            break
        top += tall + 20.0
print('JSON ' + json.dumps([at[0] / width, at[1] / height] if at else None))
"""

MARK_WORLD = """
w = _need_game()
mode = unreal.GameplayStatics.get_game_mode(w)
mode.set_editor_property('tags', list(mode.get_editor_property('tags')) + [%r])
""" % MARK

# One step of walking: where the player is, the next point of the path it heads for, and the view turned towards it
# (by at most max_turn degrees, so the recording pans instead of cutting).
STEER = """
import math
w = _need_game()
player = unreal.GameplayStatics.get_player_character(w, 0)
pc = unreal.GameplayStatics.get_player_controller(w, 0)
path, index, reach, max_turn = {path!r}, {index!r}, {reach!r}, {max_turn!r}
l = player.get_actor_location()
def flat(p):
    return ((p[0] - l.x) ** 2 + (p[1] - l.y) ** 2) ** 0.5
while index < len(path) - 1 and flat(path[index]) < reach:
    index += 1
goal = path[index]
r = pc.get_control_rotation()
want = math.degrees(math.atan2(goal[1] - l.y, goal[0] - l.x))
turn = (want - r.yaw + 180.0) % 360.0 - 180.0
turn = max(-max_turn, min(max_turn, turn))
pc.set_control_rotation(unreal.Rotator(roll=0.0, pitch={pitch!r} if {pitch!r} is not None else r.pitch, yaw=r.yaw + turn))
mode = unreal.GameplayStatics.get_game_mode(w)
sentries = unreal.GameplayStatics.get_all_actors_of_class(w, unreal.WasamiEnemySentry)
escaped = {escape!r} and any(f.get_editor_property('hold') for f in unreal.WidgetLibrary.get_all_widgets_of_class(
    w, unreal.WasamiBlackFadeWidget, False))
print('JSON ' + json.dumps({{'at': [l.x, l.y, l.z], 'index': index, 'left': flat(goal), 'turn': turn,
                             'time': unreal.GameplayStatics.get_time_seconds(w),
                             'captured': len(unreal.GameplayStatics.get_all_actors_of_class(w, unreal.WasamiCapture)) > 0,
                             'paused': unreal.GameplayStatics.is_game_paused(w),
                             'objective': str(mode.get_editor_property('current_objective')),
                             'checkpoint': mode.get_save().get_editor_property('hospital').get_editor_property('level_checkpoint'),
                             'cones': {{a.get_actor_label(): a.get_viewcone().is_on() for a in sentries if a.get_viewcone()}},
                             'spotted': [a.get_actor_label() for a in sentries if a.is_chasing()],
                             'escaped': escaped}}))
"""

# The sentries' cones: where each looks from and how far (07 record: the cone's actor 72 cm over the sentry's head,
# pitched 20 degrees down; a player inside Angle of its forward and nearer than Length, and in full view, is spotted).
CONES = """
w = _need_game()
out = []
for a in unreal.GameplayStatics.get_all_actors_of_class(w, unreal.WasamiEnemySentry):
    c = a.get_viewcone()
    if c:
        l, f = c.get_actor_location(), c.get_actor_forward_vector()
        out.append([a.get_actor_label(), a.get_editor_property('offset'), [l.x, l.y, l.z], [f.x, f.y, f.z],
                    c.get_editor_property('angle'), c.get_editor_property('length')])
print('JSON ' + json.dumps(out))
"""

NAV_PATH = """
w = _need_game()
player = unreal.GameplayStatics.get_player_character(w, 0)
start = player.get_actor_location()
end = unreal.Vector({x!r}, {y!r}, {z!r} if {z!r} is not None else start.z)
path = unreal.NavigationSystemV1.find_path_to_location_synchronously(w, start, end)
points = [[p.x, p.y, p.z] for p in path.get_editor_property('path_points')] if path else []
print('JSON ' + json.dumps(points))
"""

FACE = """
w = _need_game()
pc = unreal.GameplayStatics.get_player_controller(w, 0)
r = pc.get_control_rotation()
pc.set_control_rotation(unreal.Rotator(roll=0.0, pitch={pitch!r} if {pitch!r} is not None else r.pitch, yaw={yaw!r}))
"""


def flat(a, b):
    return math.hypot(a[0] - b[0], a[1] - b[1])


class Cones:
    """Keeps the player out of the sight of Zone 2's six sentries (GET PAST THE NURSES), as a player would: the cones
    take turns, each looking for 9 s and resting for 11 s, those whose sentry's Offset is 10 ten seconds after those with
    0 (07 record: the first UpdateSight 0.3 to 0.5 s after Activate, the look 1 s after TurnOn, Turn every 10 s). The
    player stops before a stretch of the way that a cone looks at while the cone looks, or would look before the player
    is out of it. A cone's clock starts from awake (the game time the sentries were activated) and is set again when the
    cone is seen to turn on or off; with neither, the cone counts as looking."""
    ON, CYCLE = 9.0, 20.0
    FIRST_LOOK = 1.4            # s after Activate (and Offset) that a cone first looks
    MARGIN_ANGLE, MARGIN_LENGTH = 4.0, 100.0
    STEP = 25.0                 # cm between the samples along the way
    LOOK_AHEAD = 260.0          # stop when a stretch begins this near (a sprint stops within about 90 cm)
    RUN, SLOW = 600.0, 400.0    # cm/s: the sprint to a stretch, and a slower pace out of it (corners)
    SLACK = 1.2                 # s: a cone looks every 0.3 to 0.5 s, and a turn is seen up to a step late
    CENTRE = 85.0               # the player's capsule centre over the navigation path

    def __init__(self, g, awake=None):
        self.g = g
        self.cones = {c[0]: c for c in g.ed.json(CONES)}
        self.clock = {name: (awake + c[1] + self.FIRST_LOOK if awake is not None else None)
                      for name, c in self.cones.items()}  # a time the cone turned on
        self.seen = {}
        self.samples, self.stretches, self.at_sample = [], [], 0
        self.holding = False

    def looking(self, name, t):
        on_at = self.clock.get(name)
        return on_at is None or (t - on_at) % self.CYCLE < self.ON

    def covers(self, p):
        names = []
        for name, (_, _, l, f, angle, length) in self.cones.items():
            v = [p[i] - l[i] for i in range(3)]
            d = math.sqrt(sum(c * c for c in v))
            if 1.0 < d < length + self.MARGIN_LENGTH:
                cos = sum(v[i] * f[i] for i in range(3)) / d
                if math.degrees(math.acos(max(-1.0, min(1.0, cos)))) < angle + self.MARGIN_ANGLE:
                    names.append(name)
        return names

    def prepare(self, path):
        """Samples the way from the player along the path and finds the stretches the cones look at."""
        here = self.g.status()["player"]
        points = [[here[0], here[1], here[2] - self.CENTRE]] + path
        self.samples, arc = [], 0.0
        for a, b in zip(points, points[1:]):
            length = flat(a, b)
            n = max(1, int(length // self.STEP))
            for k in range(n):
                t = k / n
                p = [a[i] + (b[i] - a[i]) * t for i in range(3)]
                p[2] += self.CENTRE
                self.samples.append((arc + length * t, p[0], p[1], self.covers(p)))
            arc += length
        end = points[-1]
        self.samples.append((arc, end[0], end[1], self.covers([end[0], end[1], end[2] + self.CENTRE])))
        self.stretches = []
        for at, _, _, names in self.samples:
            if not names:
                continue
            if self.stretches and self.stretches[-1][1] >= at - 1.5 * self.STEP:
                self.stretches[-1][1] = at
                self.stretches[-1][2].update(names)
            else:
                self.stretches.append([at, at, set(names)])
        self.at_sample = 0
        self.g.log("cones on the way: %s" % ", ".join("%.0f-%.0f cm %s" % (a, b, "/".join(sorted(n)))
                                                     for a, b, n in self.stretches))

    def hold(self, step):
        now = step["time"]
        for name, on in step["cones"].items():
            if name in self.seen and self.seen[name] != on:
                self.clock[name] = now if on else now - self.ON
            self.seen[name] = on
        window = range(self.at_sample, min(len(self.samples), self.at_sample + 40))
        self.at_sample = min(window, key=lambda i: flat(self.samples[i][1:3], step["at"]))
        here = self.samples[self.at_sample][0]
        blocking = set()
        for start, end, names in self.stretches:
            if end < here or start <= here or start - here > self.LOOK_AHEAD:
                continue  # behind, inside (the way out is on), or not yet near
            t, t_out = max(0.0, (start - here) / self.RUN - 0.4), (end - here) / self.SLOW + self.SLACK
            for name in names:
                if name not in step["cones"]:
                    continue  # its sentry is gone or has left its shelf
                if step["cones"][name] and t < 0.5 or any(self.looking(name, now + t + k * 0.1)
                                                          for k in range(int((t_out - t) / 0.1) + 1)):
                    blocking.add(name)
        if bool(blocking) != self.holding:
            self.holding = bool(blocking)
            self.g.log(("waiting for %s" % "/".join(sorted(blocking))) if blocking else "going on")
        return self.holding


class Play:
    def __init__(self, editor, viewport, shots):
        self.ed = editor
        self.viewport = viewport
        self.shots = shots
        self.keys_down = []
        self.log_start = time.time()

    # --- reporting ---------------------------------------------------------------------------------------------------
    def log(self, text):
        print("[%6.1f] %s" % (time.time() - self.log_start, text), flush=True)

    def status(self):
        return self.ed.json(STATUS)

    def brief(self, s=None):
        s = s or self.status()
        if not s.get("pie"):
            return "not in PIE"
        if s.get("title"):
            return "%s cp %s lives %s title%s%s" % (s["world"].replace("UEDPIE_0_", ""), s.get("checkpoint"), s.get("lives"),
                                                    " RESUME" if s.get("resume") else "", " POPUP" if s.get("popup") else "")
        p = s.get("player")
        return "%s cp %s lives %s shards %s obj %r player %s%s" % (
            s["world"].replace("UEDPIE_0_", ""), s.get("checkpoint"), s.get("lives"), s.get("shards"),
            s.get("objective"), "(%.0f, %.0f, %.0f) yaw %.0f" % tuple(p[:4]) if p else None,
            " PAUSED" if s.get("paused") else "")

    def shot(self, name):
        if not self.shots:
            return
        answer = desktop.request("shot", region=list(self.viewport), scale=0.5, name="pt_%s.png" % name)
        self.log("shot %s" % (answer.get("result", {}).get("path") if answer.get("ok") else answer))

    # --- input -------------------------------------------------------------------------------------------------------
    def send(self, cmd, **payload):
        payload.setdefault("allow", ALLOW)
        answer = desktop.request(cmd, timeout=60, **payload)
        if not answer.get("ok"):
            raise Failed("desktop %s: %s" % (cmd, answer.get("error")))
        return answer["result"]

    def focus(self):
        """Clicks the middle of the viewport so that the game has the keyboard (before anything turns the view)."""
        left, top, right, bottom = self.viewport
        self.send("click", x=(left + right) // 2, y=(top + bottom) // 2)
        time.sleep(0.3)

    def click_at(self, fx, fy):
        """Clicks the viewport at a fraction of its width and height (a button of a screen with the cursor)."""
        left, top, right, bottom = self.viewport
        self.send("click", x=round(left + (right - left) * fx), y=round(top + (bottom - top) * fy))

    def click_widget(self, cls, name):
        """Clicks the middle of a screen's button, found by the widget's class and the button's name."""
        at = self.ed.json(WIDGET_AT.format(cls=cls, name=name))
        if not at:
            raise Failed("no %s button %s on the screen" % (cls, name))
        self.log("click %s.%s at (%.3f, %.3f) of the viewport" % (cls, name, at[0], at[1]))
        self.click_at(*at)

    def key(self, *names, gap_ms=80):
        self.send("key", keys=list(names), gap_ms=gap_ms)

    def down(self, names):
        self.send("down", keys=list(names))
        self.keys_down = list(names)

    def up(self):
        if self.keys_down:
            desktop.request("up", keys=self.keys_down)  # no window check: letting go never acts on anything
            self.keys_down = []

    def console(self, *commands):
        self.ed.run(pie.COMMANDS.format(commands=list(commands)))
        self.log("console: %s" % "; ".join(commands))

    def place(self, x, y, z=90.15, yaw=-90.0, pitch=0.0):
        self.ed.run(pie.PLACE.format(x=x, y=y, z=z, yaw=yaw, pitch=pitch))
        self.log("place (%.0f, %.0f, %.0f) yaw %.0f" % (x, y, z, yaw))

    def face(self, yaw, pitch=None):
        self.ed.run(FACE.format(yaw=yaw, pitch=pitch))

    # --- waiting -----------------------------------------------------------------------------------------------------
    def wait_for(self, what, test, timeout, every=0.25):
        deadline = time.time() + timeout
        while True:
            s = self.status()
            if test(s):
                return s
            if time.time() > deadline:
                raise Failed("%s did not happen within %.0f s (%s)" % (what, timeout, self.brief(s)))
            time.sleep(every)

    def wait_game_time(self, seconds):
        self.wait_for("game time %.1f" % seconds, lambda s: s.get("pie") and s.get("time", 0) >= seconds, seconds + 60)

    def mark_world(self):
        """Tags the game mode of the world in play, so that the next one is told from it."""
        self.ed.run(MARK_WORLD)

    def wait_new_world(self, level, timeout=90, warmup=2.0, mark=True):
        """Waits for the level to be opened anew (a death, the ambulance, an open) and to have a player. mark False
        when the world was marked before what opens it (an open travels on the next tick)."""
        if mark:
            self.mark_world()
        self.log("waiting for %s to open" % level)

        def fresh(s):
            # the title has nobody to play: its screen instead
            return (s.get("pie") and level in s.get("world", "") and (s.get("title") if level == TITLE else s.get("player"))
                    and not s.get("marked"))

        s = self.wait_for("%s to open" % level, fresh, timeout, every=0.5)
        time.sleep(warmup)
        s = self.status()
        self.log("opened: " + self.brief(s))
        return s

    def expect(self, what, test, timeout=5.0):
        s = self.wait_for(what, test, timeout)
        self.log("ok: %s -- %s" % (what, self.brief(s)))
        return s

    # --- walking -----------------------------------------------------------------------------------------------------
    def nav_path(self, x, y, z=None):
        return self.ed.json(NAV_PATH.format(x=x, y=y, z=z))

    def walk(self, x, y, z=None, reach=70.0, timeout=60.0, until=None, keys=None, pitch=None, max_turn=35.0,
             straight=False, snap=False, guard=None, escape=False):
        """Walks (Shift + W) along the navigation path to (x, y) until within reach, or until until(step) is true.
        Returns the last step. The view keeps the pitch unless one is given; snap turns it to the path at once. A guard
        (Cones) stops the player, keys up, while guard.hold(step) is true; the time held does not count to the timeout.
        escape: the step says whether the escape's black fade is up ('escaped'), for an until."""
        path = None if straight else self.nav_path(x, y, z)
        if not path:
            if not straight:
                self.log("no navigation path to (%.0f, %.0f): straight line" % (x, y))
            path = [[x, y, z or 0.0]]
        else:
            path = path[1:] or [[x, y, z or 0.0]]
            path[-1] = [x, y, path[-1][2]]  # the path ends on the navmesh; walk to the point itself
        self.log("walk to (%.0f, %.0f): %d points" % (x, y, len(path)))

        def steer(index, turn):
            return self.ed.json(STEER.format(path=path, index=index, reach=max(reach, 110.0), max_turn=turn, pitch=pitch,
                                             escape=escape))

        if snap:
            steer(0, 180.0)
        if guard:
            guard.prepare(path)
        index, stuck_since, stuck_at, repaths, refocused = 0, None, None, 0, False
        deadline = time.time() + timeout
        started, first_at = time.time(), None
        held_since = None
        self.down(keys or WALK_KEYS)
        try:
            while True:
                step = steer(index, max_turn)
                index = step["index"]
                if until and until(step):
                    return step
                if index == len(path) - 1 and step["left"] <= reach:
                    return step
                if step["paused"] or step["captured"]:
                    raise Failed("stopped while walking (%s)" % ("paused" if step["paused"] else "captured"))
                if step["spotted"] and guard:
                    raise Failed("spotted by %s" % ", ".join(step["spotted"]))
                at = step["at"]
                if guard and guard.hold(step):
                    if held_since is None:
                        self.up()
                        held_since = time.time()
                    time.sleep(0.05)
                    continue
                if held_since is not None:
                    deadline += time.time() - held_since
                    held_since = None
                    self.down(keys or WALK_KEYS)
                    stuck_at, stuck_since = at, time.time()
                first_at = first_at or at
                if not refocused and time.time() - started > 1.5 and flat(at, first_at) < 1.0:
                    # not a step at all: the keys are not reaching the game (seen once after a few deaths and opens)
                    self.log("the keys do not reach the game: clicking the viewport again")
                    self.up()
                    self.focus()
                    self.down(keys or WALK_KEYS)
                    refocused = True
                    stuck_at, stuck_since = at, time.time()
                elif stuck_at is None or flat(at, stuck_at) > 40.0:
                    stuck_at, stuck_since = at, time.time()
                elif time.time() - stuck_since > 2.0:
                    repaths += 1
                    if repaths > 3:
                        raise Failed("stuck at (%.0f, %.0f) walking to (%.0f, %.0f)" % (at[0], at[1], x, y))
                    self.log("stuck at (%.0f, %.0f): path again" % (at[0], at[1]))
                    fresh = None if straight else self.nav_path(x, y, z)
                    if fresh and len(fresh) > 1:
                        path = fresh[1:]
                        path[-1] = [x, y, path[-1][2]]
                        index = 0
                        if guard:
                            guard.prepare(path)
                    stuck_at, stuck_since = at, time.time()
                if time.time() > deadline:
                    raise Failed("did not reach (%.0f, %.0f) within %.0f s (at (%.0f, %.0f))"
                                 % (x, y, timeout, at[0], at[1]))
                time.sleep(0.05)
        finally:
            self.up()

    def walk_through(self, points, **kwargs):
        step = None
        for point in points:
            step = self.walk(*point, **kwargs)
        return step

    # --- the game's things -------------------------------------------------------------------------------------------
    def shards(self):
        return self.ed.json("""
w = _need_game()
print('JSON ' + json.dumps([[a.get_actor_label(), a.get_actor_location().x, a.get_actor_location().y,
                             a.get_actor_location().z] for a in unreal.GameplayStatics.get_all_actors_of_class(w, unreal.WasamiShard)]))
""")

    def collect_all_but(self, keep, near, floor=None):
        """Collects every shard (as if touched, as Wasami.CollectShards does) but the keep nearest to near (on the floor
        of the height floor when given: Zone 2's maze has two)."""
        shards = sorted(self.shards(), key=lambda s: (floor is not None and abs(s[3] - floor) > 250.0, flat(s[1:3], near)))
        names = [s[0] for s in shards[keep:]]
        self.ed.run("""
w = _need_game()
names = set(%r)
for a in unreal.GameplayStatics.get_all_actors_of_class(w, unreal.WasamiShard):
    if a.get_actor_label() in names:
        a.collect(False)
""" % names)
        self.log("collected %d shards by hand, left %s" % (len(names), [(s[0], round(s[1]), round(s[2]))
                                                                      for s in shards[:keep]]))
        return shards[:keep]

    def enemies(self):
        return self.ed.json("""
w = _need_game()
out = []
for a in unreal.GameplayStatics.get_all_actors_with_tag(w, 'Enemy'):
    l = a.get_actor_location()
    chasing = a.is_chasing() if hasattr(a, 'is_chasing') else None
    out.append([a.get_actor_label() or a.get_name(), l.x, l.y, l.z, chasing])
print('JSON ' + json.dumps(out))
""")


# ------------------------------------------------------------------------------------------------------------------
# Sections. Each starts where the one before it stops; SETUPS put the game there for a section run alone.

ZONE1, ZONE2, TITLE = "L_Hospital_Zone1", "L_Hospital_Zone2", "L_Title"
# FadeOut's red flash (the line comes at 1.65 s) and its black (whole by 3.75 s), from NEW GAME's click (14 record).
TITLE_FLASH, TITLE_BLACK = 1.65, 4.0


def title(g):
    """The title screen's NEW GAME: with a game begun it asks RESTART? first and its YES goes on. The music fades, the
    screen pulses red and goes black, and 10 s on Zone 1 opens at the elevator's arrival with the save started over."""
    s = g.expect("the title screen", lambda s: s.get("title"))
    g.shot("title")
    g.mark_world()
    g.click_widget("WasamiTitleScreenWidget", "NewGame")
    if s.get("resume"):
        g.expect("the RESTART? question", lambda s: s.get("popup"))
        time.sleep(0.5)  # its fade in
        g.shot("title_restart")
        g.click_widget("WasamiPopUpWidget", "Yes")
    started = time.time()
    time.sleep(TITLE_FLASH)
    g.shot("title_flash")
    time.sleep(max(0.0, TITLE_BLACK - (time.time() - started)))
    g.shot("title_black")
    s = g.wait_new_world(ZONE1, mark=False, timeout=30)
    if s.get("checkpoint") != 4 or s.get("lives") != 3:
        raise Failed("Zone 1 did not open from the start with 3 lives (%s)" % g.brief(s))


def z1_arrive(g):
    """04: the elevator arrives, the lock on the doors in front is picked (F), the maze's box saves 5."""
    g.wait_game_time(11.5)  # the elevator's doors open at about 11 s
    g.shot("z1_arrive_doors")
    g.walk(0, 1010)
    g.face(-90.0, 0.0)
    # the lock shows 7 s after the doors open (the intercom); press F until the doors are unlocked
    deadline = time.time() + 20
    while not g.ed.json("""
w = _need_game()
box = [a for a in unreal.GameplayStatics.get_all_actors_of_class(w, unreal.WasamiDoorBreak)]
print('JSON ' + json.dumps(bool(box) and box[0].get_components_by_class(unreal.BoxComponent)[0].is_collision_enabled()))
"""):
        if time.time() > deadline:
            raise Failed("the lock did not show")
        time.sleep(0.3)
    g.shot("z1_arrive_lock")
    presses = 0
    while presses < 100:
        g.key(*(["f"] * 10), gap_ms=60)
        presses += 10
        locked = g.ed.json("""
w = _need_game()
doors = [a for a in unreal.GameplayStatics.get_all_actors_of_class(w, unreal.WasamiDoubleDoors) if a.get_actor_label() == 'BP_06_DoubleDoors11']
print('JSON ' + json.dumps(doors[0].get_editor_property('locked') if doors else None))
""")
        if not locked:
            break
    else:
        raise Failed("the doors stayed locked after 100 presses of F")
    g.log("the lock gave after %d presses of F" % presses)
    time.sleep(1.2)  # the doors swing open
    g.walk(15, 385)
    g.expect("COLLECT ALL SHARDS (saved 5)", lambda s: s.get("checkpoint") == 5 and "SHARDS" in s.get("objective", "").upper())
    g.shot("z1_arrive_maze")


def z1_maze(g):
    """05: pick up a few shards on foot, then walk into an enemy: caught, the death screen, 05 opened again."""
    s = g.status()
    start = s["player"]
    for _ in range(2):
        shard = min(g.shards(), key=lambda a: flat(a[1:3], start))
        before = g.status()["shards"]
        g.walk(shard[1], shard[2], reach=40.0)
        g.expect("shard %s picked up" % shard[0], lambda s: s["shards"] < before, 3.0)
        start = shard[1:3]
    lives = g.status()["lives"]
    enemy = min(g.enemies(), key=lambda e: flat(e[1:3], start))
    g.log("walking into %s at (%.0f, %.0f)" % (enemy[0], enemy[1], enemy[2]))
    deadline = time.time() + 90
    while True:
        enemy = next((e for e in g.enemies() if e[0] == enemy[0]), None)
        if enemy is None:
            raise Failed("the enemy went away")
        try:
            step = g.walk(enemy[1], enemy[2], reach=60.0, timeout=4.0, until=lambda st: st["captured"] or st["paused"])
        except Failed as error:
            if "captured" in str(error) or "within" in str(error):
                step = None
            else:
                raise
        s = g.status()
        if s.get("captured"):
            break
        if time.time() > deadline:
            raise Failed("not caught within 90 s")
    g.log("caught: " + g.brief(s))
    time.sleep(1.5)
    g.shot("z1_maze_capture")
    g.wait_for("the death screen", lambda s: s.get("paused"), 10)
    time.sleep(1.5)  # the screen fades in from black
    g.shot("z1_maze_death")
    g.wait_new_world(ZONE1)
    g.expect("05 again with a life less", lambda s: s.get("checkpoint") == 5 and s.get("lives") == lives - 1)


def z1_shards(g):
    """05 again: the rest of the shards but the one nearest are collected by hand; the last on foot breaks the barrier.
    Only one is left: the nurses patrol back towards 05 within seconds and chase faster (800 cm/s) than the player
    sprints (600), so a longer walk gets caught a second time."""
    s = g.status()
    left = g.collect_all_but(1, s["player"][:2])
    here = s["player"][:2]
    for shard in sorted(left, key=lambda a: flat(a[1:3], here)):
        g.walk(shard[1], shard[2], reach=40.0)
    g.expect("REACH THE PARKING LOT", lambda s: s["shards"] == 0 and "PARKING" in s.get("objective", "").upper(), 4.0)
    g.shot("z1_shards_done")


def z1_parking(g):
    """05 to 06: through the barrier's gap (placed in the corridor before it) to the box that fades to the car park."""
    g.place(0, -18000, yaw=-90.0)
    time.sleep(0.5)
    g.walk(4975, -23395, reach=40.0, timeout=90.0, until=lambda st: "TUNNEL" in st["objective"].upper())
    # the box fades to black and puts the player at 06_Start facing the two nurses that chase from 22 m away at
    # 800 cm/s from that moment: run at once (z1_ambulance), don't wait for the fade
    g.log("06: REACH THE TUNNEL -- " + g.brief())


def z1_ambulance(g):
    """06: past the doors that lock behind, into the tunnel, up the garage lift, Teleportation onto the ambulance's roof,
    the ride and the loading screen to Zone 2's cell. Starts running at once: the nurses are on the way."""
    g.walk(7210, -22255, reach=60.0, snap=True)
    g.shot("z1_ambulance_doors")
    g.walk(10375, -21600, reach=80.0)
    g.expect("GET ON TOP OF THE AMBULANCE", lambda s: "AMBULANCE" in s.get("objective", "").upper(), 4.0)
    g.walk(11249, -21250, reach=30.0)
    g.face(90.0, 0.0)
    time.sleep(3.0)  # the lift goes up in 2.5 s
    s = g.status()
    g.log("on the lift: " + g.brief(s))
    g.shot("z1_ambulance_lift")
    g.key("space")
    time.sleep(1.0)
    for _ in range(6):
        power = g.ed.json("""
w = _need_game()
player = unreal.GameplayStatics.get_player_character(w, 0)
print("JSON " + json.dumps(str(player.get_component_by_class(unreal.WasamiPowerComponent).get_socket_power(False))))
""")
        if "TELEPORT" in power.upper():
            break
        g.key("2")
        time.sleep(0.5)
    else:
        raise Failed("the right socket never showed Teleportation (%s)" % power)
    g.key("e")
    time.sleep(0.8)
    # the aim starts 1000 cm ahead and a notch of the wheel moves it 125 cm (Lv5, 04 record): aim at the middle of the
    # roof (BlockingVolume_Ambulance_5, y -20075); its back edge drops the player at a low frame rate (11 record)
    notches = max(0, min(4, round((-20075.0 - s["player"][1] - 1000.0) / 125.0)))
    for _ in range(notches):
        g.send("scroll", delta=120)
        time.sleep(0.2)
    g.log("aim: %d notches forward from y %.0f" % (notches, s["player"][1]))
    time.sleep(0.5)
    g.shot("z1_ambulance_aim")
    g.send("click")
    g.expect("on the roof: GOOD LUCK (saved 7)", lambda s: s.get("checkpoint") == 7, 5.0)
    time.sleep(4.0)
    g.shot("z1_ambulance_ride")
    g.wait_new_world(ZONE2, timeout=60)
    g.expect("Zone 2's cell (7)", lambda s: s.get("checkpoint") == 7)
    g.shot("z2_cell")


DOOR_BREAK = """
w = _need_game()
breaks = unreal.GameplayStatics.get_all_actors_of_class(w, unreal.WasamiDoorBreak)
boxes = breaks[0].get_components_by_class(unreal.BoxComponent) if breaks else []
print('JSON ' + json.dumps(bool(boxes) and boxes[0].is_collision_enabled()))
"""


def pick_lock(g, presses_max=100):
    """Presses F by the zone's door break until its box goes (the lock gives); the player stands in the box."""
    deadline = time.time() + 20
    while not g.ed.json(DOOR_BREAK):
        if time.time() > deadline:
            raise Failed("the lock did not show")
        time.sleep(0.3)
    presses = 0
    while presses < presses_max:
        g.key(*(["f"] * 10), gap_ms=60)
        presses += 10
        if not g.ed.json(DOOR_BREAK):
            g.log("the lock gave after %d presses of F" % presses)
            return
    raise Failed("the lock held after %d presses of F" % presses_max)


def z2_cell(g):
    """Zone 2's cell (7): the spikes come down from the opening and reach the player's head in about 19 s; the lock on
    the cell's door is picked (F), the player goes out and along to the corridor's box (saved 8, GET PAST THE NURSES),
    where the six sentries wake."""
    g.walk(-14145, 1330, reach=30.0, snap=True)
    g.face(-90.0, 0.0)
    g.shot("z2_cell_lock")
    pick_lock(g)
    time.sleep(1.0)
    g.shot("z2_cell_door")
    # the cell's navmesh ends at the door, which was shut when the paths were built: straight out through it first
    g.walk(-14145, 1000, reach=60.0, straight=True)
    step = g.walk(-11285, -93, reach=40.0, timeout=60.0, until=lambda st: st["checkpoint"] == 8)
    g.sentries_awake = step["time"]
    g.expect("GET PAST THE NURSES (saved 8)",
             lambda s: s.get("checkpoint") == 8 and "NURSES" in s.get("objective", "").upper())


def z2_corridor(g):
    """8: past the six sentries on their shelves to the maze's box (saved 9, COLLECT ALL SHARDS), stopping before a
    stretch of the way that a cone looks at while it looks or would look before the player is through (Cones)."""
    guard = Cones(g, getattr(g, "sentries_awake", None))
    g.shot("z2_corridor")
    g.walk(-3400, 0, reach=60.0, timeout=120.0, guard=guard, until=lambda st: st["checkpoint"] == 9)
    g.expect("COLLECT ALL SHARDS (saved 9)",
             lambda s: s.get("checkpoint") == 9 and "SHARDS" in s.get("objective", "").upper())
    g.shot("z2_maze_start")


def z2_maze(g):
    """9: the maze's shards but the one nearest on the player's floor are collected by hand; walking to the last saves
    10 (COLLECT THE RING PIECE) and takes the three nurses away."""
    s = g.status()
    left = g.collect_all_but(1, s["player"][:2], floor=s["player"][2])
    for shard in left:
        g.walk(shard[1], shard[2], reach=40.0)
    g.expect("COLLECT THE RING PIECE (saved 10)",
             lambda s: s.get("checkpoint") == 10 and "RING" in s.get("objective", "").upper(), 5.0)
    g.shot("z2_maze_done")


ALTAR = (-8584.0, -983.0, 120.0)  # where to look at the altar (ring_statue_2) from its way in
EYE = 95.0                        # the camera over the capsule's centre (02 record)


def z2_altar(g):
    """10: back along the corridor (its sentries are gone) to the altar; a click on it puts up the ring piece's screen
    (the game stops), CLOSE takes it down: the altar's lights, the barrier and the piece go and the garage's doors unlock
    (HEAD TOWARDS THE GARAGE)."""
    # The altar stands among stretchers and ambulances, off the navmesh (the level's NavModifierVolume2 keeps the nurses
    # out). The one way in for the player's capsule (radius 50) is from the north-west, between the stretchers west of
    # the altar and the ambulance north of it; from its end the altar is 2 m away over a stretcher (found with capsule
    # traces on a 40 cm grid, 2026-09-19).
    g.walk(-8360, 0, reach=60.0, timeout=90.0)
    g.walk_through([(-8880, -520), (-8880, -760), (-8760, -880)], reach=40.0, straight=True)
    here = g.status()["player"]
    dx, dy, dz = ALTAR[0] - here[0], ALTAR[1] - here[1], ALTAR[2] - (here[2] + EYE)
    g.face(math.degrees(math.atan2(dy, dx)), math.degrees(math.atan2(dz, math.hypot(dx, dy))))
    time.sleep(0.6)
    g.shot("z2_altar")
    left, top, right, bottom = g.viewport
    g.send("click", x=(left + right) // 2, y=(top + bottom) // 2)
    g.expect("the ring piece's screen", lambda s: s.get("paused"), 5.0)
    time.sleep(1.5)  # the piece grows and CLOSE bounces in (09 record)
    g.shot("z2_altar_piece")
    # CLOSE is about 100 px over the bottom of a 1038 x 656 viewport (09 record); a miss clicks nothing
    for dy in (100, 85, 115, 70, 130):
        g.send("click", x=(left + right) // 2, y=bottom - dy)
        time.sleep(1.2)
        if not g.status().get("paused"):
            break
    else:
        raise Failed("CLOSE did not take the ring piece's screen down")
    g.expect("HEAD TOWARDS THE GARAGE",
             lambda s: not s.get("paused") and "GARAGE" in s.get("objective", "").upper(), 5.0)


# The score screen: ShowResults' Delays give it the input about 7 s after it comes up (13 record), and NEXT sits at
# the viewport's lower right (at (2810, 824) of the 1040 x 652 viewport on 2026-09-19).
RESULTS_INPUT = 7.0
NEXT_BUTTON = (0.95, 0.948)


def z2_escape(g):
    """Through the unlocked doors to the garage's box (GET TO THE PORTAL: the portal opens), then along the garage to
    the portal: the player stops, the screen goes black and the game pauses under the score screen (You Escaped!, the
    results, FINAL RANK). Its NEXT fades out, and 5 s on the title opens with the save emptied (no RESUME)."""
    g.walk(-6300, -3950, reach=60.0, timeout=60.0, until=lambda st: "PORTAL" in st["objective"].upper())
    g.shot("z2_escape_garage")
    g.walk(-10357, -7700, reach=30.0, timeout=90.0, escape=True, until=lambda st: st["escaped"])
    g.log("escaped: " + g.brief())
    g.expect("the game paused under the score screen", lambda s: s.get("paused") and s.get("clear"))
    time.sleep(1.0)
    g.shot("z2_escape_escaped")
    time.sleep(RESULTS_INPUT)
    g.shot("z2_escape_results")
    g.mark_world()
    g.click_at(*NEXT_BUTTON)
    g.expect("the game unpaused 4 s after NEXT", lambda s: not s.get("paused"), timeout=8.0)
    s = g.wait_new_world(TITLE, mark=False)
    if s.get("checkpoint") != 0 or s.get("resume") or s.get("lives") != 3:
        raise Failed("the title did not open with the save emptied and 3 lives (%s)" % g.brief(s))


SECTIONS = [title, z1_arrive, z1_maze, z1_shards, z1_parking, z1_ambulance, z2_cell, z2_corridor, z2_maze, z2_altar, z2_escape]

# How a section run alone begins: the save's checkpoint (None: the save started over; KEEP: as it is) and the level
# opened again, then console commands.
KEEP = "keep"
SETUPS = {
    "title": (KEEP, TITLE, []),
    "z1_arrive": (None, ZONE1, []),
    "z1_maze": (5, ZONE1, []),
    "z1_shards": (5, ZONE1, []),
    "z1_parking": (5, ZONE1, ["Wasami.CollectShards"]),
    "z1_ambulance": (6, ZONE1, []),
    "z2_cell": (7, ZONE2, []),
    "z2_corridor": (8, ZONE2, []),
    "z2_maze": (9, ZONE2, []),
    "z2_altar": (10, ZONE2, []),
    "z2_escape": (10, ZONE2, ["Wasami.Flow OnRingPieceCollect"]),
}
# Sections that must start moving as soon as the level is up (the 06 nurses chase from the start, the cell's spikes
# come down).
NO_WARMUP = {"z1_ambulance", "z2_cell"}


def setup(g, name):
    checkpoint, level, commands = SETUPS[name]
    if checkpoint is None:
        g.console("Wasami.ResetSave")
    elif checkpoint != KEEP:
        g.console("Wasami.Checkpoint %d" % checkpoint, "Wasami.Lives 3")
    g.mark_world()
    g.console("open " + level)
    g.wait_new_world(level, mark=False, warmup=0.0 if name in NO_WARMUP else 2.0)
    if commands:
        g.console(*commands)
        time.sleep(1.0)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="cmd", required=True)
    sub.add_parser("list")
    sub.add_parser("status")
    p = sub.add_parser("run")
    p.add_argument("sections", nargs="*")
    p.add_argument("--from", dest="first")
    p.add_argument("--to", dest="last")
    p.add_argument("--setup", action="store_true", help="put the game where the first section begins")
    p.add_argument("--record", help="record the viewport to Intermediate/DesktopAgent/shots/<name>")
    p.add_argument("--record-seconds", type=float, default=900.0)
    p.add_argument("--fps", type=int, default=30)
    p.add_argument("--shots", action="store_true")
    p.add_argument("--viewport", nargs=4, type=int, default=VIEWPORT)
    p.add_argument("--no-focus", action="store_true", help="don't click the viewport first")
    args = ap.parse_args()

    names = [f.__name__ for f in SECTIONS]
    if args.cmd == "list":
        for f in SECTIONS:
            print("%-14s %s" % (f.__name__, " ".join(f.__doc__.split())))
        return 0

    editor = Editor()
    try:
        g = Play(editor, tuple(getattr(args, "viewport", VIEWPORT)), getattr(args, "shots", False))
        if args.cmd == "status":
            print(g.brief())
            return 0
        chosen = args.sections or names[names.index(args.first or names[0]):names.index(args.last or names[-1]) + 1]
        unknown = [n for n in chosen if n not in names]
        if unknown:
            print("unknown sections: %s (see list)" % ", ".join(unknown), file=sys.stderr)
            return 1
        if not g.status().get("pie"):
            print("PIE is not running (python Tools/pie.py start)", file=sys.stderr)
            return 1
        if not args.no_focus:
            g.focus()
        if args.setup:
            g.log("setup for %s" % chosen[0])
            setup(g, chosen[0])
        if args.record:
            answer = desktop.request("record", grab="gdi", region=list(args.viewport), seconds=args.record_seconds,
                                     fps=args.fps, name=args.record)
            g.log("recording: %s" % (answer.get("result") if answer.get("ok") else answer))
        for name in chosen:
            g.log("== %s: %s" % (name, " ".join(globals()[name].__doc__.split())))
            globals()[name](g)
            g.log("== %s done: %s" % (name, g.brief()))
        return 0
    except Failed as error:
        print("FAILED: %s" % error, file=sys.stderr)
        return 1
    finally:
        try:
            desktop.request("up", keys=WALK_KEYS, timeout=5)
        except Exception:
            pass
        editor.close()


if __name__ == "__main__":
    sys.exit(main())
