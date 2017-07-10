var selected_nodes = [];
var db;

function add_markers(svg)
{
	svg.append('svg:defs').append('svg:marker')
		.attr('id', 'red-arrow')
		.attr('viewBox', '0 -50 100 100')
		.attr('refX', 100)
		.attr('markerWidth', 25)
		.attr('markerHeight', 25)
		.attr('orient', 'auto')
		.style("fill", "red")
		.append('svg:path')
		.attr('d', 'M0,-10L37,0L0,10');

	svg.append('svg:defs').append('svg:marker')
		.attr('id', 'blue-arrow')
		.attr('viewBox', '0 -50 100 100')
		.attr('refX', 100)
		.attr('markerWidth', 25)
		.attr('markerHeight', 25)
		.attr('orient', 'auto')
		.style("fill", "blue")
		.append('svg:path')
		.attr('d', 'M0,-10L37,0L0,10');

	svg.append('svg:defs').append('svg:marker')
		.attr('id', 'black-arrow')
		.attr('viewBox', '0 -50 100 100')
		.attr('refX', 100)
		.attr('markerWidth', 25)
		.attr('markerHeight', 25)
		.attr('orient', 'auto')
		.style("fill", "black")
		.append('svg:path')
		.attr('d', 'M0,-10L37,0L0,10');

	svg.append('svg:defs').append('svg:marker')
		.attr('id', 'red-arrow-nonsel')
		.attr('viewBox', '-50 -50 50 100')
		.attr('markerWidth', 25)
		.attr('markerHeight', 25)
		.attr('orient', 'auto')
		.style("fill", "red")
		.append('svg:path')
		.attr('d', 'M-46,-10L-9,0L-46,10');

	svg.append('svg:defs').append('svg:marker')
		.attr('id', 'blue-arrow-nonsel')
		.attr('viewBox', '-50 -50 50 100')
		.attr('markerWidth', 25)
		.attr('markerHeight', 25)
		.attr('orient', 'auto')
		.style("fill", "blue")
		.append('svg:path')
		.attr('d', 'M-46,-10L-9,0L-46,10');

	svg.append('svg:defs').append('svg:marker')
		.attr('id', 'black-arrow-nonsel')
		.attr('viewBox', '-50 -50 50 100')
		.attr('markerWidth', 25)
		.attr('markerHeight', 25)
		.attr('orient', 'auto')
		.style("fill", "black")
		.append('svg:path')
		.attr('d', 'M-46,-10L-9,0L-46,10');
	
	// todo: this is horrible and gross, clean up
	// todo: can't we make markers that inherit the line color?
}

function get_id(x) { return x.id; }

function tick_graph(svg)
{
	svg.select("#links").selectAll(".link")
		.attr("x1", function(d) { return d.source.x; })
		.attr("y1", function(d) { return d.source.y; })
		.attr("x2", function(d) { return d.target.x; })
		.attr("y2", function(d) { return d.target.y; });

	svg.select("#nodes").selectAll(".node")
		.attr("cx", function(d) { return d.x; })
		.attr("cy", function(d) { return d.y; })
		.style("fill", function(d) {
				if (d.id == selected_node) return "lightgreen";
				return "white";
		});

	svg.select("#labels").selectAll(".link_label_group")
		.attr("transform", function(d) {
			return "translate(" +
				((d.source.x + d.target.x) / 2) + "," +
				((d.source.y + d.target.y) / 2) + ")";
		});

	svg.select("#labels").selectAll(".node_label_group")
		.attr("transform", function(d) { return "translate(" + d.x + "," + d.y + ")"; });
}

function mouse_over_transition(d)
{
/*
	d3.select("#infos").selectAll(".link_info").style("display", function(dd)
		{
			return (dd == d ? "inline" : "none");
		});
*/
}

function mouse_not_over_transition(d)
{
//	d3.select("#infos").selectAll(".link_info").style("display", "none");
}

function mouse_over_position(d)
{
/*
	d3.select("#infos").selectAll(".node_info").style("display", function(dd)
		{
			return (dd == d ? "inline" : "none");
		});
*/
	mouse_over_node(d);
}

function mouse_not_over_position(d)
{
//	d3.select("#infos").selectAll(".node_info").style("display", "none");
}

