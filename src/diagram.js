var drag = 0.20;
var view = [0, false];
var queued_frames = [];
var last_keyframe = null;
var selected_node = null;
var reo = zero_reo();
var kf = 0;
var show_neighbours = false;

var selected_nodes = [];

var svg;
var force;

function make_graph()
{

	var width = document.body.clientWidth,
	height = document.body.clientHeight;

	svg = d3.select("#mynetwork").append("svg")
		.attr("width", '100%')
		.attr("height", '100%');

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

	svg.append("g").attr("id", "links");
	svg.append("g").attr("id", "nodes");
	svg.append("g").attr("id", "labels");

	force = d3.layout.force()
		.charge(function(d)
			{
				if (selected_nodes.indexOf(d.id) != -1)
					return -1000;
				return -300;
			})
		.gravity(0.01)
		.linkDistance(200)
		.size([width, height]);

	update_graph();
}

function on_edit()
{
	show_neighbours = !show_neighbours;
	document.getElementById('sharelinkspan').style.display = (document.getElementById('edit_mode_checkbox').checked ? 'inline' : 'none');
	update_graph();
}

function get_id(x) { return x.id; }

function update_graph()
{
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

	for (var i = 0; i != transitions.length; ++i)
	{
		var t = transitions[i];

		var color = "black";
		if (t.properties.indexOf("top") != -1) color = "red";
		if (t.properties.indexOf("bottom") != -1) color = "blue";

		if (show_neighbours
			? (selected_nodes.indexOf(t.to.node) != -1 || selected_nodes.indexOf(t.from.node) != -1)
			: (selected_nodes.indexOf(t.to.node) != -1 && selected_nodes.indexOf(t.from.node) != -1))
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

	document.getElementById("sharelink").href = "index.html?" + selected_nodes.join(",");

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

	svg.on("mouseup", function(){ force.alpha(0.01); });

	var nodesel = svg.select("#nodes").selectAll(".node").data(G.nodes, get_id);
	var labelsel = svg.select("#labels").selectAll(".combined_node_label").data(G.nodes, get_id);
	var linksel = svg.select("#links").selectAll(".link").data(G.links, get_id);
	var linklabelsel = svg.select("#labels").selectAll(".link_label").data(G.links, get_id);
	var linklabel2sel = svg.select("#labels").selectAll(".link_label2").data(G.links, get_id);

	linklabelsel.enter()
		.append("text")
		.attr("class", "link_label")
		.attr("text-anchor", "middle")
		.text(function(d){
			var l = transitions[d.id].desc_lines[0];
			return (l != "..." ? l : ""); });

	linklabel2sel.enter()
		.append("text")
		.attr("class", "link_label2")
		.attr("text-anchor", "middle")
		.text(function(d){
			var l = transitions[d.id].desc_lines;
			return (l.length > 1 ? l[1] : ''); });

	linksel.enter()
		.append("line")
		.attr("class", "link")
		.attr("marker-end", function(d){ return "url(#" + d.color + "-arrow)"; })
		.style("stroke", function(d){ return d.color; });

	nodesel.enter().append("circle")
		.attr("class", "node")
		.on('click', node_clicked)
		.style("fill", "url(#radial-gradient)")
		.on('mouseover', mouse_over_node)
		.call(force.drag);

	var label_groups = labelsel.enter()
		.append("g")
		.attr("class", "combined_node_label")
		.on('click', node_clicked)
		.on('mouseover', mouse_over_node)
		.call(force.drag);

	label_groups
		.append("text")
		.attr("text-anchor", "middle")
		.text(function(d){return d.desc_lines[0];})
		.attr("y", function(d){return [5,-5,-12][d.desc_lines.length-1];});

	label_groups
		.append("text")
		.attr("text-anchor", "middle")
		.text(function(d){return d.desc_lines.length > 1 ? d.desc_lines[1] : '';})
		.attr("y", function(d){return [0,17,7][d.desc_lines.length-1];});

	label_groups
		.append("text")
		.attr("text-anchor", "middle")
		.text(function(d){return d.desc_lines.length > 2 ? d.desc_lines[2] : '';})
		.attr("y", 26);

	linksel.exit().remove();
	nodesel.exit().remove();
	labelsel.exit().remove();
	linklabelsel.exit().remove();
	linklabel2sel.exit().remove();

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

		update_graph();
	}

	function mouse_over_node(d)
	{
		var candidate = d.id;

		var foundit = false;

		var n = nodes[selected_node];

		n.outgoing.forEach(function(s)
			{
				var t = transitions[s.transition];
				if (!foundit && !s.reverse && step_to(s).node == candidate)
				{
					foundit = true;

					reo = compose_reo(inverse_reo(step_from(s).reo), reo);

					for (var i = 1; i != t.frames.length; ++i)
						queued_frames.push(apply_reo(reo, t.frames[i]));

					reo = compose_reo(step_to(s).reo, reo);

					selected_node = candidate;
				}
			});

		n.incoming.forEach(function(s)
			{
				var t = transitions[s.transition];
				if (!foundit && !s.reverse && step_from(s).node == candidate)
				{
					foundit = true;

					reo = compose_reo(inverse_reo(step_to(s).reo), reo);

					for (var i = t.frames.length - 1; i != 0; --i)
						queued_frames.push(apply_reo(reo, t.frames[i - 1]));

					reo = compose_reo(step_from(s).reo, reo);

					selected_node = candidate;
				}
			});

			// todo: clean up
		tick_graph();
	}

	function tick_graph()
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
			})
			.attr("r", function(d)
				{
					return selected_nodes.indexOf(d.id) == -1 ? 7 : 50;
				});

		linklabelsel
			.attr("x", function(d) { return (d.source.x + d.target.x) / 2; })
			.attr("y", function(d) { return (d.source.y + d.target.y) / 2; });

		linklabel2sel
			.attr("x", function(d) { return (d.source.x + d.target.x) / 2; })
			.attr("y", function(d) { return (d.source.y + d.target.y) / 2 + 15; });

		svg.select("#labels").selectAll(".combined_node_label")
			.attr("transform", function(d) { return "translate(" + d.x + "," + d.y + ")"; });
	}

	tick_graph();

	force.on("tick", tick_graph);
}

