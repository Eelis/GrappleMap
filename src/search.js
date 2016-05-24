var selected_tags = [];
var drag = 0.20;
var view = [0, false];
var last_keyframe = null;
var selected_node = null;
var selected_nodes = [];
var substrs = [];
var reo = zero_reo();
var kf = 0;
var svg;
var force;

function node_has_tag(node, tag)
{
	return node.tags.indexOf(tag) != -1;
}

function trans_has_tag(trans, tag)
{
	return trans.tags.indexOf(tag) != -1;
}

function trans_kinda_has_tag(trans, tag)
{
	return (trans_has_tag(trans, tag) ||
		(node_has_tag(nodes[trans.from.node], tag)
		&& node_has_tag(nodes[trans.to.node], tag)));
}

function node_is_selected(node)
{
	for (var i = 0; i != selected_tags.length; ++i)
		if (node_has_tag(node, selected_tags[i][0]) != selected_tags[i][1])
			return false;

	for (var i = 0; i != substrs.length; ++i)
		if (node.description.indexOf(substrs[i]) == -1 &&
			!any_has_substr(node.tags, substrs[i]))
			return false;

	return true;
}

// todo: encode substrs in query string

function any_has_substr(a, s)
{
	for (var i = 0; i != a.length; ++i)
		if (a[i].toLowerCase().indexOf(s) != -1)
			return true;
	return false;
}

function trans_is_selected(trans)
{
	for (var i = 0; i != selected_tags.length; ++i)
		if (trans_kinda_has_tag(trans, selected_tags[i][0]) != selected_tags[i][1])
			return false;


	for (var i = 0; i != substrs.length; ++i)
		if (!any_has_substr(trans.description, substrs[i]) &&
			!any_has_substr(trans.tags, substrs[i]))
			return false;

	// todo: also implied tags?

	return true;
}

function tag_refines(tag)
{
	var r = {
		nodes_in: 0,
		nodes_out: 0,
		trans_in: 0,
		trans_out: 0 };

	nodes.forEach(function(n){
		if (node_is_selected(n))
			if (node_has_tag(n, tag)) ++r.nodes_in; else ++r.nodes_out;
	});

	transitions.forEach(function(trans){
		if (trans_is_selected(trans))
		{
			if (trans_kinda_has_tag(trans, tag))
				++r.trans_in;
			else
				++r.trans_out;
		}
	});

	return r;
}

function add_tag(t, b)
{
	selected_tags.push([t, b]);
	on_query_changed();
}

function remove_tag(t)
{
	selected_tags.splice(selected_tags.indexOf(t), 1);
	on_query_changed();
}

function on_query_changed()
{
	selected_nodes = [];
	for (var n = 0; n != nodes.length; ++n)
		if (node_is_selected(nodes[n]))
			selected_nodes.push(n);

	update_tag_list();
	update_position_pics();
	update_transition_pics();
	update_graph();

	history.replaceState(null, "", "index.html" + query_string_for_selection());
		// todo: handle back nav
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

function sorted_selected_tags()
{
	var r = [[], []];
	selected_tags.forEach(function(x) { r[x[1] ? 1 : 0].push(x[0]); });
	return r;
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

	document.getElementById('nonelabel').style.display =
		(excl_ref || (sorted_selected_tags()[0].length != 0)
			? 'inline'
			: 'none');
}

function query_string_for_selection()
{
	var s = "?";

	for (var i = 0; i != selected_tags.length; ++i)
	{
		var t = selected_tags[i];
		if (i != 0) s += ',';
		if (!t[1]) s += '-';
		s += t[0];
	}

	return s;
}

function refinements(b)
{
	var li = document.createElement("li");

	var options = document.createElement("select");
	options.onchange = function(b_){ return function(){ add_tag(options.value, b_); }; }(b);

	options.add(simple_option(""));

	var sst = sorted_selected_tags();

	tags.forEach(function(t)
	{
		var r = tag_refines(t);
		if (sst[b ? 1 : 0].indexOf(t) == -1
			&& ((r.nodes_in != 0 && r.nodes_out != 0) || (r.trans_in != 0 && r.trans_out != 0)))
			options.add(simple_option(t
				+ " (" + (b ? r.nodes_in : r.nodes_out)
				+ "p, " + (b ? r.trans_in : r.trans_out) + "t)", t));
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

	document.getElementById('pos_count_label').innerHTML = selected_nodes.length;

	var elem = document.getElementById("position_pics");

	document.getElementById("diagramlink").href = "../diagram/index.html?" + selected_nodes.join(",");

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
			link.href = "../composer/index.html?" + e; // todo: preserve view

			var img = document.createElement("img");
			img.src = "../t" + e + "200x150" + view_code(view) + ".gif";
			img.setAttribute('title', transitions[e].description);
			link.appendChild(img);

			elem.appendChild(link);
		});
}


function update_graph()
{
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

		var color = "black";
		if (t.properties.indexOf("top") != -1) color = "red";
		if (t.properties.indexOf("bottom") != -1) color = "blue";

		if (trans_is_selected(t)
			|| node_is_selected(nodes[t.to.node])
			|| node_is_selected(nodes[t.from.node]))
			G.links.push(
				{ source: addnode(t.from.node)
				, target: addnode(t.to.node)
				, id: i
				, color: color
				});
	}

	for (var i = 0; i != nn.length; ++i)
		G.nodes.push(nodes[nn[i]]);

	if (G.nodes.length > 50) return;

	force.nodes(G.nodes);
	force.links(G.links);
	force.start();

	make_svg_graph_elems(svg, G, force);
}

var targetpos;

function updateCamera()
{
}

function node_clicked()
{
}

function mouse_over_node(d)
{
	if (d.id == selected_node) return;

	if (!try_move(d.id))
	{
		selected_node = d.id;
		targetpos = thepos = last_keyframe = nodes[selected_node].position;
		queued_frames = [];
		reo = zero_reo();
	}

	tick_graph(svg);
}

function on_substrs()
{
	substrs = document.getElementById('substrs_box').value.split(' ');

	for (var i = 0; i != substrs.length; ++i)
		substrs[i] = substrs[i].toLowerCase();

	on_query_changed();
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
			});

		var s = window.location.href;
		var qmark = s.lastIndexOf('?');
		if (qmark != -1)
		{
			var arg = s.substr(qmark + 1);

			arg.split(",").forEach(function(a){
					if (a[0] == "-")
						selected_tags.push([a.substr(1), false]);
					else
						selected_tags.push([a, true]);
				});
		}

		thepos = last_keyframe = nodes[0].position;

		canvas = document.getElementById('renderCanvas');
		engine = new BABYLON.Engine(canvas, true);

		makeScene(thepos);

		make_graph();

		scene.activeCamera = externalCamera;

		update_view_controls();

		on_query_changed();
		selected_node = selected_nodes[0];

		window.addEventListener('resize', function() { engine.resize(); });

		tick_graph(svg);
	});