function make_svg_graph_elems(svg, G, force)
{
	var node_shapes = svg.select("#nodes").selectAll(".node").data(G.nodes, get_id);
	var link_shapes = svg.select("#links").selectAll(".link").data(G.links, get_id);
	var node_labels = svg.select("#labels").selectAll(".node_label_group").data(G.nodes, get_id);
	var link_labels = svg.select("#labels").selectAll(".link_label_group").data(G.links, get_id);
	var node_infos = d3.select("#infos").selectAll(".node_info").data(G.nodes, get_id);
	var link_infos = d3.select("#infos").selectAll(".link_info").data(G.links, get_id);

	{
		var s = link_labels.enter().append('g')
			.attr("class", "link_label_group")
			.on('mouseover', mouse_over_transition)
			.on('mouseout', mouse_not_over_transition);

		s	.append("text")
			.attr("class", "link_label")
			.attr("y", function(d){
				return db.transitions[d.id].desc_lines.length > 1 ? -5 : 0; })
			.text(function(d){
				var l = db.transitions[d.id].desc_lines[0];
				return (l != "..." ? l : ""); });

		s	.append("text")
			.attr("class", "link_label")
			.attr("y", 13)
			.text(function(d){
				var l = db.transitions[d.id].desc_lines;
				return (l.length > 1 ? l[1] : ''); });
	}

	link_shapes.enter()
		.append("line")
		.attr("class", "link")
		.style("stroke", function(d){ return d.color; })
		.on('mouseover', mouse_over_transition)
		.on('mouseout', mouse_not_over_transition);

	link_shapes.attr("marker-end", function(d){
			var sel = (selected_nodes.indexOf(d.target.id) != -1);
			return "url(#" + d.color + "-arrow" + (sel ? "" : "-nonsel") + ")";
		});

	node_infos.enter()
		.append("div")
		.attr("class", "node_info")
		.style("display", "none")
		.html(function(d){
				var n = db.nodes[d.id];
				return n.description + "<br>" +
					"tags: " + n.tags.join(", ") + "<br>" +
					"(p" + d.id + ")"; // todo: line #
			});

	link_infos.enter()
		.append("div")
		.attr("class", "link_info")
		.style("display", "none")
		.html(function(d){
				var trans = db.transitions[d.id];
				return trans.description.join("<br>") +
					"<br>(t" + d.id + " @ line " + trans.line_nr + ")";
			});

	node_shapes.enter().append("circle")
		.attr("class", "node")
		.on('click', node_clicked)
		.on('mouseover', mouse_over_position)
		.on('mouseout', mouse_not_over_position)
		.call(force.drag);

	node_shapes.attr("r", function(d)
			{
				return selected_nodes.indexOf(d.id) == -1 ? 7 : 50;
			});

	node_shapes.style("stroke-width", function(d)
			{
				return (selected_nodes.indexOf(d.id) == -1 ? 1.5 : 2) + "px";
			});

	{
		var s = node_labels.enter().append("g")
			.attr("class", "node_label_group")
			.on('click', node_clicked)
			.on('mouseover', mouse_over_position)
			.on('mouseout', mouse_not_over_position)
			.call(force.drag);

		s	.append("text")
			.attr("class", "node_label")
			.text(function(d){return d.desc_lines[0];})
			.attr("y", function(d){return [5,-5,-12][d.desc_lines.length-1];});

		s	.append("text")
			.attr("class", "node_label")
			.text(function(d){return d.desc_lines.length > 1 ? d.desc_lines[1] : '';})
			.attr("y", function(d){return [0,17,7][d.desc_lines.length-1];});

		s	.append("text")
			.attr("class", "node_label")
			.text(function(d){return d.desc_lines.length > 2 ? d.desc_lines[2] : '';})
			.attr("y", 26);
	}

	node_shapes.exit().remove();
	node_labels.exit().remove();
	link_shapes.exit().remove();
	link_labels.exit().remove();
}

function make_graph()
{
	svg = d3.select("#mynetwork");
	add_markers(svg);

	force = d3.layout.force()
		.charge(function(d)
			{
				if (selected_nodes.indexOf(d.id) != -1)
					return -1000;
				return -300;
				// todo: if this is not cached, cache it
			})
		.gravity(0.01)
		.linkDistance(200)
		.size([document.body.clientWidth, document.body.clientHeight]);

	svg.on("mouseup", function(){ force.alpha(0.01); });

	force.on("tick", function(){ tick_graph(svg); });
}

var selected_tags = [];
var drag = 0.20;
var view = [1, false];
var selected_node = null;
var selected_nodes = [];
var substrs = [];
var svg;
var force;
var paged_positions;
var paged_transitions;

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
		(node_has_tag(db.nodes[trans.from], tag)
		&& node_has_tag(db.nodes[trans.to], tag)));
}

