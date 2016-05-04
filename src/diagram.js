var drag = 0.20;
var view = [0, false];
var queued_frames = [];
var last_keyframe = null;
var selected_node = null;
var reo = zero_reo();
var kf = 0;

var selected_nodes = [];

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

		var color = "black";
		if (t.properties.indexOf("top") != -1) color = "red";
		if (t.properties.indexOf("bottom") != -1) color = "blue";

		if (selected_nodes.indexOf(t.to.node) != -1 ||
			selected_nodes.indexOf(t.from.node) != -1)
			G.links.push(
				{ source: addnode(t.from.node)
				, target: addnode(t.to.node)
				, transition: t
				, color: color
				});
	}

	for (var i = 0; i != nn.length; ++i)
	{
		var n = nodes[nn[i]];
		G.nodes.push(
			{ 'node': n
			, 'desc_lines': n.description.split('\n')
			, 'selected': selected_nodes.indexOf(nn[i]) != -1
			});
	}

	if (G.nodes.length > 50) return;

	var width = document.body.clientWidth,
	height = 960;

	var svg = d3.select("#mynetwork").append("svg")
		.attr("width", width)
		.attr("height", height);

	svg.append('svg:defs').append('svg:marker')
		.attr('id', 'red-arrow')
		.attr('viewBox', '0 -50 100 100')
		.attr('refX', 100)
		.attr('markerWidth', 40)
		.attr('markerHeight', 40)
		.attr('orient', 'auto')
		.style("fill", "red")
		.append('svg:path')
		.attr('d', 'M0,-10L20,0L0,10');

	svg.append('svg:defs').append('svg:marker')
		.attr('id', 'blue-arrow')
		.attr('viewBox', '0 -50 100 100')
		.attr('refX', 100)
		.attr('markerWidth', 40)
		.attr('markerHeight', 40)
		.attr('orient', 'auto')
		.style("fill", "blue")
		.append('svg:path')
		.attr('d', 'M0,-10L20,0L0,10');

	svg.append('svg:defs').append('svg:marker')
		.attr('id', 'black-arrow')
		.attr('viewBox', '0 -50 100 100')
		.attr('refX', 100)
		.attr('markerWidth', 40)
		.attr('markerHeight', 40)
		.attr('orient', 'auto')
		.style("fill", "black")
		.append('svg:path')
		.attr('d', 'M0,-10L20,0L0,10');

	var force = d3.layout.force()
		.charge(-300)
		.gravity(0.01)
		.linkDistance(200)
		.size([width, height])
		.nodes(G.nodes)
		.links(G.links)
		.start();

	svg.on("mouseup", function(){ force.alpha(0.01); });

	var linksel = svg.selectAll(".link")
		.data(G.links)
		.enter();

	var link = linksel
		.append("line")
		.attr("class", "link")
		.attr("marker-end", function(d){ return "url(#" + d.color + "-arrow)"; })
		.style("stroke", function(d){ return d.color; });

	var trans_label = linksel
		.append("text")
		.attr("text-anchor", "middle")
		.text(function(d){ return d.transition.description[0]; });

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

	function node_clicked(d)
	{
		selected_nodes.push(d.node.id);
		update_graph();
	}

	function mouse_over_node(d)
	{
		var candidate = d.node.id;

		if (candidate == selected_node) return;

		var foundit = false;

		if (selected_node == null)
		{
			selected_node = candidate;
			tick_graph(); // todo: necessary?
			return;
		}

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

		if (!foundit)
		{
			selected_node = candidate;
			thepos = nodes[selected_node].position;
			reo = zero_reo();
		}

		tick_graph();
	}

	node.on('click', node_clicked);
	node.on('mouseover', mouse_over_node);
	//label.on('mouseover', mouse_over_node); // todo: rest of label as well

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
					if (d.selected) return 50;
					return 15;
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

var targetpos;

function tick()
{
	if (queued_frames.length != 0)
	{
		kf += 0.1;

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

			arg.split(",").forEach(function(a)
				{
					selected_nodes.push(parseInt(a));
				});
		}

		thepos = last_keyframe = keyframe = nodes[0].position;

		canvas = document.getElementById('renderCanvas');
		engine = new BABYLON.Engine(canvas, true);

		makeScene();

		update_graph();

		scene.activeCamera = externalCamera;
	});
