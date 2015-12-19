# FAQ

## What is this?

GrappleMap is:

1. a [database](http://eelis.net/GrappleMap/) of interconnected grappling positions
   and transitions, modeled with stick figures in 3D

2. a set of [tools](https://github.com/Eelis/GrappleMap) to build and explore that
   database, written in C++, and using OpenGL for rendering


## What tools do you have?

- grapplemap-gui is the editor and interactive explorer

- grapplemap-browse generates indexed HTML pages with pictures, animated gifs, and
  GraphViz graphs

- grapplemap-playback shows a scripted or random fight


## Why are you making this?

I want to have a fully interconnected, indexed, animated, reference-cited map of
MMA-applicable grappling technique, with convenient tools for exploring and extending it.

I'm not aware of an existing one, so I'm making one.

The EA UFC games do map a bunch of grappling, and have awesome graphics and animations.
But that data and code is completely closed, proprietary, and commercial, so not usable
for my purposes. Also, I would not be able to extend the movement data with data of the
same quality (because I don't have the resources for actual motion capturing).

Games are not my main goal, but maybe eventually I'll try making some silly games based
on the database. They'll probably only be playable by grappling afficionados though.


## What about gi techniques?

I personally have no interest in gi-specific techniques, there are no such
techniques in the database, and clothes are not modeled in the software
(which would be a lot of work).

If someone else wants to make GiGrappleMap, power to them!


## How lifelike are the bodies?

Not very. The bodies used are the glorified stick figures you see in the pictures.

Since editing simplicity was a priority, I started with even simpler stick figures
with fewer joint and simpler limbs. However, these could not faithfully express the
techniques I tried to map, so I made the bodies slightly more complex. Since the early
days, I've made a few more tweaks to limb proportions and connectedness, but by now
I'm pretty satisfied with the balance struck between editing simplicity and ability
to express technique.

The proportions of the limbs are roughly mine. I guess that means I can't map
techniques that only work with other body types, but I'm not too concerned about that.


## How lifelike are the movements?

There are some neatly polished transitions in the database here and there,
but most of it is still only very rough approximations that will need a *lot* of
fleshing out and polishing.

I'm not using motion capturing technology, so it's hard manual labour to make the
movements look good in the editor (where you manipulate the bodies as rag dolls by
dragging their joints).

Also, almost all the movements are taken from instructional materials, which only show
a few varieties per position. In actual fights, positions occur in infinite variety.

Also, I do not map the rapid small-scale hand fighting and battling for grips
and controls.
Usually the level of detail in the database is that a grip or control is either
acquired without great strain, or not at all, and that's it. Here, I benefit a
lot from the fact that no-gi grappling tends to be more about hooks.


## How about some nicer graphics?

For the near future, my focus is mostly on expanding and polishing the database,
and improving the tools to help me do that.

For nice graphics, the joint coordinates in the database can probably just be
fed into real skeletal animation systems in e.g. Blender or Ogre.
However, the result will still look robotic for the reasons mentioned in the
previous item.

I do definitely plan to add VR headset support, for example for the HTC Vive.
And when I do, I may well spend some time on at least making nice graphics for
backgrounds/surroundings, so I can have a sweet virtual dojo on top of Mount Everest,
and model technique there by walking around the bodies and adjusting joints directly.


## How are you deciding what to put in the database?

I pick and choose from the best instructional Jiu Jitsu materials that I have at
the moment (mostly from Marcelo Garcia, Ryan Hall, and Eddie Bravo), and stuff I
see in top level competition.

- What about non-grounded grappling like clinch fighting and takedowns?

  I'm open to it and want to try mapping pieces of it, but worry that the greater freedom
  of movement might be hard to map as a graph with fixed positions.

- What about striking?

  I personally don't have the stomach for mapping the art of ground-and-pound, but
  would accept patches that add strikes. We can easily tag these specific transitions,
  so that they can be filtered out if I want to generate a random pure grappling
  sequence.

  I do map strike *threats*, where the threat of a strike is either used to create
  a grapple opening, or where grappling is used to avert a strike (e.g. by disrupting
  the striker's base, preventing the strike from materializing).

- What about the fence/cage (in MMA)?

  I am not in principle opposed to adding fence support, but worry that it might be very
  hard to do properly, because there's so many angles at which you can be near the fence.


## What is the database format?

The database is a <a href='https://github.com/Eelis/GrappleMap/blob/master/positions.txt'>plain text file</a> that declares:

1. Positions (with name, tags, description/reference, and joint coordinates)

2. Transitions (with name, tags, description/reference, and joint coordinates for each frame)

It's a pretty ad-hoc and minimalistic format, but it has the nice property that
textual diffs show which positions and transitions differ, and that an ordinary
text editor can be used to edit the names, descriptions, and tags.

I fully expect that the format will evolve into something different to accommodate future features.


## Do you train personally?

No, I have no martial arts credentials whatsoever.


## Why not use existing open source character animation software as the starting point?

Because I didn't think that route would lead me to satisfactory results as fast.


## Why not write the code in Haskell (like your other projects)?

Because I want to eventually run the code in web browsers, and I expect that the C/C++
tooling for that will mature much earlier than the Haskell tooling.


## What license is the code and data under?

None. I don't believe in information ownership, so I donate everything into the public domain.

I'd love to see additional uses of the code/data!


## Can I help?

I can certainly use help, because there is *so much* grappling technique!

But:

- I haven't actually spent any effort yet to port the tools to Windows/WebGL,
  to document them, and to make them usable by non-techies

- There is no diff tool yet (it's on the todo), so modifications to existing positions
  and transitions are hard to review

- The GrappleMap must remain in the public domain, so contributions have to be as well

If all of that is no problem, mail me (grapplemap@contacts.eelis.net) or find me
as "Eelis" on Freenode IRC.