function node_is_selected(node)
{
	for (var i = 0; i != selected_tags.length; ++i)
		if (node_has_tag(node, selected_tags[i][0]) != selected_tags[i][1])
			return false;

	for (var i = 0; i != substrs.length; ++i)
		if (!any_has_substr(node.description, substrs[i]) &&
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

	selected_nodes.forEach(function(n){
		if (node_has_tag(db.nodes[n], tag)) ++r.nodes_in; else ++r.nodes_out;
	});

	selected_edges.forEach(function(trans){
		if (trans_kinda_has_tag(db.transitions[trans], tag))
			++r.trans_in;
		else
			++r.trans_out;
	});

	return r;
}

var results_dirty = false;

function add_tag(t, b)
{
	selected_tags.push([t, b]);
	results_dirty = true;
}

function remove_tag(t)
{
	selected_tags.splice(selected_tags.indexOf(t), 1);
	results_dirty = true;
}

var selected_edges = [];

function recompute_results()
{
	if (results_dirty)
	{
		results_dirty = false;

		selected_nodes = [];
		for (var n = 0; n != db.nodes.length; ++n)
			if (node_is_selected(db.nodes[n]))
				selected_nodes.push(n);

		if (selected_nodes.length != 0)
			Module.cursor_canvas_goto(selected_nodes[0]);

		selected_edges = [];
		for (var n = 0; n != db.transitions.length; ++n)
			if (trans_is_selected(db.transitions[n]))
				selected_edges.push(n);

		update_tag_list();
		update_position_pics();
		update_transition_pics();
		update_graph();

		history.replaceState(null, "", "index.html" + query_string_for_selection());
			// todo: handle back nav
	}

	setTimeout(recompute_results, 100);
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
	if (selected_tags.length == 0) return "";

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

	db.tags.forEach(function(t)
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

function add_paged_elems(target, page_size)
{
	var page = 0;

	function refresh(items, renderitem)
	{
		target.innerHTML = "";

		if (items.length > page_size)
		{
			target.appendChild(document.createTextNode("Page: "));

			for (var i = 0; i*page_size < items.length; ++i)
			{
				if ((i+1) * page_size >= items.length && page > i)
					page = i;

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
							refresh(items, renderitem);
						};}(i));
					a.appendChild(document.createTextNode(i + 1));
					target.appendChild(a);
				}
			}

			target.appendChild(document.createElement("br"));
		}
		else page = 0;

		for (var i = page * page_size; i < items.length && i < (page + 1) * page_size; ++i)
		{
			renderitem(items[i], target);
		}
	}

	return refresh;
}

function position_image_title(n)
{
	return n.description.join("\n").replace(/\\n/g, ' '); // ?
}

function transition_image_title(t)
{
	return t.description.join('\n').replace(/\\n/g, ' ')
		+ "(t" + t.id + " @ line " + t.line_nr + ")";
}

function update_position_pics()
{
	document.getElementById('pos_count_label').innerHTML = selected_nodes.length;

	paged_positions(selected_nodes, function(n, target)
		{
			var link = document.createElement("a");

			var vc = view_code(view);

			link.href = "res/p" + n + vc + ".html";

			var img = document.createElement("img");
			img.src = image_url + "/p" + n + vc + "320x240.png";
			img.setAttribute('title', position_image_title(db.nodes[n]));
			link.appendChild(img);

			target.appendChild(link);
		});
}

function update_transition_pics()
{
	document.getElementById('trans_count_label').innerHTML = selected_edges.length;

	paged_transitions(selected_edges, function(e, target)
		{
			var link = document.createElement("a");
			link.href = "composer/index.html?" + e; // todo: preserve view

			var vid = document.createElement("video");
			vid.autoplay = true;
			vid.loop = true;
			vid.width = 200;
			vid.height = 150;
			vid.setAttribute('title', transition_image_title(db.transitions[e]));

			var vidsrc = document.createElement("source");
			vidsrc.src = image_url + "/t" + e + "200x150" + view_code(view) + ".mp4";
			vidsrc.type = "video/mp4";
			vid.appendChild(vidsrc);

			link.appendChild(vid);
			target.appendChild(link);
		});
}


