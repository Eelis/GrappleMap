var drag = 0.20;
var view = [0, false];
var last_keyframe = null;
var selected_node = null;
var reo = zero_reo();
var kf = 0;
var targetpos;

var svg;
var force;

function on_edit()
{
	node_selection_changed();

	document.getElementById('click_instruction').style.display =
		(document.getElementById('edit_mode_checkbox').checked
			? 'block'
			: 'none');
}

function on_mirror_button_clicked()
{
	mirror(targetpos);
	reo.mirror = !reo.mirror;
}

function auto_enable_edit_mode()
{
	if (selected_nodes.length <= 5)
	{
		document.getElementById('edit_mode_checkbox').checked = true;
		document.getElementById('click_instruction').style.display = 'block';
	}
}

function node_selection_changed()
{
	try
	{
		history.replaceState(null, "", "index.html?" + selected_nodes.join(","));
			// todo: use state
	}
	catch (e) {}

	var G = { nodes: [], links: [] };

	var nn = [];

	function addnode(n)
	{
		if (nn.indexOf(n) == -1)
		{
			G.nodes.push(nodes[n]);
			nn.push(n);
		}
	}

	selected_nodes.forEach(addnode);

	transitions.forEach(function(t)
		{
			if (document.getElementById('edit_mode_checkbox').checked &&
				(selected_nodes.indexOf(t.to.node) != -1 ||
				 selected_nodes.indexOf(t.from.node) != -1))
			{
				addnode(t.from.node);
				addnode(t.to.node);
			}
		});

	for (var i = 0; i != transitions.length; ++i)
	{
		var t = transitions[i];

		var color = "black";
		if (t.properties.indexOf("top") != -1) color = "red";
		if (t.properties.indexOf("bottom") != -1) color = "blue";

		if (nn.indexOf(t.to.node) != -1 && nn.indexOf(t.from.node) != -1)
		{
			addnode(t.from.node);
			addnode(t.to.node);

			G.links.push(
				{ source: nodes[t.from.node]
				, target: nodes[t.to.node]
				, id: i
				, color: color
				});
		}
	}

	force.nodes(G.nodes);
	force.links(G.links);

	G.nodes.forEach(function(n)
		{
			if (n.hasOwnProperty("x"))
				n.fixed = true;
		});

	force.start();

	for (var i = 0; i != 30; ++i) force.tick();

	G.nodes.forEach(function(n){ n.fixed = false; });

	make_svg_graph_elems(svg, G, force);

	tick_graph(svg);

}

function node_clicked(d)
{
	if (d3.event.defaultPrevented) return; // ignore drag

	if (!document.getElementById('edit_mode_checkbox').checked) return;

	var i = selected_nodes.indexOf(d.id);
	if (i == -1)
		selected_nodes.push(d.id);
	else if (selected_nodes.length >= 2)
		selected_nodes.splice(i, 1);
	else
		return;

	auto_enable_edit_mode();
	node_selection_changed();
}

function query_for(n)
{
	var q = n.tags.join(",");
	n.discriminators.forEach(function(d){ q += ",-" + d; });
	return q;
}

function update_links()
{
	document.getElementById('search_link').href =
		"../index.html?" + query_for(nodes[selected_node]);

	document.getElementById('composer_link').href =
		"../composer/index.html?p" + selected_node;
}

function mouse_over_node(d)
{
	if (d.id != selected_node && try_move(d.id))
	{
		tick_graph(svg);
		update_links();
	}
}

function updateCamera()
{
	var vv = document.getElementById("view_select").value;
	if (vv == "external") return;
	var pl = parseInt(vv);
	var opp = 1 - pl;

	firstPersonCamera.upVector = thepos[pl][Head].subtract(thepos[pl][Neck]);

	firstPersonCamera.setTarget(
		thepos[pl][LeftFingers]
		.add(thepos[pl][RightFingers])
		.scale(0.5));

	firstPersonCamera.position = thepos[pl][Head];
}

window.addEventListener('DOMContentLoaded',
	function()
	{
		transitions.forEach(function(t)
			{
				t.desc_lines = t.description[0].split('\n');
			});

		nodes.forEach(function(n)
			{
				n.desc_lines = n.description.split('\n');
				n.x = Math.random() * 1000;
				n.y = Math.random() * 1000;
			});

		var s = window.location.href;
		var qmark = s.lastIndexOf('?');
		if (qmark != -1)
		{
			var arg = s.substr(qmark + 1);

			arg.split(",").forEach(function(a)
				{
					selected_nodes.push(parseInt(a));
				});

			selected_node = selected_nodes[0];
		}
		else
		{
			selected_node = randInt(nodes.length);
			selected_nodes = [selected_node];
		}

		thepos = last_keyframe = keyframe = nodes[selected_node].position;

		canvas = document.getElementById('renderCanvas');
		engine = new BABYLON.Engine(canvas, true);

		makeScene(keyframe);

		update_links();
		make_graph();

		auto_enable_edit_mode();
		node_selection_changed();

		tick_graph(svg);

		scene.activeCamera = externalCamera;

		window.addEventListener('resize', function() { engine.resize(); });
	});


function on_view_change()
{
	var vv = document.getElementById("view_select").value;

	scene.activeCamera = (vv == "external" ? externalCamera : firstPersonCamera);
}
