var selected_tags = [];
var drag = 0.20;
var view = [0, false];

function node_has_tag(node, tag)
{
	return node.tags.indexOf(tag) != -1;
}

function trans_has_tag(trans, tag)
{
	return trans.tags.indexOf(tag) != -1;
}

function node_is_selected(node)
{
	for (var i = 0; i != selected_tags.length; ++i)
		if (node_has_tag(node, selected_tags[i][0]) != selected_tags[i][1])
			return false;
	return true;
}

function trans_is_selected(trans)
{
	if (node_is_selected(nodes[trans.to.node]) && node_is_selected(nodes[trans.from.node]))
		return true;

	for (var i = 0; i != selected_tags.length; ++i)
		if (trans_has_tag(trans, selected_tags[i][0]) != selected_tags[i][1])
			return false;

	return true;
}

function tag_refines(t)
{
	var inclusions = 0;
	var exclusions = 0;

	nodes.forEach(function(n){
		if (node_is_selected(n))
		{
			if (node_has_tag(n, t)) ++inclusions;
			else ++exclusions;
		}
	});

	transitions.forEach(function(trans){
		if (trans_is_selected(trans))
		{
			if (trans_has_tag(trans, t)) ++inclusions;
			else ++exclusions;
		}
	});

	return [inclusions, exclusions];
}

function add_tag(t, b)
{
	selected_tags.push([t, b]);
	on_tag_selection_changed();
}

function remove_tag(t)
{
	selected_tags.splice(selected_tags.indexOf(t), 1);
	on_tag_selection_changed();

}

function on_tag_selection_changed()
{
	update_tag_list();
	update_position_pics();
	update_transition_pics();
	update_graph();
}

function opposite_heading(h) { return h < 2 ? h + 2 : h - 2; }

function heading_rotate_left(h) { return (h + 3) % 4; }
function heading_rotate_right(h) { return (h + 1) % 4; }

function view_rotate_left(v)
{
	return [heading_rotate_left(v[0]), v[1]];
}

function view_rotate_right(v)
{
	return [heading_rotate_right(v[0]), v[1]];
}

function view_mirror_x(v)
{
	return [(v[0] % 2) == 0 ? v[0] : opposite_heading(v[0]), !v[1]];
}

function view_mirror_y(v)
{
	return view_rotate_right(view_mirror_x(view_rotate_left(v)));
}

function update_view_controls()
{
	var elem = document.getElementById("view_controls");

	elem.innerHTML = "View: ";

	function control(label, f)
	{
		var a = document.createElement("a");
		a.href = "";
		a.addEventListener("click", function(e){
				e.preventDefault();
				view = f(view);
				update_position_pics();
				update_transition_pics();
			});
		a.appendChild(document.createTextNode(label));
		elem.appendChild(a);
	}

	control("↻", view_rotate_left);
	elem.appendChild(document.createTextNode(" "));
	control("↺", view_rotate_right);

	elem.appendChild(document.createTextNode(", "));

	control("⇄", view_mirror_x);
	elem.appendChild(document.createTextNode(" "));
	control("⇅", view_mirror_y);
}

function update_tag_list()
{
	var incl_ul = document.getElementById('include_tags');
	var excl_ul = document.getElementById('exclude_tags');

	incl_ul.innerHTML = "";
	excl_ul.innerHTML = "";

	selected_tags.forEach(function(st)
	{
		var u = document.createTextNode(st[0] + " ");

		var li = document.createElement("li");

		li.appendChild(u);

		var btn = document.createElement("button");
		btn.style.margin = "3px";
		btn.appendChild(document.createTextNode("×"));
		btn.addEventListener("click", function(t){ return function(){
				remove_tag(t);
			}; }(st) );

		li.appendChild(btn);

		if (st[1]) incl_ul.appendChild(li);
		else excl_ul.appendChild(li);
	});

	var incl_ref = refinements(true);
	if (incl_ref) incl_ul.appendChild(incl_ref);

	var excl_ref = refinements(false);
	if (excl_ref) excl_ul.appendChild(excl_ref);

	set_share_link();
}

function set_share_link()
{
	var url = "?";

	for (var i = 0; i != selected_tags.length; ++i)
	{
		var st = selected_tags[i];
		if (i != 0) url += ',';
		if (!st[1]) url += '-';
		url += st[0];
	}

	document.getElementById('share_link').href = url;
}

function refinements(b)
{
	var li = document.createElement("li");

	var options = document.createElement("select");
	options.onchange = function(b_){ return function(){ add_tag(options.value, b_); }; }(b);

	options.add(simple_option(""));

	tags.forEach(function(t)
	{
		var r = tag_refines(t);
		if (r[0] != 0 && r[1] != 0)
			options.add(simple_option(t + " (" + r[b ? 0 : 1] + ")", t));
	});

	if (options.length == 1) return null;

	li.appendChild(options);
	return li;
}

