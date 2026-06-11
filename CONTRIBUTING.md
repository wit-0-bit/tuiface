# Contributing to Textface

Thanks for your interest! This page is for humans — whether you want to send
a change back or take the project somewhere else entirely. (AI agents and
day-to-day conventions: see [AGENTS.md](AGENTS.md).)

## The short version

- Bug fixes, test improvements, and legibility fixes: very welcome.
- New complications: **propose before you build.** Feel free to pitch
  anything, but know that I'm planning on being *very* selective. Watchfaces
  with seemingly hundreds of complications make it hard to find the three you
  give a shit about, and Textface is not going to become one of them. A "no"
  usually means "not at the cost of the face's focus," not "bad idea."
- Want a complication I turned down, or a different aesthetic? **Fork it!**
  Forking isn't the consolation prize here — it's half the plan. Textface
  only gets to stay small and opinionated because anyone who wants something
  different can cheaply make it. Notes below to get you going.

## Dev setup

You'll need the [Pebble SDK](https://developer.repebble.com). The `pebble`
CLI lives in a project-local virtualenv:

```sh
git clone --recurse-submodules <your-fork>   # test/unity is a submodule
source pebble-env/bin/activate
pebble build
pebble install --emulator emery
```

Unit tests run on the host with no SDK or emulator:

```sh
cd test && make test
```

## What a good PR looks like

1. **It's discussed first if it changes behavior or structure.** Open an
   issue describing the problem and your intended approach. For pure bug
   fixes (see [ISSUES.md](ISSUES.md) for known ones), you can skip straight
   to the PR.
2. **It comes with tests.** Everything testable gets tested — formatting,
   thresholds, theme logic. Add cases to `test/test_watchface.c` and register
   them with `RUN_TEST(...)`. `cd test && make test` must pass.
3. **It respects the project values** in [AGENTS.md](AGENTS.md): TUI-like but
   legible, high-contrast themes, minimal configuration, utility over
   approachability. PRs that add settings face extra scrutiny — every option
   has to earn its place.
4. **It follows the hard rules:**
   - `messageKeys` in `package.json` is **append-only** (reordering scrambles
     users' persisted settings).
   - `ComplicationDataSource` enum values are stable persisted identifiers —
     append, never renumber.
   - The edge sidebars (steps/battery) stay hardcoded —
     [docs/SIDEBARS.md](docs/SIDEBARS.md) explains why.

## Proposing a complication

Open an issue with: what data it shows, where it comes from (on-watch sensor
vs. phone fetch), which slot sizes it fits (top slots are wide, bottom slots
are narrow), and why you'd put it in one of *your* five slots. If it's
approved, [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md#adding-a-new-complication-source)
has the step-by-step implementation recipe.

Fair warning: I plan on being very choosy. I'm aiming to keep this watchface
easy to develop and maintain — I'm admittedly not the best at C, so a small,
simple codebase matters more to me than feature count. If I pass on your
idea, forks are warmly encouraged.

## Forking notes

Forks are encouraged — seriously. A fork with your name on it, your three
complications, and your palette is a *success* for this project, not a loss.
You don't need permission, and you don't need to feel bad about it. If you
build something neat, I'd love a link back, but even that's optional. When
you fork:

- **Change the `uuid` and `displayName`** in `package.json` (and ideally
  `name`/`author`) so your build doesn't collide with installed copies of
  Textface. Any UUID v4 generator works.
- `targetPlatforms` controls which watches you build for; this repo currently
  targets `emery` (Pebble Time 2), and the layout constants are hardcoded for
  its 200x228 screen. Supporting other platforms means deriving layout from
  `layer_get_bounds()` — a known limitation, not a design rule.
- Weather comes from [Open-Meteo](https://open-meteo.com) with no API key; be
  a good citizen with request frequency if you change the refresh logic.
- [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) is the map; everything else
  follows from there.

## Orientation

- [README.md](README.md) — what Textface is, for users
- [AGENTS.md](AGENTS.md) — values, conventions, hard rules
- [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) — how it works
- [docs/SIDEBARS.md](docs/SIDEBARS.md) — sidebar design rationale
- [ISSUES.md](ISSUES.md) — known bugs · [TODOs.md](TODOs.md) — idea backlog
