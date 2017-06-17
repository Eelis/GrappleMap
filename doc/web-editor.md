# GrappleMap Web Editor Manual

## 1. Introduction

The [GrappleMap editor](http://eel.is/GrappleMap/editor) is a simple character animation
program for networks of grappling positions. It operates on GrappleMap database
files, which contain only two things:

* positions (= a pose + some metadata)
* transitions (= 2 or more poses + some metadata)

On startup, the GrappleMap database is used, but another database file can also be loaded.

## 2. Browsing and Playback

At all times, there is a currently selected _path_ consisting of one or more transitions.
The path can be manipulated using the buttons on the path display.

You can scroll through the positions along the path by:

- using the mouse scroll wheel, or
- clicking and dragging joints with the right mouse button, or
- hovering over the path display in the side bar

To go to a position not along the current path, click 'Go to'.
Teleporting this way will reset the path.

## 3. Editing

### Local edits, and reorientations

A _local_ edit is one that only affects the current transition, not
other transitions or connecting positions. Because you don't want to
accidentally be making nonlocal edits, by default only local edits are allowed.

Edits to intermediate frames (i.e. frames other than the first and last
in the transition)
are always local. Edits to the first or last frame in a transition
(i.e. one that is at a connecting position) are only local if they are mere
reorientations.

A _mere reorientation_ is defined as a mirror operation,
a player swap, or a rotation or horizontal
translation of all of the two players' joints simultaneously.

### Identity transitions

Identity transitions are those whose end position is a mere reorientation
of their starting position. These are not allowed in the GrappleMap, and the editor will not let
you make them.

This means that a new transition from an existing position A
to a new to-be-defined position A' can only be created by first creating
A→B for some B≠A, then copying the first frame to produce A→A→B,
then editing the second frame to make it A→A'→B, and finally
deleting the third frame to make it A→A'. The editor simply will not permit the last
step until A→A' is not an identity transition.

### Metadata

Metadata for a transition or position is one or more lines of text, the first
line of which is the name.

A line beginning with "tags:" lists tags, while a line beginning with
"properties:" lists properties. The following properties are currently defined:

- "top" / "bottom"

  The editor does not care about these at all, but they are used in the other
  interfaces.

- "bidirectional"

  As the name suggests.

- "detailed"

  Doubles the keyframe density from 5 keyframes per second to 10. The increased control over timing and joint paths that this enables can make detailed transitions look significantly better, but they are a little bit more work to edit. A good approach is to start with a regular transition, and only make it detailed if it turns out to be necessary.

### Double-checking and contributing changes

When the database is written back to file after an editing session, a good way
to double check one's changes is by diff-ing with the previous version (e.g.
using [Meld](http://meldmerge.org/)).

There is no integrated way to submit changes to _the_ GrappleMap hosted at eel.is/GrappleMap.
If you want to contribute your work (please do!), then send a git pull request
and/or get in touch with Eelis.