function simple_option(text, v)
{
	var opt = document.createElement("option");
	opt.text = text;
	if (v) opt.value = v;
	return opt;
}

function view_code(v)
{
	return ["nesw","NESW"][v[1] ? 1 : 0][v[0]];
}

var pos_page_index = 0;

function add_paged_elems(target, page_size, items, renderitem)
{
	var page = 0;

	function refresh()
	{
		target.innerHTML = "";

		if (items.length > page_size)
		{
			target.appendChild(document.createTextNode("Page: "));

			for (var i = 0; i*page_size < items.length; ++i)
			{
				target.appendChild(document.createTextNode(' '));

				if (i == page)
				{
					var b = document.createElement("b");
					b.appendChild(document.createTextNode(page + 1));
					target.appendChild(b);
				}
				else
				{
					var a = document.createElement("a");
					a.href = "";
					a.addEventListener("click", function(i_){return function(e){
							e.preventDefault();
							page = i_;
							refresh();
						};}(i));
					a.appendChild(document.createTextNode(i + 1));
					target.appendChild(a);
				}
			}

			target.appendChild(document.createElement("br"));
		}

		for (var i = page * page_size; i < items.length && i < (page + 1) * page_size; ++i)
		{
			renderitem(items[i], target);
		}
	}

	refresh();
}

function update_position_pics()
{
	var page_size = 12;

	var selected_nodes = [];
	for (var n = 0; n != nodes.length; ++n)
		if (node_is_selected(nodes[n]))
			selected_nodes.push(n);

	document.getElementById('pos_count_label').innerHTML = selected_nodes.length;

	var elem = document.getElementById("position_pics");

	add_paged_elems(elem, 9, selected_nodes, function(n, _)
		{
			var link = document.createElement("a");

			var vc = view_code(view);

			link.href = "../p" + n + vc + ".html";

			var img = document.createElement("img");
			img.src = "../p" + n + vc + "320x240.png";
			img.setAttribute('title', nodes[n].description);
			link.appendChild(img);

			elem.appendChild(link);
		});
}

function update_transition_pics()
{
	var elem = document.getElementById("transition_pics");

	var selected_edges = [];
	for (var n = 0; n != transitions.length; ++n)
		if (trans_is_selected(transitions[n]))
			selected_edges.push(n);

	document.getElementById('trans_count_label').innerHTML = selected_edges.length;

	add_paged_elems(elem, 12, selected_edges, function(e, _)
		{
			var link = document.createElement("a");
			link.href = "../composer/index.html?" + n; // todo: preserve view

			var img = document.createElement("img");
			img.src = "../t" + e + "200x150" + view_code(view) + ".gif";
			img.setAttribute('title', transitions[e].description);
			link.appendChild(img);

			elem.appendChild(link);
		});
}

