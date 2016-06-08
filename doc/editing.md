# Editing existing transitions

First, determine the number of the transition you want to edit. Then, run:

	grapplemap-editor t#

In the editor, press 'v' to go into edit mode.

Now use the mouse scrollwheel to cycle through the transition's frames, or
drag joints with the right mouse button along the path they take through
the transition.

Drag joints with the left mouse button to change their position.

Press 's' to save.

Run "grapplemap-editor -h" for a complete list of controls.


# Adding transitions between existing positions

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

After that, you can start inserting and editing intermediate frames.


# Modifying start/end nodes

If in the editor you modify a starting or ending position of a transition, then
there are two options:

- If it was only a rotation/translation/mirroring modification, no worries.

- Otherwise (if it was a joint manipulation), all other transitions
  that had that position as their start or end position will be
  automatically updated as well.

In other words, the start/end positions of transitions are shared
with other transitions connected to the same position, and so when
editing these connecting positions, you must be extra careful
to ensure that the movement is correct in all affected transitions.

This sounds hard, but to make it easy, if you drag joints
in *view* mode (to get there, press 'v' again), you can drag them *through*
the connecting points to seemlessly enter into the neighbouring
transitions. This makes it easy to "grab a joint" and drag it along
the path it takes through multiple connected transitions, to see
where edits need to be made, and to immediately make them.

So in an editing session, it is normal to not just edit one transition,
but to go back and forth between edit mode and view mode, to edit a
group of multiple connected transititions, changing both the branching
(start/end) positions as well as the intermediate frames.


# Adding positions

TODO


# Top vs bottom

There are many positions where it is not meaningful to speak of the "top" or "bottom" player,
and where the classification is arbitrary. But even for transitions along such positions,
the top/bottom properties should be used to indicate which player's initiative it is.

For positions where there *is* a clear top and bottom player, but where the
wrong player is currently on top, you can swap the players by swapping the
first two lines with the last two lines of the code block. There is no need to modify
transitions that start/end at the position; when loading the file, the software
understands that two positions that differ only in that the players are swapped,
are the same position.
