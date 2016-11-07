# FAQ

## What is the GrappleMap?

1. A database of interconnected grappling positions and transitions,
   animated with stick figures.

2. A set of tools to build and explore this database.


## Which grappling techniques are included?

The intent is to map as many proven MMA-applicable grappling techniques as possible,
regardless of whether they come from wrestling, Brazilian Jiu-Jitsu, Judo,
Sambo, etc etc.


## What is included per technique?

The map only demonstrates techniques in their bare textbook form, and does not
include any discussion or explanation. However, the map almost always
cites source instructional materials, which readers are strongly
encouraged to consult for detailed breakdowns and tactics.


## How is the map organized?

### 1) as a graph

Techniques are modeled in a big [directed graph](https://en.wikipedia.org/wiki/Graph_%28discrete_mathematics%29),
where each vertex is
a concrete pose of the two stick figure players, and each edge is a transition from one such position to another.

### 2) by tags

In addition to being interconnected, positions and transitions are also named and tagged.
Tags are used for categorization into domains (e.g. deep_half, side\_control),
but also for more detailed information about poses (e.g. bottom\_supine, top\_posture\_broken),
and also for presence of any specific grips or controls (e.g. lockdown, kimura, crossface, arm_drag).


## What is the purpose of the map?

- To summarize proven techniques that address various situations
- To serve as an index into the literature, giving suggestions for specifically relevant instructional materials
- To give concrete ideas for drills to practice
- To (maybe one day) serve as training data set for machine learning algorithms to discover new jiu jitsu
- To be beautiful


## Won't it take forever to get decent coverage and quality?

If I have to do it all by myself, probably yes.
But I hope that once word gets around about the GrappleMap,
other grappling nerds will want to chip in and help me make it
the most awesome (no-gi) grappling map ever. :)

See also [Can I help map techniques?](#can-i-help-map-techniques).


## What tools are there?

Apart from the composer and explorer, which are part of the web pages,
there is a transition editor, and some utilities for making videos of
scripted or randomly generated matches, like this one:

[![demo](https://img.youtube.com/vi/sdygmrlm-ck/0.jpg)](https://www.youtube.com/watch?v=sdygmrlm-ck)


## What's the editor like?

It's a very minimalistic but highly specialized character animation editor
where you edit transitions simply by dragging the joints into place for each
respective keyframe. It is written in C++ and uses GLFW. It runs on Linux
and [Windows](http://eel.is/GrappleMap/windows-packages/)
(and probably other platforms, too, but I haven't tried).

A mini manual is [here](https://github.com/Eelis/GrappleMap/blob/master/doc/editing.md).

## What license is the code and data under?

None; the GrappleMap code and data is released into the public domain.


## How well can the map model grappling technique?

There are two aspects to this: the graph structure itself, and the
quality of the individual transition animations.

The graph structure itself is obviously a gross simplification.
Real grappling is a continuous space where
even individual positions and transitions occur in infinite variety.
Still, all grappling instructionals include examples
of forms that a technique can or should take.
The GrappleMap tries to connect and integrate all these fragmented
examples, in order to form a more complete picture.

The transition animations are pretty rudimentary, in part because
they are manually edited rather than motion-captured, and in part
because the animation system itself is currently based on
very simplistic fixed-interval keyframes, which limits joints to
5 direction changes per second.
This means that things like small-scale hand fighting and battling
for grips is barely modeled at all.
Usually the level of detail in the map is that a grip is either
acquired without great strain, or not at all, and that's it.
The timing of techniques is also not really captured at all.
The animations are really just schematics showing how the different
entanglements and postures flow into eachother, not live action.

## What about gi techniques?

I personally have no interest in gi-specific techniques, there are no such
techniques in the database, and clothes are not modeled in the software
(doing it right would be a lot of additional work).

If someone else wants to make GiGrappleMap, power to them!


## Wouldn't more realistic bodies/skeletons be better?

To make creating and editing transitions as simple as possible, the stick figures
originally had even fewer "joints"/"bones". I added more only as
necessary to allow execution of techniques I wanted to map. This
process resulted in the current stick figures, which I think strike a good
balance that fits the purpose of the GrappleMap.


## What about other body types?

I'm not really interested in mapping those few rare techniques that only work for
specific body types.


## What about the fence/cage (in MMA)?

It's on the [todo list](todo.txt).


## How about some nicer graphics?

Maybe later. (Patches welcome!)


## How come the animations run so poorly?

Try using Chrome. It's the only browser that I've ever gotten good WebGL performance out of. :/


## What is the database format?

The database is a [plain text file](https://github.com/Eelis/GrappleMap/blob/master/GrappleMap.txt) that declares:

1. Positions (with name, tags, description/reference, and joint coordinates)

2. Transitions (with name, tags, description/reference, and joint coordinates for each frame)

It's a pretty ad-hoc and minimalistic format, but it has the nice property that
textual diffs show which positions and transitions differ, and that an ordinary
text editor can be used to edit the names, descriptions, and tags.

I fully expect that the format will evolve into something different to accommodate future features.


## Do you train personally?

Back in university, I trained with their MMA club for a year or so, but it was super
casual, and a long time ago. I basically have no martial arts credentials whatsoever.


## Can I help map techniques?

Absolutely! There is *way* too much grappling technique for me to do this by myself,
so nothing would please me more than to see the GrappleMap become a collaborative effort.

The transition editor is minimalistic but gets the job done, is easy to build on Linux,
and a cross-compiled Windows package can be found [here](http://eel.is/GrappleMap/windows-packages/).
It is certainly my intention to make the tools usable by non-techies, though collaboration
through Github does involve version control, and editing the database text
file does require a decent text editor... Eventually I'll probably make the editor either
a Qt application, or make it run in the browser, or both. In any case, documentation
for the editor is [here](https://github.com/Eelis/GrappleMap/blob/master/doc/editing.md).

Note that unless you work on one of the areas where GrappleMap coverage is still
very sparse (such as takedowns or leg locks), adding content will not merely be a matter
of *adding* transitions, but will also very likely involve reworking or tweaking
existing transitions and positions, to reflect a more refined understanding of the
different angles, dynamics, and controls around the area in question. But that is to be expected:
every part of the map is fair game for refinement at all times.

Anyway, I hang out in #grapplemap on Freenode IRC. Do [drop by](https://webchat.freenode.net/) or mail me at grapplemap@contacts.eelis.net, I'll gladly help people get going.


## Can I help improve the software/website?

Absolutely! For ideas for things you could work on, see the [todo list](todo.txt).