function update_graph()
{
	d3.select("svg").remove();

	var nn = [];
	var G = { nodes: [], links: [] };

	function addnode(n)
	{
		var i = nn.indexOf(n);
		if (i == -1)
		{
			nn.push(n);
			i = nn.length - 1;
		}
		return i;
	}

	for (var i = 0; i != transitions.length; ++i)
	{
		var t = transitions[i];
		if (trans_is_selected(t)
			|| node_is_selected(nodes[t.to.node])
			|| node_is_selected(nodes[t.from.node]))
			G.links.push(
				{ source: addnode(t.from.node)
				, target: addnode(t.to.node)
				, transition: t
				});
	}

	for (var i = 0; i != nn.length; ++i)
	{
		var n = nodes[nn[i]];
		G.nodes.push(
			{ 'node': n
			, 'desc_lines': n.description.split('\n')
			, 'selected_by_tags': node_is_selected(n)
			});
	}

	if (G.nodes.length > 50) return;

	var width = document.body.clientWidth,
	height = 960;

	var svg = d3.select("#mynetwork").append("svg")
		.attr("width", width)
		.attr("height", height);

	svg.append('svg:defs').append('svg:marker')
		.attr('id', 'end-arrow')
		.attr('viewBox', '0 -50 100 100')
		.attr('refX', 100)
		.attr('markerWidth', 120)
		.attr('markerHeight', 120)
		.attr('orient', 'auto')
//		.style("fill", "red")
		.append('svg:path')
		.attr('d', 'M0,-5L20,0L0,5');

	var force = d3.layout.force()
		.charge(-200)
		.gravity(0.01)
		.linkDistance(200)
		.size([width, height])
		.nodes(G.nodes)
		.links(G.links)
		.start();

	var linksel = svg.selectAll(".link")
		.data(G.links)
		.enter();

	var link = linksel
		.append("line")
		.attr("class", "link")
		.attr("marker-end", "url(#end-arrow)") // hmm, why doesn't marker-pattern work..
		.style("stroke", function(d){
			if (d.transition.properties.indexOf("top") != -1) return "red";
			if (d.transition.properties.indexOf("bottom") != -1) return "blue";
			return "#999"; });

	var trans_label = linksel
		.append("text")
		.attr("text-anchor", "middle")
		.text(function(d){
			return d.transition.description[0];
			});

	var nodesel = svg.selectAll(".node")
		.data(G.nodes)
		.enter();

	var node = nodesel
		.append("circle")
		.attr("class", "node")
		.call(force.drag);
	
	var labelsel = svg.selectAll(".node_label")
		.data(G.nodes)
		.enter();

	var label = labelsel
		.append("text")
		.attr("text-anchor", "middle")
		.text(function(d){return d.desc_lines[0];})
		.call(force.drag);

	var label2 = labelsel
		.append("text")
		.attr("text-anchor", "middle")
		.text(function(d){return d.desc_lines[1];})
		.call(force.drag);

	var selected_node = null;

	label.on('mouseover', function(d)
		{
			selected_node = d.node.id;
			keyframe = nodes[selected_node].position;
			tick_graph();
		});

	function tick_graph()
	{
		link
			.attr("x1", function(d) { return d.source.x; })
			.attr("y1", function(d) { return d.source.y; })
			.attr("x2", function(d) { return d.target.x; })
			.attr("y2", function(d) { return d.target.y; });

		node
			.attr("cx", function(d) { return d.x; })
			.attr("cy", function(d) { return d.y; })
			.attr("r", function(d)
				{
					if (d.node.id == selected_node) return 75;
					if (d.selected_by_tags) return 50;
					return 5;
				});

		trans_label
			.attr("x", function(d) { return (d.source.x + d.target.x) / 2; })
			.attr("y", function(d) { return (d.source.y + d.target.y) / 2; });

		label
			.attr("x", function(d) { return d.x; })
			.attr("y", function(d) { return d.y - (d.desc_lines.length == 1 ? -5 : 0); });

		label2
			.attr("x", function(d) { return d.x; })
			.attr("y", function(d) { return d.y + 20; });
	}

	force.on("tick", tick_graph);
}


function interpolate(a, b, c)
{
	return a.scale(1 - c).add(b.scale(c));
}

function interpolate_position(a, b, c)
{
	var p = [[],[]];

	for (var pl = 0; pl != 2; ++pl)
	for (var j = 0; j != joints.length; ++j)
		p[pl].push(interpolate(a[pl][j], b[pl][j], c));

	return p;
}


function tick()
{
	thepos = (thepos ? interpolate_position(thepos, keyframe, drag) : keyframe);
}

function makeScene()
{
	scene = new BABYLON.Scene(engine);
	scene.clearColor = new BABYLON.Color3(1,1,1);

	externalCamera = new BABYLON.ArcRotateCamera("ArcRotateCamera",
		0, // rotation around Y axis
		3.14/4, // rotation around X axis
		2.5, // radius
		new BABYLON.Vector3(0, 0, 0),
		scene);

	externalCamera.wheelPrecision = 30;
	externalCamera.lowerBetaLimit = 0;
	externalCamera.upperRadiusLimit = 4;
	externalCamera.lowerRadiusLimit = 1;

	firstPersonCamera = new BABYLON.FreeCamera("FreeCamera", new BABYLON.Vector3(0, 1, -5), scene);
	firstPersonCamera.minZ = 0.05;

	externalCamera.attachControl(canvas);

	var light = new BABYLON.HemisphericLight('light1', new BABYLON.Vector3(0,1,0), scene);

	var draw = animated_position_from_array(keyframe, scene);

	scene.registerBeforeRender(function(){
			tick();
			if (thepos) draw(thepos);
		});

	engine.runRenderLoop(function(){ scene.render(); });
}

window.addEventListener('DOMContentLoaded',
	function()
	{
		var s = window.location.href;
		var qmark = s.lastIndexOf('?');
		if (qmark != -1)
		{
			var arg = s.substr(qmark + 1);

			arg.split(",").forEach(function(a){
					if (a[0] == "-")
						add_tag(a.substr(1), false);
					else
						add_tag(a, true);
				});
		}



		thepos = keyframe = nodes[0].position;

		canvas = document.getElementById('renderCanvas');
		engine = new BABYLON.Engine(canvas, true);

		makeScene();

		scene.activeCamera = externalCamera;



		update_view_controls();

		on_tag_selection_changed();
	});
