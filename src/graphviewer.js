var selected_tags = [];
var drag = 0.20;

function trans_has_tag(trans, tag)
{
	return (trans.tags.indexOf(tag) != -1
		|| nodes[trans.to.node].tags.indexOf(tag) != -1
		|| nodes[trans.from.node].tags.indexOf(tag) != -1);
}

function node_has_tag(node, tag)
{
	return node.tags.indexOf(tag) != -1;
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
	update_graph();
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

function update_position_pics()
{
	var elem = document.getElementById("position_pics");

	elem.innerHTML = "";

	var c = 0;

	for (var n = 0; n != nodes.length; ++n)
		if (node_is_selected(nodes[n]))
		{
			++c;

			var link = document.createElement("a");

			link.href = "../p" + n + "w.html";

			var img = document.createElement("img");
			img.src = "../p" + n + "w480x360-2.png";
			img.setAttribute('title', nodes[n].description);
			link.appendChild(img);

			elem.appendChild(link);
		}

	document.getElementById('matchheader').innerHTML = c + " matching positions:";
}

function update_graph()
{
	d3.select("svg").remove();

	var nn = [];
	var G = { nodes: [], links: [] };

	for (var i = 0; i != transitions.length; ++i)
	{
		var t = transitions[i];
		if (trans_is_selected(t))
		{
			var from = t.from.node;
			var to = t.to.node;
			var a = nn.indexOf(from);
			if (a == -1)
			{
				nn.push(from);
				a = nn.length - 1;
			}
			var b = nn.indexOf(to);
			if (b == -1)
			{
				nn.push(to);
				b = nn.length - 1;
			}

			G.links.push({source: a, target: b, transition: t});
		}
	}

	for (var i = 0; i != nn.length; ++i)
		G.nodes.push(nodes[nn[i]]);

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
		.text(function(d){
			return d.transition.description[0];
			});

	var nodesel = svg.selectAll(".node")
		.data(G.nodes)
		.enter();

	var node = nodesel
		.append("circle")
		.attr("class", "node")
		.attr("r", function(node){
			if (node_is_selected(node))
				return 10;
			else
				return 5;
			})
		.call(force.drag);
	
	var label = svg.selectAll(".tnode")
		.data(G.nodes)
		.enter()
		.append("foreignObject")
		.attr("width", 120);

	var selected_node = null;

	label.on('mouseover', function(d)
		{
			selected_node = d.id;
			keyframe = nodes[d.id].position;
			tick_graph();
		});

	var bod = label.append("xhtml:div")
		.style("text-align", "center");
//		.style("background", "white")
//		.style("border", "solid 1px black");
	
	bod.append("text")
		.attr("text-anchor",'middle')
		.text(function(d){return d.description;})
		.call(force.drag);

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
			.attr("r", function(node) { if (node.id == selected_node) return 20; else return 10; });

		trans_label
			.attr("x", function(d) { return (d.source.x + d.target.x) / 2; })
			.attr("y", function(d) { return (d.source.y + d.target.y) / 2; });

		label
			.attr("x", function(d) { return d.x - 60; })
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

			arg.split(",").forEach(function(t){ add_tag(t, true); });
		}

		thepos = keyframe = nodes[0].position;

		canvas = document.getElementById('renderCanvas');
		engine = new BABYLON.Engine(canvas, true);

		makeScene();

		on_tag_selection_changed();

		scene.activeCamera = externalCamera;
	});
