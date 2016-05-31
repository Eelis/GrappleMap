Run "grapplemap-editor -h" in a command prompt for usage instructions.

grapplemap-editor operates on GrappleMap.txt, which is the database in (Unix) text format.

grapplemap-dbtojs converts GrappleMap.txt to transitions.js, which is used by the composer and explorer.

So the basic workflow at the moment is:
	1. do some editing with grapplemap-editor, or by hand in GrappleMap.txt (using
	    a text editor that understands Unix line endings);
	2. run grapplemap-dbtojs to regenerate transitions.js;
	3. look around with the composer or explorer to see if the changes look good.

(Generation of position pages with images and animated gifs doesn't work on Windows yet.)
