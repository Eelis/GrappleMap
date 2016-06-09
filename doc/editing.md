# GrappleMap Editor

## 1. Viewing transitions

Run "grapplemap-editor t#", where # is the transition number.

Use the mouse scroll wheel to cycle through the transition's frames, or
drag joints with the right mouse button along the path they take through
the transition.

Note that you can drag joints through connecting positions into
neighbouring transitions. This is very useful when you are making
edits that affect more than one transition (see below).


## 2. Editing transitions

In the editor, press 'v' to go into edit mode.

Drag joints with the left mouse button to change their position.

Press 's' to save.

Run "grapplemap-editor -h" for a complete list of controls.


## 3. Modifying connecting positions

A "connecting position" is one where transitions connect,
so connecting positions are the start and end positions
of transitions.

If in the editor you modify a connecting position, then
there are two options:

- If it was only a rotation/translation/mirroring modification, no worries.

- Otherwise (if it was a joint manipulation), all other transitions
  that had that position as their start or end position will be
  automatically updated as well.

So when editing connecting positions, you must be extra careful
to ensure that the movement is correct in all affected transitions.


## 4. Adding transitions 

### 4.1 Between two existing connecting positions

For both positions, find their code block in GrappleMap.txt. For example, if you want to start from the position named "combat base vs butterfly", you'd find this block:

	combat base\nvs butterfly
	tags: bottom_seated combat_base butterfly
		NzaApjBvaAAkOgderRD2aExFOvbPrIDvbGyuQVblx0FIh4xNNHfesYJXe6sUNDlgzbH9l
		bypOjgCyDHIgzygOsguCOHHgJCCOzgRD4HvgZDUN3gjE3H7gsERLDiSt7KPlQzjKLnhBB
		HZazAUL7azAFIUaEEwMLaEEiIsbzDyMtbDDjD6eBHHONfNH6HybGM8LcbQMJGyktMYLUk
		yMUFPf0LXOqgCMGFQfQHNNRg1IqF4gPGVOmhBHmGtgqFKNGhpGeJmfqOuJblhMsI9nBK4

The group of four indented lines together encode the position.

To add the stub for your new transition to the map, append the following lines
to the end of GrappleMap.txt:

- 1 line description
- 1 line: "properties: <list of properties>" (technically optional)
- 1 line: "tags: <list of tags>" (optional)
- 1 line: "ref: <description of source material>" (technically optional)
- 4-line code block for initial position
- 4-line code block for final position

Save the file (make sure your text editor uses Unix line endings), and now run
grapplemap-editor. By default, it loads the last transition in the file, so it'll
load the one you just added.

If you now go back and forth between the two frames using the scroll wheel, or
by dragging joints with the right mouse button, then unless you got lucky, you'll
see that the relative orientation of the start and end position is arbitrary, so
you'll have to fix it up by rotating/translating/mirroring (see -h for keys).

You can now start inserting and editing intermediate frames.

### 4.2 New positions

If you want to start or end a transition from a position that is not an existing
connecting position, but rather an intermediate position of a transition, then you will need to
split that transition at the position first, by pressing 'b' (for 'branch').
When you do, you'll see that the transition has been split at the current position.

You can now follow the procedure from the previous section.


## 5. Properties

### "top" / "bottom"

There are many positions where it is not meaningful to speak of the "top" or "bottom" player,
and where the classification is arbitrary. But even for transitions along such positions,
the top/bottom properties should be used to indicate which player's initiative it is.

For positions where there *is* a clear top and bottom player, but where the
wrong player is currently on top, you can swap the players by swapping the
first two lines with the last two lines of the code block. There is no need to modify
transitions that start/end at the position; when loading the file, the GrappleMap
tools understand that two positions that differ only in that the players are swapped,
are the same position.

### "bidirectional"

As the name suggests.