function prepare_graph(frugal, ignore_selected_transitions)
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

	for (var i = 0; i != db.transitions.length; ++i)
	{
		var t = db.transitions[i];

		var color = "black";
		if (t.properties.indexOf("top") != -1) color = "red";
		if (t.properties.indexOf("bottom") != -1) color = "blue";

		function frugal_op(x, y) { return frugal ? (x && y) : (x || y); }

		if ((!ignore_selected_transitions && trans_is_selected(t))
			|| frugal_op(node_is_selected(db.nodes[t.to]),
			             node_is_selected(db.nodes[t.from])))
			G.links.push(
				{ source: addnode(t.from)
				, target: addnode(t.to)
				, id: i
				, color: color
				});
	}

	for (var i = 0; i != nn.length; ++i)
		G.nodes.push(db.nodes[nn[i]]);

	return G;
}

function update_graph()
{
	var nb = document.getElementById('neighbourhood');
	var nn = document.getElementById('no_neighbourhood');

	if (selected_nodes.length == 0)
	{
		nb.style.display = 'none';
		nn.style.display = 'inline';
		nn.innerHTML = "No positions to display.<br><br>";
		return
	}

	var limit = 30;

	var G = prepare_graph(false, false);
	if (G.nodes.length > limit)
		G = prepare_graph(true, false);
	if (G.nodes.length > limit)
		G = prepare_graph(true, true);

	var toobig = G.nodes.length == 0 || (G.nodes.length > limit);

	nb.style.display = (toobig ? 'none' : 'inline');
	nn.style.display = (toobig ? 'inline' : 'none');

	if (toobig)
	{
		nn.innerHTML = "Too many positions to display. (&gt; " + limit + ")<br><br>";
		return;
	}

	force.nodes(G.nodes);
	force.links(G.links);
	force.start();

	make_svg_graph_elems(svg, G, force);
}

var targetpos;

function node_clicked()
{
}

function goto_explorer()
{
	var connected_env = grow(selected_node,
		function(n)
		{
			return selected_nodes.indexOf(n) != -1;
		});

	window.location.href = "explorer/index.html?" + connected_env.join(",");
}

function mouse_over_node(d)
{
	if (d.id == selected_node) return;

	if (Module.cursor_canvas_goto(d.id))
	{
		selected_node = d.id;
		tick_graph(svg);
	}
}

function on_substrs()
{
	substrs = document.getElementById('substrs_box').value.split(' ');

	for (var i = 0; i != substrs.length; ++i)
		substrs[i] = substrs[i].toLowerCase();

	results_dirty = true;
}

function emscripten_loaded()
{
	db = Module.cursor_canvas_main();

	var canvas = document.getElementById('canvas');
	canvas.style.width = '100%';
	canvas.style.height = '100%';
		// because GLFW appears to override these
	canvas.style.backgroundColor = "black";

	db.transitions.forEach(function(t)
		{
			t.desc_lines = t.description[0].split('\\n'); // todo: really necessary?
		});

	db.nodes.forEach(function(n)
		{
			if (n.description.length == 0)
				n.desc_lines = "?";
			else
				n.desc_lines = n.description[0].split('\\n');

			n.x = Math.random() * 1000;
			n.y = Math.random() * 1000;
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

	paged_positions = add_paged_elems(document.getElementById('position_pics'), 9);
	paged_transitions = add_paged_elems(document.getElementById('transition_pics'), 12);

	make_graph();

	update_view_controls();

	results_dirty = true;

	recompute_results();

//	window.addEventListener('resize', function() { engine.resize(); });

	tick_graph(svg);

	sync_resolution();
}

function sync_resolution()
{
	var canvas = document.getElementById('canvas');
	Module.resolution(canvas.clientWidth, canvas.clientHeight);
	setTimeout(sync_resolution, 1000);
}

function on_view_change()
{
	var vv = document.getElementById("view_select").value;

	//scene.activeCamera = (vv == "external" ? externalCamera : firstPersonCamera);
}

var Module = {
	preRun: [],
	postRun: [],
	print: (function() { return; })(),
	printErr: function(text) {
		if (arguments.length > 1) text = Array.prototype.slice.call(arguments).join(' ');
		if (0) // XXX disabled for safety typeof dump == 'function') {
			dump(text + '\n'); // fast, straight to the real console
		else
			console.error(text);
	},
	setStatus: function(text) {
		console.log("status: " + text);
		if (text == "") emscripten_loaded();
		// todo: this is surely not the proper way to notice that emscripten is done loading
	},
	canvas: document.getElementById('canvas'),
	monitorRunDependencies: function(left) {}
};

window.onerror = function(event)
	{
		Module.setStatus = function(text) {
				if (text) Module.printErr('[post-exception status] ' + text);
			};
	};

function highlight_segment(seq, seg, pos)
{
}