var targetpos;

function tick()
{
	if (queued_frames.length != 0)
	{
		kf += Math.max(0.1, queued_frames.length / 80);

		if (kf < 1)
		{
			targetpos = interpolate_position(last_keyframe, queued_frames[0], kf);
		}
		else
		{
			kf = 0;
			targetpos = last_keyframe = queued_frames.shift();
		}
	}

	if (targetpos)
		thepos = (thepos ? interpolate_position(thepos, targetpos, drag) : targetpos);

	// todo: base on real elapsed time like composer does

	updateCamera();
}

function updateCamera()
{
	var vv = document.getElementById("view_select").value;
	if (vv == "external") return;
	var pl = parseInt(vv);
	var opp = 1 - pl;

	firstPersonCamera.setTarget(
		thepos[pl][LeftFingers]
		.add(thepos[pl][RightFingers])
		.add(thepos[opp][Head].scale(2))
		.scale(0.25));
	firstPersonCamera.fov = 1.6;
	firstPersonCamera.position = thepos[pl][Head];
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

			arg.split(",").forEach(function(a)
				{
					selected_nodes.push(parseInt(a));
				});

			selected_node = selected_nodes[0];
		}

		thepos = last_keyframe = keyframe = nodes[selected_node].position;

		canvas = document.getElementById('renderCanvas');
		engine = new BABYLON.Engine(canvas, true);

		makeScene();

		make_graph();

		scene.activeCamera = externalCamera;
	});


function on_view_change()
{
	var vv = document.getElementById("view_select").value;

	scene.activeCamera = (vv == "external" ? externalCamera : firstPersonCamera);
}
