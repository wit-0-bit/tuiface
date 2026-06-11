# Contributing to Textface

To be honest, I didn't think anyone was going to be interested in downloading
this watchface, let alone want to contribute to it. It seemed a bit silly to
write a file that I fully expect nobody to read.

That said, I think it is fairly likely that you are in fact reading this file
right now. So, I will operate under the assumption that I am communicating to
someone interested in contributing to this watchface. If this is not the
case, I can only offer my sincere apologies.

So, since you *are* reading this, I suppose I should set some expectations and
conventions about how I am wanting to maintain this project. (This page is for
humans — whether you want to send a change back or take the project somewhere
else entirely. AI agents and day-to-day conventions: see
[AGENTS.md](AGENTS.md).)

## The short version

- Bug fixes, test improvements, and legibility fixes: very welcome.
- New complications: I recommend proposing a complication before spending
  significant time on it. Every complication has to justify its existence with
  a clear use case that shines as a complication.
- Want a complication I turned down, or a different aesthetic? **Fork it!**
  More projects should have opinions! A chorus of different perspectives on
  what a watchface can be is an ideal outcome as far as I am concerned.

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

1. **Ideally, it's discussed first if it changes behavior or structure.**
   Open an issue describing the problem and your intended approach. Again, I
   recommend reaching out beforehand to avoid wasted time, but I am not a
   stickler for process here. For pure bug fixes (see [ISSUES.md](ISSUES.md)
   for known ones), feel free to skip straight to the PR.
2. **It comes with tests, and it's formatted.** Everything testable gets
   tested — value formatting, thresholds, theme logic. Add cases to
   `test/test_watchface.c` and register them with `RUN_TEST(...)`. Run
   `make format` before committing; `make test` (clang-format check + unit
   tests) must pass, and CI runs the same checks on every PR.
3. **It respects the project values** (see [Philosophy in the
   README](README.md#philosophy)): TUI-like but legible, high-contrast themes,
   minimal configuration, utility over approachability. PRs that add settings
   face extra scrutiny — every option has to earn its place.
4. **It follows the hard rules:**
   - `ComplicationDataSource` enum values are stable persisted identifiers —
     append, never renumber. (Persistence keys live in
     [`messaging.h`](src/c/messaging.h) and are independent of `messageKeys`
     ordering, so that array is free to be reordered.)
   - The edge sidebars (steps/battery) stay hardcoded —
     [docs/SIDEBARS.md](docs/SIDEBARS.md) explains why.

## Proposing a complication

As I plan on being very selective about what I build/merge in, I recommend
proposing ideas before spending significant amounts of time and resources on
them — unless you are comfortable forking the watchface.

If you'd like to sound out an idea, open an issue with: what data it shows,
where it comes from (on-watch sensor vs. phone fetch), which slot sizes it
fits (top slots are wide, bottom slots are narrow), and why you'd put it in
one of *your* five slots. Either way,
[docs/ARCHITECTURE.md](docs/ARCHITECTURE.md#adding-a-new-complication-source)
has the step-by-step implementation recipe when you're ready to build.

## Forking notes

You don't need permission. Please just remember to **change the `uuid` and
`displayName`** in `package.json` (and ideally `name`/`author`) so your build
doesn't collide with installed copies of Textface. Any UUID v4 generator
works. [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) is the map for everything
else.

## Orientation

- [README.md](README.md) — what Textface is, for users
- [AGENTS.md](AGENTS.md) — values, conventions, hard rules
- [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) — how it works
- [docs/SIDEBARS.md](docs/SIDEBARS.md) — sidebar design rationale
- [ISSUES.md](ISSUES.md) — known bugs · [TODOs.md](TODOs.md) — idea backlog
