var selected_tags = [];

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
		if (!node_has_tag(node, selected_tags[i]))
			return false;
	return true;
}

function trans_is_selected(trans)
{
	for (var i = 0; i != selected_tags.length; ++i)
		if (!trans_has_tag(trans, selected_tags[i]))
			return false;
	return true;
}

function tag_refines(t)
{
	var inclusions = false;
	var exclusions = false;

	for (var i = 0; i != transitions.length; ++i)
	{
		if (trans_is_selected(transitions[i]))
		{
			if (trans_has_tag(transitions[i], t))
				inclusions = true;
			else
				exclusions = true;
		}
	}

	return inclusions && exclusions;
}

function add_tag(t)
{
	selected_tags.push(t);
	update_tag_controls();
	generate_graph();
}

function remove_tag(t)
{
	selected_tags.splice(selected_tags.indexOf(t), 1);
	update_tag_controls();
	generate_graph();
}

function update_tag_controls()
{
	{
		var elem = document.getElementById("selected_tags");

		elem.innerHTML = "Tags:";

		for (var i = 0; i != selected_tags.length; ++i)
		{
			elem.appendChild(document.createElement("br"));
			elem.appendChild(document.createTextNode(" ▸ " + selected_tags[i] + " "));

			var btn = document.createElement("button");
			btn.style.margin = "3px";
			btn.appendChild(document.createTextNode("-"));
			btn.addEventListener("click", function(t){ return function(){
					remove_tag(t);
				}; }(selected_tags[i]) );

			elem.appendChild(btn);
		}
	}

	{
		var elem = document.getElementById("refinements");

		elem.innerHTML = "Refine:";

		for (var i = 0; i != tags.length; ++i)
			if (tag_refines(tags[i]))
			{
				var btn = document.createElement("button");
				btn.style.margin = "5px";
				btn.appendChild(document.createTextNode(tags[i]));
				btn.addEventListener("click", function(t){ return function(){
						add_tag(t);
					}; }(tags[i]) );

				elem.appendChild(btn);
			}
	}
}

function generate_graph()
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
	height = document.body.clientHeight;

	var svg = d3.select("#mynetwork").append("svg")
		.attr("width", width)
		.attr("height", height);

	svg.append('svg:defs').append('svg:marker')
		.attr('id', 'end-arrow')
		.attr('viewBox', '0 -5 10 10')
		.attr('refX', 6)
		.attr('markerWidth', 100)
		.attr('markerHeight', 100)
		.attr('orient', 'auto')
		.append('svg:path')
		.attr('d', 'M0,-1L2,0L0,1')
		.attr('fill', '#000');

	var force = d3.layout.force()
		.charge(-200)
		.gravity(0.01)
		.linkDistance(200)
		.size([width, height])
		.nodes(G.nodes)
		.links(G.links)
		.start();

	var link = svg.selectAll(".link")
		.data(G.links)
		.enter().append("line")
		.attr("class", "link")
		.style("stroke", function(d){
			if (d.transition.properties.indexOf("top") != -1) return "red";
			if (d.transition.properties.indexOf("bottom") != -1) return "blue";
			return "#999"; });

	var node = svg.selectAll(".node")
		.data(G.nodes)
		.enter()
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
		.attr("width", 150);

	var selected_node = null;

	label.on('mouseover', function(d)
		{
			selected_node = d.id;
			keyframe = nodes[d.id].position;
			tick_graph();
		});

	var bod = label.append("xhtml:div")
		.style("text-align", "center")
		.style("background", "white")
		.style("border", "solid 1px black");
	
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

		label
			.attr("x", function(d) { return d.x - 75; })
			.attr("y", function(d) { return d.y; });
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
			selected_tags = arg.split(",");
		}

		thepos = keyframe = nodes[0].position;

		canvas = document.getElementById('renderCanvas');
		engine = new BABYLON.Engine(canvas, true);

		makeScene();

		update_tag_controls();
		generate_graph();

		scene.activeCamera = externalCamera;
	});
